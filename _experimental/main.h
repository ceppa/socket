
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
#include <mysql/mysql.h>

#include "common.h"
#include "ceiabi.h"

int socketConnect(char *address,int port);
void dumpBuffer(unsigned char *buf,int len);
void *get_shared_memory_segment(const int size, int *shmid,
		const char *filename);
MYSQL *mysqlConnect();
void killAllSystems();
