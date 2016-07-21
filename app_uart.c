#include <stdio.h>
#include <fcntl.h>

int main(void)
{
	int fd;
	char buff1[1024] = "hello,uart";
	char buff2[1024];

	fd = open("/dev/ttySAC0", O_RDWR);
	if (fd = -1) {
		printf("open error\n");
		return -1;
	}

	write(fd, buff1, sizeof(buff1));

	read(fd, buff2, sizeof(buff2));

	printf("%s\n", buff2);
}


