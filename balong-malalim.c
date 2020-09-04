#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "balong-malalim.h"

int main(int argc, char *argv[])
{
	FILE *ldr;
	int res;
	char *loader_file;
	char *devname = "/dev/ttyUSB0";

	printf("\n     Balong Chipset USB Emergency USB Bootloader.\n");
	printf("** Full credits to forth32 for the original source **\n");

	if(argc < 2){
		printf("Usage: %s [loader file]\n", argv[0]);
		return -1;
	}

	loader_file = argv[1];

	/* Check if the loader file exists */
	ldr = fopen(loader_file, "rb");
	if(!ldr){
		printf("Error. File \"%s\" does not exist.\n", loader_file);
		return -1;
	}

	/* Check if a valid loader file */
	fread(&res, 4, 1, ldr);
	if(res != 0x20000){
		printf("Error. File \"%s\" is not a valid loader file\n", loader_file);
		return -1;
	}

	res = prepare_loader(ldr);
	if(!res){
		printf("\nError reported in prepare_loader() function.\n\n");
		return -1;
	}

	res = open_port(devname);
	if(res == -1){
		printf("\nError reported in open_port() function.\n\n");
		return -1;
	}

	
	res = send_loader(devfd);
	if(!res){
		printf("\nError reported in send_loader() function.\n\n");
		return -1;
	}

	fclose(ldr);

	printf("\nDownload Complete!!\n\n");
	return 0;
}
