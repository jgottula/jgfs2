#include <err.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
	int fd;
	
	if ((fd = open(argv[1], O_RDWR)) == -1) {
		err(1, "open failed");
	}
	
	if (lseek(fd, 1024, SEEK_SET) == (off_t)-1) {
		err(1, "lseek failed");
	}
	
	char buffer[512];
	
	switch (write(fd, buffer, 512)) {
	case 512:
		break;
	case -1:
		close(fd);
		err(1, "write failed");
	default:
		close(fd);
		errx(1, "write incomplete");
	}
	
	close(fd);
	
	return 0;
}
