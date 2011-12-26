#include <stdio.h>

int main(void)
{
	int i = 0;
	printf("%s   %d\n", __func__,!!i);
	return 0;
}
