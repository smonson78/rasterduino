#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <libserialport.h>
#include <FreeImage.h>

const char *serial_port = "/dev/ttyUSB0";

typedef struct sp_port sp_port_t;
typedef struct sp_port_config sp_port_config_t;
typedef enum sp_return sp_return_t;

uint16_t image_x, image_y;
uint16_t y_steps_per_scanline;
uint16_t backlash_compensation_steps;
uint16_t ramp_steps;
uint16_t velocity;
int final_width;

sp_port_t *port;

int get_response(int timeout)
{
    sp_return_t result;
    uint8_t buf;
    do
    {
        // Wait for # prefix
        result = sp_blocking_read(port, &buf, 1, timeout);
        if (result == 0)
        {
            return 0;
        }
    } while (buf != '#');
    
    // Now read second character
    result = sp_blocking_read(port, &buf, 1, 100);
    if (result == 0)
        return 0;
    
    return buf;
}

// Wait up to 200ms for handshake sequence ##
int handshake()
{
    sp_nonblocking_write(port, "##", 2);
    sp_drain(port);

    if (get_response(500) == '#')
        return 1;
    return 0;
}

void show_debug()
{
    while (1)
    {
        char buf[16];
        int result = sp_blocking_read(port, buf, 1, 500);
        if (result == 0)
        {
            break;
        }
        printf("%c", buf[0]);
    }
    printf("\n");
}

// The device should respond #Y or #N
void wait_for_ok()
{
    int response = get_response(200);
    if (response == 0)
    {
        fprintf(stderr, "No response from device.\n");
        show_debug();
        exit(5);
    } 
    else if (response == 'N')
    {
        fprintf(stderr, "Device reported error response.\n");
        show_debug();
        exit(5);
    }
    else if (response != 'Y')
    {
        printf("Unexpected device reponse: #%c\n", response);
        show_debug();
        exit(5);
    }
}

void send_command(const char *command)
{
    printf("--> %s\n", command);
    sp_nonblocking_write(port, command, strlen(command));
    sp_drain(port);
    wait_for_ok();
    printf("    OK\n");
}

int do_parameters(int argc, char **argv)
{
    int c;
    
    // Defaults
    backlash_compensation_steps = 0;
    y_steps_per_scanline = 5;
    ramp_steps = 1000;
    velocity = 500;
    final_width = -1;
        
    while ((c = getopt(argc, argv, "b:v:r:s:w:")) != -1)
    {
        switch (c)
        {
            case 'b':
                backlash_compensation_steps = atoi(optarg);
                break;
            case 'v':
                velocity = atoi(optarg);
                break;
            case 'r':
                ramp_steps = atoi(optarg);
                break;
            case 's':
                y_steps_per_scanline = atoi(optarg);
                break;
            case 'w':
                final_width = atoi(optarg);
                break;
        }
    }
    
    return optind;
}

