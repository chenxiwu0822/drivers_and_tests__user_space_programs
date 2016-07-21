#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUF_LEN	20

int main()

{
	int fd, num;
	char rs_ch[BUF_LEN];
	fd_set rfds, wfds;

	fd = open("/dev/tao", O_RDONLY | O_NONBLOCK);
	if (fd == -1) {
		return -1;
	}

	while (1) {
		FD_ZERO(&rfds);			
		FD_ZERO(&wfds);			
		FD_SET(fd, &rfds);			
		FD_SET(fd, &wfds);

		select(fd + 1, &rfds, &wfds, NULL, NULL);

		if (FD_ISSET(fd, &rfds))
			puts("poll: can be read\n");
		if (FD_ISSET(fd, &wfds))
			puts("poll: can be write\n");
		sleep(1);
	}

	return 0;
}

