#ifndef COMMON_H
#define COMMON_H
 
#include <mysql/mysql.h>
#include <unistd.h>
#include <sys/shm.h>
typedef unsigned char bool;

#define BACKLOG 10 
#define MYPORT 3480
#define MAXDATASIZE 5120
#define MAXCONNECTIONS 20
#define TERMINATOR "FiNe"
#define MSGKEY 75
#define MSGLENGTH 100

#define TRUE  1
#define FALSE 0


#define ANALOG 0
#define DIGITAL 1
#define DIGITAL_OUT 2
#define MULTIMETER 3
#define KNX_IN 4
#define KNX_OUT 5
#define BACNET 6


struct shmid_ds shmid_struct;

struct effemeridi
{
	int dawn;
	int sunset;
} effemeridiTable[12][31];


int msgid;

char logPath[255];
char logFileName[80];
int logFileFd;

bool *reloadCommand;
int SHMRELOADCOMMAND;


int ***deviceToid; //type,num,ch
int MAXATTEMPTS;
int OFFSET_EFFE;
char LOCALITA[30];
char NOME[30];
int RECORDDATATIME;
int RNDSEED;
MYSQL *connection, mysql;

 
#endif /* COMMON_H */
