#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "balong-malalim.h"

void print_usage(char *program_name);

int main(int argc, char *argv[])
{
	FILE *ldr;
	int res;
	char devname[16], progname[16];
	uint8_t flags;
	int c;

	printf("    Balong Chipset Emergency USB Bootloader.\n");
	printf("** Full credits to forth32 for the original source. **\n");

	strcpy(progname, argv[0]);
	while(--argc > 0 && *(++argv)[0] == '-'){
		c = *++argv[0];
		switch(c){
			case 'h':
				flags += 1;
				break;
			case 'p':
				flags += 2;
				strcpy(devname, *++argv);
				--argc;
				break;
			case 'm':
				flags += 4;
				break;
			case 'f':
				flags += 8;
				break;
			case 'b':
				flags += 16;
				break;
			case 'c':
				flags += 32;
				break;
			default:
				printf("\n%s: Error. Invalid option. - \"-%c\".\n\n", progname, c);
				return -1;
		}
	}

	if(flags & 0x01){
		print_usage(progname);
		return -1;
	}
	
	if(argc > 1){
		printf("\n%s: Too many parameters. ", progname);
		printf("Try -h for help.\n\n");
		return -1;
	}else if(argc == 0){
		printf("\n%s: Too few parameters. ", progname);
		printf("Try -h for help.\n\n");
		return -1;
	}

	/* Check if the loader file exists */
	ldr = fopen(*argv, "rb");
	if(!ldr){
		printf("\nError. File \"%s\" does not exist.\n\n", *argv);
		return -1;
	}

	/* Check if a valid loader file */
	fread(&res, 4, 1, ldr);
	if(res != 0x20000){
		printf("\nError. File \"%s\" is not a valid loader file\n\n", *argv);
		return -1;
	}
	
	if(!prepare_loader(ldr, flags))
		return -1;

	/* Set the /dev/ttyUSB0 as the default device */
	if(!((flags & 0x02) >> 1))
		strcpy(devname, "/dev/ttyUSB0");

	res = open_port(devname);
	if(res == -1) 
		return -1;

	
	if(!send_loader(devfd))
		return -1;
	
	fclose(ldr);
	
	/* if -f or -b option is set */
	if(((flags & 0x08) >> 3) || ((flags & 0x016) >> 4))
		printf("\nDevice is now in Fastboot mode.\n");

	printf("\nDownload Complete!!\n\n");
	return 0;
}

void print_usage(char *progname)
{
	printf("\n\nUsage: %s [options] [arguments] <loader file>\n\n", progname);
	printf("Options:\n\n");
	printf(" %2s%15s%5c%s", "-p", "/dev/ttyUSBx", 0x20, "Serial port for communicating with the  bootloader (default is /dev/ttyUSB0).\n");
	printf("%23cwhere x is an integer.\n", 0x20);
	printf(" %2s%20c%s", "-f", 0x20, "Put the device in fastboot mode.\n");
	printf(" %2s%20c%s", "-b", 0x20, "Similar to -f, additionally performs patch for bad block signatures.\n");
	printf(" %2s%20c%s", "-m", 0x20, "Show the bootloader partition table and exit.\n");
	printf(" %2s%20c%s", "-c", 0x20, "Disable patch erase procedure.\n\n");
	
}
