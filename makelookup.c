#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char **argv)
{
    int running = 1;
    int steps, total_steps;
	int64_t c, d, n, t1, t2;
	FILE *outfile;

    if (argc < 3) {
        printf("usage: %s steps acceleration\n", argv[0]);
        exit(1);
    }

    total_steps = atoi(argv[1]);
    c = atoi(argv[2]);
    t1 = 0;
    t2 = 0;
    n = 0;
 
    outfile = fopen("lookup.bin", "w");
    for (steps = 0; steps < total_steps; steps++)
    {
        uint8_t temp;
        n += c; 
        t2 = t1;
        t1 = sqrt(n); 
        d = t1 - t2; 
#if 0
        printf("%d.\tn=%" PRIu64 " t1=%" PRIu64" t2=%" PRIu64 " d=%" PRIu64 "\n", 
                steps, n, t1, t2, d);
#endif
        temp = d & 0xff;
        fwrite(&temp, 1, 1, outfile);
        temp = (d >> 8) & 0xff;
        fwrite(&temp, 1, 1, outfile);
    }
    fclose(outfile);
    printf("Generated acceleration to rate %" PRIu64 
        " (%.1f steps per second) in %d steps.\n",
        d, 2000000.0 / d, steps);
    return 0;
}
