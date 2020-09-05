#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "balong-malalim.h"
#include "patcher.h"
#include "parts.h"

int locate_kernel(char *buf, uint32_t size);

int prepare_loader(FILE *loader, uint8_t flags)
{
	int i, res;
	int  koffset;
	int fbflag = 1, bflag = 1, cflag = 1, mflag = 1;
	uint32_t ptoff;
	struct ptable_t *ptable;

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
			printf("\nThe bootloader does not have a fastboot component.");
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

	ptoff = find_ptable_ram(blk[1].buf, blk[1].size);
	ptable = (struct ptable_t *)(blk[1].buf + ptoff);

	/* test if -m option is enabled */
	if((flags & 0x04) >> 2){
		if(ptoff == 0){	/* partition table is not found */
			printf("\nPartition table not found.\n");
			exit(EXIT_FAILURE);
		}
		show_partition_table(*ptable);
		exit(EXIT_SUCCESS);
	}

	/* test if -b option is enabled then perform */
	/* patch erase procedure for ignoring bad blocks */	
	if((flags & 0x16) >> 4){
		res = perasebad(blk[1].buf, blk[1].size);
		if(res == 0){
			printf("\nperasebad() function error.");
			printf(" loading not possible.\n");
			return 0;
		}
	}
	/* test if -c option is enabled, if not */
	/* perform patching procedure based on chipset */
	if(!((flags & 0x32) >> 5)){
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
			printf("\nPatch applied on offset 0x%08x\n", blk[1].offset + res);
		else{
			printf("\nPatch signature not found. Use -c to boot without patching.\n");
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
