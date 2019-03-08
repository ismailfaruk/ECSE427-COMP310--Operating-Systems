#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	char *s = "hello world";
	#ifdef SEG
		*s = 'H';
	#else
		printf("The line is: %s\n", s);
		fflush(stdout);
	#endif
}
