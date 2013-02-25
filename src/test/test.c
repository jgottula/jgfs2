#include <err.h>


extern void test_mmap();
extern void test_tree();


int main(int argc, char **argv) {
	warnx("performing unit tests");
	
	//test_mmap();
	test_tree();
}
