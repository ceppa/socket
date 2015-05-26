#ifndef CEIABI_H
#define CEIABI_H

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


#define CEIABIRecvLen 5120

struct CEIABIGateway
{
	int id_CEIABI_gateway;
	char address[16];
	int port;
	char description[50];
	bool enabled;
	char status;
	int sockfd;
	char failures;
	unsigned char recvBuf[CEIABIRecvLen];
	int recvBufReceived;
	int in_ch;
};
struct CEIABIGateway *CEIABIGateways;

int SHMCEIABIGATEWAYSID;
int NUMCEIABIGATEWAYS;

void doCEIABI(int l);
int doConnectCEIABI(int id);
int operateCEIABI(int sockfd,int id_CEIABI_gateway, int timeout);
void CEIABIDisconnected(int id_CEIABI_gateway);
int id_CEIABI_gatewayToId(int id_CEIABI_gateway);
int loadCEIABI(bool reload);
void updateCEIABI(int id_CEIABI_gateway);
void checkCEIABIReload(int id);
void CEIABIGatewayDisconnected(int id);
WORD CEIABI_crc16(BYTE *Buffer,WORD Len);
int putcrc(BYTE *buffer,WORD crc);
DWORD CEIABI_totime(unsigned char *buf);
int cleanCEIABILine(unsigned char *buf, int len);
int getCEIABILine(unsigned char *buf,unsigned char *bufout, int *len);

#endif /* CEIABI_H */
