#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "balong-malalim.h"

int send_packet(unsigned char *buf, int len, int devfd);

int send_loader(int devfd)
{
	int packet_count;
	int offset;
	int datasize = 1024;
	int i;

	/* prepare the packets */
	unsigned char header[14] = {0xfe, 0, 0xff};
	unsigned char data[1040] = {0xda, 0, 0};
	unsigned char tail[5] = {0xed, 0, 0, 0, 0};

	printf("\n\n%-9s%-15s%8s", "Blk No.", "Component", "Address");
	printf("%6c%8s%4c%%%s\n\n", 0x20, "Size", 0x20, "Download");

	for(i = 0; i < 2; i++){
		packet_count = 1;

		header[3] = blk[i].lmode;
		*((unsigned int *)&header[4]) = htonl(blk[i].size);
		*((unsigned int *)&header[4]) = htonl(blk[i].addr);
		if(!send_packet(header, 14, devfd)){
			printf("\nModem rejected data packet.\n");
			return 0;
		}
		

		for(offset = 0; offset + datasize < blk[i].size; offset += 1024){
			printf("\r [%d] %5c%-15s", blk[i].lmode, 0x20, i ? "usbboot" : "raminit");
			printf("0x%08x%5c%8d%5c", blk[i].addr, 0x20,  blk[i].size, 0x20);
			printf("%i%%", (offset + datasize)*100 / blk[i].size);

			data[1] = packet_count;
			data[2] = ~packet_count & 0xff;
			memcpy((void *)(data + 3), (void *)(blk[i].buf + offset), datasize);
			if(!send_packet(data, datasize + 5, devfd)){
				printf("\nModem rejected data packet.\n");
				return 0;
			}
			
			packet_count++;
		}
	}
	printf("\r [%d] %5c%-15s", blk[i].lmode, 0x20, i ? "usbboot" : "raminit");
	printf("0x%08x%5c%8d%5c", blk[i].addr, 0x20,  blk[i].size, 0x20);
	printf("%i%%", 100);
	printf("\n");

	/* Do the remaining packets */
	data[1] = packet_count;
	data[2] = ~packet_count & 0xff;
	memcpy((void *)(data + 3), (void *)(blk[i].buf + offset), blk[i].size - offset);
	if(!send_packet(data, blk[i].size - offset + 5, devfd)){
		printf("\nModem rejected data packet.\n");
		return 0;
	}
	packet_count++;

	/* Do the tail */
	tail[1] = packet_count;
	tail[2] = ~packet_count & 0xff;
	if(!send_packet(tail, 5, devfd)){
		printf("\nModem rejected data packet.\n");
		return 0;
	}
	
	return 1;
}

void checksum(unsigned char *buf, int len);

int send_packet(unsigned char *buf, int len, int devfd)
{
	unsigned char reply_buf[1024];
	unsigned int res = 0;

	checksum(buf, len);
	
	write(devfd, buf, len);
	tcdrain(devfd);
	res = read(devfd, reply_buf, 1024);
	/*
	printf("debug: res = %d, reply_buf[0] = 0x%02x\n", res, reply_buf[0]);
	*/
	if(res == 0 || reply_buf[0] != 0xAA)
		return 0;

	return 1;
}

void checksum(unsigned char *buf, int len)
{
	int i, csum = 0;
	unsigned int c;
	unsigned int cconst[] = {
		0x0000, 0x1021, 0x2042, 0x3063,
		0x4084, 0x50A5, 0x60C6, 0x70E7,
		0x8108, 0x9129, 0xA14A, 0xB16B,
		0xC18C, 0xD1AD, 0xE1CE, 0xF1EF
	};

	for(i = 0; i < (len - 2); i++){
		c = buf[i] & 0xff;
		csum = ((csum >> 4) & 0xffff) ^ cconst[(c >> 4) ^ (csum >> 12)];
		csum = ((csum >> 4) & 0xffff) ^ cconst[(c & 0xf) ^ (csum >> 12)];
	}
	buf[len-1] = csum & 0xff;
	buf[len-2] = (csum >> 8) & 0xff;
}

	
