#include <fcntl.h>
#include <termios.h>
#include "balong-malalim.h" 

int open_port(char *devname)
{
	int res;

	devfd = open(devname, O_RDWR | O_NOCTTY | O_SYNC);
	/*
	printf("debug: devfd = %d\n", devfd);
	*/
	if(devfd == -1){
		fprintf(stderr, "Error: Device \"%s\" not found\n", devname);
		return devfd;
	}
	
	/* get the current options */
	res = tcgetattr(devfd, &options);
	if(res == -1)
		return -1;
	
	bzero(&options, sizeof(options));	/* initialize the optios to zero */

	/* set the options */
	options.c_cflag |= B115200 | CS8 | CLOCAL | CREAD;
	options.c_lflag &= 0;
	options.c_iflag &= 0;
	options.c_oflag &= 0;
	options.c_cc[VTIME] = 30;
	options.c_cc[VMIN] = 0;
	/* save the new options */
	res = tcsetattr(devfd, TCSANOW, &options);
	if(res == -1)
		return -1;
	
	
	return devfd;
}


