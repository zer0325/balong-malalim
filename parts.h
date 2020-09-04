#include <stdint.h>

struct ptable_line {
	char name[16];
	unsigned start;
	unsigned lsize;
	unsigned length;
	unsigned loadaddr;
	unsigned entry;
	unsigned type;
	unsigned nproperty;
	unsigned count;
};

struct ptable_t {
	uint8_t head[16];
	uint8_t version[16];
	uint8_t product[16];
	struct ptable_line part[41];
	uint8_t tail[32];
};

void show_partition_table(struct ptable_t ptable);
uint32_t find_ptable_ram(char *buf, uint32_t size);

