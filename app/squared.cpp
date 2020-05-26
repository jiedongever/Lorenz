#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

int main(int argc, char*argv[])
{
	int n;
	int time;

	sscanf(argv[1], "%d", &n);
	sscanf(argv[2], "%d", &time);

	Sleep(time);
	printf("%d", n*n);

	return EXIT_SUCCESS;
}