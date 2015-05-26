#ifndef SMS_H
#define SMS_H

#include <sys/time.h>
#include <time.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "systems.h"
#include "client.h"

struct msgform 
{
	long mtype;
	char mtext[MSGLENGTH];   
};


struct sms_device
{
	int id_sms_device;
	char address[16];
	int port;
	char physical_address[15];
	char description[50];
	bool enabled;
	char status;	
	int sockfd;
	int failures;
};
struct sms_device *sms_devices;

struct sms_number
{
	int id_sms_number;
	char name[50];
	char number[50];
	bool enabled;
};
struct sms_number *sms_numbers;

int SHMSMSDEVICESID;
int SHMSMSNUMBERSID;
int NUMSMSDEVICES;
int NUMSMSNUMBERS;
int messagesPid; //pid del sms sender
int loadSMS(bool reload);
void doMessageHandler();
void handleMessage(int adId,int id,int systemId,int channelId,
	char *msg, bool is_sms,int event_type);
void checkSMSReload(int id);
void sms_disconnect(int sms_id);
int sendsms(int sockfd,char *number,char *message);
int sendAndReceived(int sockfd,char *out,int numbytes,char *expected);
int initializeSMS(int sockfd);
int doConnectSMS(int id);

#endif /* SMS_H */
