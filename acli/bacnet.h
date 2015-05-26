#ifndef BACNET_H
#define BACNET_H

#include <sys/time.h>
#include <time.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "systems.h"
#include <sys/socket.h>
#include <sys/types.h>
#include "common.h"
#include "sms.h"

#define BACNET_IP         0x81
#define UNICAST           0x0A
#define BACNET_VERSION    0x01
#define READ_PROPERTY     0x0C
#define COMPLEX_ACK       0x30
#define PROPERTY_TAG      0x19
#define PRESENT_VALUE     0x55
#define ENUMERATION       0x91
#define BACNET_ERROR      0x50
#define BINARY_INPUT_REQ      0x03
#define ANALOG_INPUT_REQ      0x00
#define BINARY_INPUT_ANS      0x91
#define ANALOG_INPUT_ANS      0x44

#define REQUEST_LENGTH      17
#define BACNET_TIMEOUT		500000
#define BACNET_MAXFAILURES	5

int receiveBacnetResponse( int socket,unsigned long *identifier,unsigned long *value);
bool waitForResponse( int socket, int timeout);
bool sendBacnetRequest( int socket,unsigned long object,char *ip,int id );
float formatBacnetValue(int object_type,unsigned long value);

struct bacnetLine
{
	int id;
	int id_bacnet_device;
	int object_type;
	unsigned long object_instance;
	char description[80];
	char hi_low_msg[80];
	char low_hi_msg[80];
	bool is_alarm;
	char alarm_msg[80];
	unsigned long value;
	bool value_valid;
};
struct bacnetLine *bacnetTable;

struct bacnetDevice
{
	int id;
	char address[16];
	int port;
	char description[80];
	bool enabled;
	char status;
	int sockfd;
	int failures;
	bool ok;
};
struct bacnetDevice *bacnetDevices;

int SHMBACNETDEVICESID;
int SHMBACNETTABLEID;
int NUMBACNETDEVICES;
int NUMBACNETLINES;

void doBacnet(int l);
int doConnectBacnet(int id);
int operateBacnet(int id_device);
void bacnetDeviceDisconnected(int id);
int id_bacnet_deviceToId(int id_bacnet_device);
int loadBacnet(bool reload);
int id_bacnet_lineToId(int id_bacnet_line);
int parseBacnetOutput(char *string,char *source,char *dest,unsigned int *data);
void resetBacnetValue(int i);
int bacnetIdentifierToId(int id_bacnet_device,unsigned long identifier);
int bacnetDeviceToId(int id_bacnet_device);
void updateBacnet(int id_bacnet_device);
void checkBacnetReload(int id);

#endif /* KNX_H */
