#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <libserialport.h>

const char *serial_port = "/dev/ttyUSB0";

typedef struct sp_port sp_port_t;
typedef struct sp_port_config sp_port_config_t;
typedef enum sp_return sp_return_t;

// Wait up to 200ms for handshake sequence ##
int handshake(sp_port_t *port)
{
    sp_return_t result;
    uint8_t buf[2];
    do
    {
        // Wait for # prefix
        result = sp_blocking_read(port, buf, 1, 200);
        if (result == 0)
        {
            return 0;
        }
    } while (buf[0] != '#');
    
    // Now read second character
    result = sp_blocking_read(port, buf, 1, 100);
    if (result == 0)
        return 0;
    
    if (buf[0] == '#')
        return 1;

    return 0;    
}

int main(int argc, char **argv)
{
	sp_port_t *port;
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
    uint8_t buf[1];
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

    buf[0] = '#';
    result = sp_nonblocking_write(port, buf, 1);
    if (result != 1)
        fprintf(stderr, "Error on write\n");
        
    result = sp_nonblocking_write(port, buf, 1);

    if (result != 1)
        fprintf(stderr, "Error on write\n");
    sp_drain(port);
    usleep(20000);


    if (handshake(port) == 0)
    {
        fprintf(stderr, "Didn't receive handshake.\n");
        exit(4);
    }

    printf("# Got handshake.\n");

    while (1)
    {
        char buf[16];
        sp_blocking_read(port, buf, 1, 0);
        printf("%c", buf[0]);
    }

	sp_close(port);
    sp_free_config(conf);
	sp_free_port(port);

	return 0;
}
