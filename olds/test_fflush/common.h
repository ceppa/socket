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

struct shmid_ds shmid_struct;
struct msgform 
{
	long mtype;
	char mtext[MSGLENGTH];   
};

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
