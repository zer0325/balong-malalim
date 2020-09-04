#include <stdio.h>
#include <string.h>
#include "parts.h"


void show_partition_table(struct ptable_t ptable)
{
	int pnum = 0, i;

	printf("\n Partition table version: %16.16s", ptable.version);
	printf("\n firmware version       : %16.16s\n", ptable.product);
	printf("\n");

	printf(" No. %16s ", "Partition Name");
	printf(" %5s ", "start");
	printf(" %3s ", "len");
	printf(" %8s ", "loadaddr");
	printf(" %8s ", "loadsize");
	printf("   %5s  ", "entry");
	printf("   %5s  ", "flags");
	printf("   %4s   ", "type");
	printf("   %5s  ", "count");

	printf("\n");
	for(i = 0; i < 94; i++)
		putchar('-');
	printf("\n");

	for(i = 0; ptable.part[i].name != 0 && (strcmp(ptable.part[i].name, "T")
				!= 0); i++){
		
		printf(" %02i ", pnum);
		printf(" %-16.16s ", ptable.part[i].name);
		printf(" %4x ", ptable.part[i].start / 0x20000);
		printf(" %4x ", ptable.part[i].length / 0x20000);
		printf(" %08x ", ptable.part[i].lsize);
		printf(" %08x ", ptable.part[i].loadaddr);
		printf(" %08x ", ptable.part[i].entry);
		printf(" %08x ", ptable.part[i].nproperty);
		printf(" %08x ", ptable.part[i].type);
		printf(" %08x ", ptable.part[i].count);
		pnum++;
		printf("\n");
	}
	printf("\n");
}

const char headmagic[16] = {
	0x70, 0x54, 0x61, 0x62, 0x6C, 0x65, 0x48, 0x65,
	0x61, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80
};
						

uint32_t find_ptable_ram(char *buf, uint32_t size)
{
	uint32_t poff;

	for(poff = 0; poff < size - 16; poff += 4){
		if(memcmp(buf + poff, headmagic, 16) == 0)
			return poff;
	}
	return 0;
}

