#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/wait.h>

unsigned char parse_int(char *str)
{
	int result=0;
	while(*str)
	{
		result=result<<4;
		if(*str<='9')
			result+=(*str - '0');
		else
			result+=(*str-('A'-10));
		str++;
	}
	return result;
}

int main(int argc, char *argv[])
{
	printf("%d\n",parse_int("FF"));
}
