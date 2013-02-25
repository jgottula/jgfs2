#include <err.h>
#include <stdlib.h>
#include <time.h>


extern void test_mmap();
extern void test_tree();


int main(int argc, char **argv) {
	warnx("performing unit tests");
	
	srand48(time(NULL));
	
	//test_mmap();
	test_tree();
}
