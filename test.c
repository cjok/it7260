#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/input.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	struct input_event buffer;
	int fd;

	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		perror("open");
		exit(-1);
	}

	while (1) {
		read(fd, &buffer, sizeof(struct input_event));
		printf("type: %d, code: %d, value: %d\n", 
			buffer.type, buffer.code, buffer.value);
	}
	close(fd);
	exit(0);
}
