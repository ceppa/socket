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
#include "analog.h"
#include "digital.h"
#include "digital_out.h"
#include "knx.h"
#include "scenari.h"
#include "irrigazione.h"
#include "multimeter.h"
#include "sms.h"
#include "systems.h"
#include "server.h"
#include "controller.h"
#include "panels.h"
#include "ai.h"


//SOCKET
int *pid;	//connessioni alle schede
int mypid;

void readSystemDefaults(MYSQL *connection);
MYSQL *mysqlConnect();


void killAllSystems();
void loadAllSystems();
void startAllSystems();
void freeshm();
int checkSystems(int *pidArray);
int operate(int sockfd,int deviceId, int timeout);
void killSystems(int *pidArray);
void killPid(int pid);
void formatMessage(char *buffer,int adId,int id,int systemId,int channelId,char *msg);
void storeAlarm(int adId,int id,int systemId,int channelId,char *msg);
void storeEvent(int event_type,int device_num,int ch_num,int ch_id,char *msg);
void storeTables();
void scriviEffemeridiSuFile(int fd);
void die(char *text);
unsigned char parse_int(char *str);
void get_sottostato(int fd,char *buf);
