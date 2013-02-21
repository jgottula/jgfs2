#include <err.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
	int fd;
	
	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		err(1, "open failed");
	}
	
	if (syncfs(fd) == -1) {
		err(1, "syncfs failed");
	}
	
	close(fd);
	
	return 0;
}
