#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "parts.h"
#include "patcher.h"


void print_usage(char *program_name);
int prepare_loader(FILE *ldr);
int open_port(char *devname);
int send_loader();


struct{
		int lmode;
		int size;
		int adr;
		int offset;
		char *pbuf;
} blk[10];

int devfd;
struct termios config;
uint8_t flags = 0;


int main(int argc, char *argv[])
{
	FILE *ldr;
	int res, i, c;
	char devname[16], progname[16];

	printf("\n\n    Balong Chipset Emergency USB Bootloader.\n");
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
				printf("\n%s: Error. Invalid option. \"-%c\". ", progname, c);
				printf("Try -h for help.\n\n");
				return 1;
		}
	}

	if(flags & 0x01){
		print_usage(progname);
		return 1;
	}
	if(argc > 1){
		printf("\n%s: Too many parameters. ", progname);
		printf("Try -h for help.\n\n");
		return 1;
	}else if(argc == 0){
		printf("\n%s: Too few parameters. ", progname);
		printf("Try -h for help.\n\n");
		return 1;
	}

	/* check if the loader file exists */
	ldr = fopen(*argv, "rb");
	if(ldr == NULL){
		printf("[Error %d]. File \"%s\" does not exist.\n", errno, *argv);
		return 1;
	}

	/* check if a valid usbloader file */
	fread(&i, 1, 4, ldr);
	if(i != 0x20000){
		printf("\nFile \"%s\" is not a valid usbloader file.\n\n", *argv);
		return 1;
	}

	res = prepare_loader(ldr);
	if(!res)
		return 1;

	/* Set /dev/ttyUSB0 as the default device */
	if(!((flags & 0x02) >> 1))
		strcpy(devname, "/dev/ttyUSB0");

	res = open_port(devname);
	if(!res)
		return 1;

	/* check if the device is in USB boot mode */
	c = 0;
	write(devfd, "A", 1);
	res = read(devfd, &c, 1);
	if(c != 0x55){
		printf("Port is not in USB Boot mode!\n");
		return 1;
	}

	res = send_loader();
	if(!res)
		return 1;

	/* if -f or -b option is set */
	if(((flags & 0x08) >> 3) || ((flags * 0x16) >> 4))
		printf("\nDevice is now in Fastboot mode.\n");

	printf("\nDownload Finished!\n");

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

int locate_kernel(char *buf, uint32_t size);

/* prepare_loader: save ldr into memory */
int prepare_loader(FILE *ldr)
{
	int i, res, koff;
	uint32_t ptoff;
	struct ptable_t *ptable;

	/* Advance the file position pointer 36-bytes from the start of the file */
	fseek(ldr, 36, SEEK_SET);
	/* Save info for the raminit block */
	fread(&blk[0], 1, 16, ldr);
	/* Save info for the usbloader block */
	fread(&blk[1], 1, 16, ldr);

	/* save each block into memory */
	for(i = 0; i < 2; i++){
		/* allocate memory */
		blk[i].pbuf = (char *)malloc(blk[i].size);
		/* save into memory */
		fseek(ldr, blk[i].offset, SEEK_SET);
		res = fread(blk[i].pbuf, 1, blk[i].size, ldr);
		if(res != blk[i].size){
			printf("\nUnexpected end of file: read %d, expected %d\n\n", res, blk[i].size);
			return 0;
		}
	}
	
	/* Fastboot patch. If -f is enabled */
	if((flags & 0x08) >> 3){
		koff = locate_kernel(blk[1].pbuf, blk[1].size);
		if(koff != 0){
			blk[1].pbuf[koff] = 0x55; 	/* signature patch */
			blk[1].size = koff + 8;		/* trim the partition */		
		} else{
			printf("\nThe bootloader does not have an ANDROID component. ");
			printf("Fastboot loading is not possible.\n\n");
			exit(0);
		}
	}
	ptoff = find_ptable_ram(blk[1].pbuf, blk[1].size);
	ptable = (struct ptable_t *)(blk[1].pbuf + ptoff);

	/* Test if -m option is enabled (show partition table map) */
	if((flags & 0x04) >> 2){
		if(ptoff == 0){	/* partition table is not found */
			printf("\nPartition table not found.\n\n");
			exit(EXIT_FAILURE);
		}
		show_partition_table(*ptable);
		exit(EXIT_SUCCESS);
	}

	/* test if -b option is enabled then perform */
	/* patch erase procedure for ignoring bad blocks */	
	if((flags & 0x16) >> 4){
		res = perasebad(blk[1].pbuf, blk[1].size);
		if(res == 0){
			printf("\nperasebad() function error.");
			printf(" loading not possible.\n\n");
			return 0;
		}
	}
	/* test if -c option is enabled, if not */
	/* perform patching procedure based on chipset */
	if(!((flags & 0x32) >> 5)){
		res = pv7r1(blk[1].pbuf, blk[1].size);
		if(res == 0)
			res = pv7r2(blk[1].pbuf, blk[1].size);
		if(res == 0)
			res = pv7r11(blk[1].pbuf, blk[1].size);
		if(res == 0)
			res = pv7r22(blk[1].pbuf, blk[1].size);
		if(res == 0)
			res = pv7r22_2(blk[1].pbuf, blk[1].size);
		if(res == 0)
			res = pv7r22_3(blk[1].pbuf, blk[1].size);
		if(res != 0)
			printf("\nPatch to be applied on offset 0x%08x....\n", blk[1].offset + res);
		else{
			printf("\nPatch signature not found. Use -c to boot without patching.\n\n");
			return 0;
		}
	}

	fclose(ldr);
	return 1;
}

