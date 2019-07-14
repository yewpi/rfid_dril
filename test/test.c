#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<sys/ioctl.h>
 
#define IOCTL_READ _IOW(0xF8, 1, char*)
#define IOCTL_WRITE _IOR(0xF8, 2, char*)
struct string {
	char* b;
	unsigned len;
};
int main()
{
        int fd;
	char buffer[25];
	struct string s;

	s.b = buffer;
	s.len = 25;
	memset(buffer, 0, 25);

	buffer[0] = 'Z';
 
        fd = open("/dev/mfrc522", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }
	
        ioctl(fd, IOCTL_WRITE, &s); 
	buffer[0] = 'A';
        ioctl(fd, IOCTL_READ, buffer); 
	printf("received %c, %x\n", *buffer, *buffer & 0xff);
 
 
        close(fd);
}
