#include <stdio.h>
#include <stdlib.h>

#define NUM 100
#define SIZE (NUM * sizeof(int))



void *malloc_fast(int size) {
	return malloc(size);
}
void *malloc_nvm(int size) {
	return malloc(size);
}
int main()
{
	int a = 0, b = 0;
	int *c, *d;
	int sum = 0;
	a = 5;
	b = 3;
	c = (int *)malloc(10 * sizeof(int));
	d = (int *)malloc(10 * sizeof(int));
	sum = a + b;
	*c = *c + 1;
	*d = *d + 1;
	for (int i = 0; i < 10; i++) {
		c[i] = 66;
	}
	
	if (__builtin_expect(a > 5, 0)) {
		sum = sum - 1;
	}
	else if (__builtin_expect(b <= 3, 1)) {
		sum = sum - 2;
	}
	else {
		sum = sum + 1;
	}

}