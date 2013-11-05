#include <stdio.h>
#include <string.h>
#include "smooth_lib.h"

void smooth_lib_present_buf(char * buf, int len)
{
	int i;

	for (i = 0; i < len; ++i)
	{
		printf("%02x%s", buf[i], i % 20 == 19 ? "\n" : " ");
	}
	printf("\n");
}

int _main()
{
	char * nice = "helloword";

	smooth_lib_present_buf(nice, strlen(nice) + 1);

	return 0;
}