int locate_kernel(char *buf, uint32_t size)
{
	int off;

	for(off = (size - 8); off > 8; off--){
		if(strncmp(buf + off, "ANDROID!", 8) == 0)
			return off;
	}
	return 0;
}


int open_port(char *devname)
{
	devfd = open(devname, O_RDWR | O_NOCTTY | O_SYNC);
	if(devfd == -1){
		printf("\nCannot open port \"%s\".\n\n", devname);
		return 0;
	}
	/* prepare the port configuration */
	bzero(&config, sizeof(config));
	config.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	config.c_iflag = 0;
	config.c_oflag = 0;
	config.c_lflag = 0;
	config.c_cc[VTIME] = 30;
	config.c_cc[VMIN] = 0;
	/* set configuration */
	tcsetattr(devfd, TCSANOW, &config);

	return 1;
}

int sendcmd(unsigned char *buff, unsigned int len);

int send_loader()
{
	unsigned i, datasize, pktcount, res, adr;
	unsigned char cmdhead[14] = {0xfe, 0, 0xff};
	unsigned char cmddata[1040] = {0xda, 0, 0};
	unsigned char cmdeod[5] = {0xed, 0, 0, 0, 0};

	printf("\n\n%10s%2c", "component", 0x20);
	printf("%10s%2c", "address", 0x20);
	printf("%10s%2c", "size", 0x20);
	printf("%10s\n", "%download");
	for(i = 0; i < 50; i++)
		putchar('-');
	printf("\n");

	for(i = 0; i < 2; i++){
		datasize = 1024;
		pktcount = 1;

		/* Prepare the start frame packet */
		*((unsigned int*)&cmdhead[4]) = htonl(blk[i].size);
		*((unsigned int*)&cmdhead[8]) = htonl(blk[i].adr);
		cmdhead[3] = blk[i].lmode;

		/* send the start frame packet */
		res = sendcmd(cmdhead, 14);
		if(!res){
			printf("\nModem rejected header packet.\n\n");
			return 0;
		}
		for(adr = 0; adr < blk[i].size; adr += 1024){
			if((adr + 1024) >= blk[i].size)
				datasize = blk[i].size - adr;
			
			printf("\r%2c%-10s%2c", 0x20, i ? "usbboot" : "raminit", 0x20);
			printf("%08x%4c", blk[i].adr, 0x20);
			printf("%8d%4c", blk[i].size, 0x20);
			printf("%d%%", (adr + datasize) * 100 / blk[i].size);

			/* prepare the data packet */
			cmddata[1] = pktcount;
			cmddata[2] = (~pktcount) & 0xff;
			memcpy(cmddata + 3, blk[i].pbuf + adr, datasize);
			pktcount++;
			if(!sendcmd(cmddata, datasize + 5)){
				printf("\nModem reject data packet\n\n");
				return 0;
			}
		}
		free(blk[i].pbuf);

		/* send the end packet */
		cmdeod[1] = pktcount;
		cmdeod[2] = ~(pktcount) & 0xff;
		if(!sendcmd(cmdeod, 5)){
				printf("\nModem rejected end of data packet.\n\n");
				return 0;
		}
		printf("\n");
	}
	return 1;
}

void csum(unsigned char *buf, int len);

int sendcmd(unsigned char *buf, unsigned int len)
{
	unsigned char replybuf[1024];
	unsigned int replylen;

	csum(buf, len);
	write(devfd, buf, len);	/* sending the command */
	tcdrain(devfd);
	replylen = read(devfd, replybuf, 1024);
	if(replylen == 0)
		return 0;
	if(replybuf[0] == 0xaa)
		return 1;
	return 0;
}

void csum(unsigned char *buf, int len)
{
	unsigned int i, c, csum = 0;
	unsigned int cconst[] = {
		0x0000, 0x1021, 0x2042, 0x3063, 
		0x4084, 0x50A5, 0x60C6, 0x70E7, 
		0x8108, 0x9129, 0xA14A, 0xB16B, 
		0xC18C, 0xD1AD, 0xE1CE, 0xF1EF
	};

	for(i = 0; i < (len - 2); i++){
		c = buf[i] & 0xff;
		csum = ((csum << 4) & 0xffff) ^ cconst[(c >> 4 ) ^ (csum >> 12)];
		csum = ((csum << 4) & 0xffff) ^ cconst[(c & 0xf) ^ (csum >> 12)];
	}
	buf[len - 2] = (csum >> 8) & 0xff;
	buf[len - 1] = csum & 0xff;
}
