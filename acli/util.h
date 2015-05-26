#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>
#include <errno.h>
#include "common.h"


char *trim(char *in);
double intToDouble(unsigned int in);
unsigned int doubleToInt(double in);
int receive(int sockfd,char *buf,int len,char *stringok,int timeout);
int socketConnect(char *address,int port);
char *strtoupper(char *st);
int h2i(char *h);
void i2h(char *h,int i);
double getCurveValue(char* values,double value);
void safecpy(char *dest,char *src);
int getLogFileName(char *dest,char *path);
int getTimeString(char *format,char *dest);
int getTime();
void *get_shared_memory_segment(const int size, int *shmid, const char *filename);
unsigned char parse_int(char *str);
void my_printf(const char *format, ...);
void my_perror(const char *s);
void dumpBuffer(unsigned char *buf,int len);
int my_round(double number);
