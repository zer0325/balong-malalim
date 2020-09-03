#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "balong-malalim.h"
#include "patcher.h"

int locate_kernel(char *buf, uint32_t size);

int prepare_loader(FILE *loader)
{
	int i, res;
	int  koffset;
	int fbflag = 1, bflag = 1, cflag = 1;

	fseek(loader, 36, SEEK_SET);
	fread(&blk[0], 16, 1, loader);
	fread(&blk[1], 16, 1, loader);

	for(i = 0; i < 2; i++){
		blk[i].buf = (char *)malloc(blk[i].size);
		fseek(loader, blk[i].offset, SEEK_SET);
		fread(blk[i].buf, blk[i].size, 1, loader);
	}
	/*	
	printf("!-----------------------!\n");
	printf(" For debugging purposes\n");
	printf("!-----------------------!\n");
	for(i = 0; i < 2; i++){
		printf("lmode = [0x%08x] %d\n", blk[i].lmode, blk[i].lmode);
		printf("size  = [0x%08x] %d\n", blk[i].size, blk[i].size);
		printf("adr   = [0x%08x] %d\n", blk[i].addr, blk[i].addr);
		printf("offset= [0x%08x] %d\n", blk[i].offset, blk[i].offset);
		printf("buf   = [%p] \n", (void *)blk[i].buf);
		printf("\n");
	}
	*/
	/* Fastboot patch */
	if(fbflag){
		koffset = locate_kernel(blk[1].buf, blk[1].size);
		if(koffset == 0){
			printf("The bootloader does not have a fastboot component.");
			printf(" Fastboot loading is not possible.\n");
			return 0;
		} else{
			blk[1].buf[koffset] = 0x55;	/* fastboot signature patch */
			blk[1].size = koffset + 8;	/* trim the bootloader */
		}
	}

	/*	
	printf("!-----------------------!\n");
	printf(" For debugging purposes\n");
	printf("!-----------------------!\n");
	i = 1;
	printf("lmode = [0x%08x] %d\n", blk[i].lmode, blk[i].lmode);
	printf("size  = [0x%08x] %d\n", blk[i].size, blk[i].size);
	printf("adr   = [0x%08x] %d\n", blk[i].addr, blk[i].addr);
	printf("offset= [0x%08x] %d\n", blk[i].offset, blk[i].offset);
	printf("buf   = [%p] \n", (void *)blk[i].buf);
	printf("\n");
	*/	
	/* Patch erase procedure for ignoring bad blocks */	
	
	if(bflag){
		res = perasebad(blk[1].buf, blk[1].size);
		if(res == 0){
			printf("perasebad() function error.");
			printf(" loading not possible.\n");
			return 0;
		}
	}
	/* Perform patching procedure based on chipset */
	
	if(!cflag){
		res = pv7r1(blk[1].buf, blk[1].size);
		if(res == 0)
			res = pv7r2(blk[1].buf, blk[1].size);
		if(res == 0)
			res = pv7r11(blk[1].buf, blk[1].size);
		if(res == 0)
			res = pv7r22(blk[1].buf, blk[1].size);
		if(res == 0)
			res = pv7r22_2(blk[1].buf, blk[1].size);
		if(res == 0)
			res = pv7r22_3(blk[1].buf, blk[1].size);
		if(res != 0)
			printf("Patch applied on offset 0x%08x\n", blk[1].offset + res);
		else{
			printf("Patch signature not found. Use -c to boot without patching.\n");
			return 0;
		}
	}
	
	
	return 1;
}

int locate_kernel(char *buf, uint32_t size)
{
	int koff;

	for(koff = size - 8; koff > 0; koff--){
		if(strncmp(buf + koff, "ANDROID!", 8) == 0)
			return koff;
	}
	return 0;
}