#ifndef COMMON_H
#define COMMON_H
 
typedef unsigned long DWORD;
typedef unsigned int WORD;
typedef unsigned char BYTE;

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
#define MSGSTOREKEY 76
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
#define CEIABI 7

#define ALARM_BACK_IN_RANGE -2
#define ALARM_NO_STORE -1
#define ALARM_AS_ALARM 0
#define ALARM_AS_EVENT 1

struct shmid_ds shmid_struct;

struct effemeridi
{
	int dawn;
	int sunset;
} effemeridiTable[12][31];



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
int TOTALSYSTEMS;
MYSQL *mysqlConnect();


 
#endif /* COMMON_H */