int main(int argc, char **argv)
{
    int lastopt = do_parameters(argc, argv);

    if (argc == lastopt)
    {
        fprintf(stderr, "\nusage: %s [options] imagefilename\n", argv[0]);
        fprintf(stderr, "\n\t-b steps:\tBacklash compensation in steps\n");
        fprintf(stderr, "\t-r steps:\tRamp up/down distance in steps\n");
        fprintf(stderr, "\t-v steps:\tVelocity given as step time in 2MHz clocks\n");
        fprintf(stderr, "\t-s steps:\tDistance between scanlines in steps\n");
        fprintf(stderr, "\t-w steps:\tWidth of the scaled image in steps\n");
        fprintf(stderr, "\n");
        
        exit(1); 
    }

    printf("FreeImage version: %s\n", FreeImage_GetVersion());
    
    const char *filename = argv[lastopt];
    FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(filename, 0);
    FIBITMAP *src_image = FreeImage_Load(fmt, filename, 0);
    FIBITMAP *image = FreeImage_ConvertToGreyscale(src_image);
    FreeImage_Unload(src_image);

    
    if (image == 0)
    {
        fprintf(stderr, "Couldn't load %s.\n", filename);
        exit(1);
    }
    
    image_x = FreeImage_GetWidth(image);
    image_y = FreeImage_GetHeight(image);

    printf("Image dimensions: %dx%d\n",
        image_x, image_y);
    
	sp_return_t result = sp_get_port_by_name(serial_port, &port);

	if (result != SP_OK)
	{
		fprintf(stderr, "Couldn't open %s (1)\n", serial_port);
		exit(1);
	}

	result = sp_open(port, SP_MODE_READ | SP_MODE_WRITE);

	if (result != SP_OK)
	{
		fprintf(stderr, "Couldn't open %s (2)\n", serial_port);
		exit(2);
	}

    // Set up port parameters
    sp_port_config_t *conf;
    result = sp_new_config(&conf);
    result = result == SP_OK ? sp_set_config_baudrate(conf, 57600) : result;
    result = result == SP_OK ? sp_set_config_parity(conf, SP_PARITY_NONE) : result;
    result = result == SP_OK ? sp_set_config_bits(conf, 8) : result;
    result = result == SP_OK ? sp_set_config_stopbits(conf, 1) : result;
    result = result == SP_OK ? sp_set_config_flowcontrol(conf, SP_FLOWCONTROL_NONE) : result;
    result = result == SP_OK ? sp_set_config(port, conf) : result;

	if (result != SP_OK)
	{
		fprintf(stderr, "Couldn't configure port\n");
		exit(3);
	}

    // wait for silence
    uint8_t buf[32];
    while (1)
    {
        sp_return_t result = sp_blocking_read(port, buf, 1, 250);
        if (result == 0)
            break;
        printf("Received %c\n", buf[0]);
    }
    

    // Wait for handshake
    printf("# Attempting handshake...\n");

    // You have to wait an age for Arduinos to wake up
    usleep(2000000);

    if (handshake() == 0)
    {
        fprintf(stderr, "Didn't receive handshake.\n");
        exit(4);
    }

    printf("# Got handshake.\n");

    // Send image parameters
    // FIXME: swap image_x and pixels
    sprintf((char *)buf, "#P%d;", image_x);
    send_command((const char *)buf);
    sprintf((char *)buf, "#Y%d;", image_y);
    send_command((const char *)buf);
    sprintf((char *)buf, "#B%d;", backlash_compensation_steps);
    send_command((const char *)buf);
    sprintf((char *)buf, "#S%d;", y_steps_per_scanline);
    send_command((const char *)buf);
    sprintf((char *)buf, "#R%d;", ramp_steps);
    send_command((const char *)buf);
    sprintf((char *)buf, "#V%d;", velocity);
    send_command((const char *)buf);
    
    if (final_width == -1)
        sprintf((char *)buf, "#X%d;", image_x);        
    else
        sprintf((char *)buf, "#X%d;", final_width);
    send_command((const char *)buf);
        
    send_command("#!");

    // Send image data line by line
    int i;
    for (i = 0; i < image_y; i++)
    {
        int response = get_response(0);
        if (response == 0)
        {
            fprintf(stderr, "No response from device.\n");
            show_debug();
            exit(5);
        } 
        else if (response == 'N')
        {
            fprintf(stderr, "Device reported error response.\n");
            show_debug();
            exit(5);
        }
        else if (response != 'D')
        {
            fprintf(stderr, "Incorrect response (%c) from device.\n", response);
            show_debug();
            exit(5);
        }
        printf("Raster line %d\n", i);
        
        /* Begin sending line data */    
        uint8_t *data = FreeImage_GetScanLine(image, image_y - i - 1);
        int x;
        for (x = 0; x < image_x; x++)
        {
            uint8_t value = 255 - data[x];
            sp_nonblocking_write(port, &value, 1);
        }        
    
    }

    FreeImage_Unload(image);
	sp_close(port);
    sp_free_config(conf);
	sp_free_port(port);

	return 0;
}
