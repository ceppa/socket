#ifndef SMS_H
#define SMS_H

#include <sys/time.h>
#include <time.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "systems.h"
#include "common.h"

struct sms_device
{
	int id_sms_device;
	char address[16];
	int port;
	char physical_address[15];
	char description[50];
	bool enabled;	
	int sockfd;
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

#endif /* SMS_H */
