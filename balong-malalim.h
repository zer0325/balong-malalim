#include <termios.h>
#include <stdio.h>

int prepare_loader(FILE *loader);
int send_loader(int devfd);
int open_port(char *devname);

struct{
	int lmode;
	int size;
	int addr;
	int offset;
	char *buf;
} blk[2];

int devfd;
struct termios options;
