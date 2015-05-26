#ifndef KNX_H
#define KNX_H

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


#define xtod(c) ((c>='0' && c<='9') ? c-'0' : ((c>='A' && c<='F') ? \
                c-'A'+10 : ((c>='a' && c<='f') ? c-'a'+10 : 0)))
#define knxRecvLen 512

struct knxLine
{
	int id_knx_line;
	int id_knx_gateway;
	bool input_output;
	char group_address[10];
	char physical_address[10];
	char data_type;
	char description[128];
	char hi_low_msg[128];
	char low_hi_msg[128];
	int record_data_time;
	time_t stored_time;
	bool is_alarm;
	bool sms;
	bool alarm_transition;	//(se 1 L -> H, se 0 H -> L)

	bool enabled;
	int req_value;	//value set or velue update
	int value;
	int value_valid;
	double value_eng;
	double value_max;
	time_t ts_max;
	double value_min;
	time_t ts_min;
};
struct knxLine *knxTable;

struct knxGateway
{
	int id_knx_gateway;
	char address[16];
	int port;
	char physical_address[15];
	int type;
	char description[50];
	bool enabled;
	char status;
	int sockfd;
	int ch_in;
	int ch_out;
	char failures;
	unsigned char recvBuf[knxRecvLen];
	int recvBufReceived;
};
struct knxGateway *knxGateways;

int SHMKNXGATEWAYSID;
int SHMKNXTABLEID;
int NUMKNXGATEWAYS;
int NUMKNXCHANNELS;

void doKNX(int l);
int doConnectKnx(int knxGatewayId);
int operateKnx(int knxId, int timeout);
void knxGatewayDisconnected(int knxGatewayId);
int id_knx_gatewayToId(int id_knx_gateway);
int loadKnx(bool reload);
int setKnx(int id,double value);
void resetKnxValue(int i);
int group_addressToId(char *group_address);
int id_knx_lineToId(int id_knx_line);
int parseKnxOutput(char *string,char *source,char *dest,unsigned int *data);
unsigned char knxValToInt(char *in);
int HextoDec(char *hex);
void updateKnx(int id_knx_gateway);
void checkKnxReload(int id);
int getResponseLine(unsigned char *buf, int len);
void fix_group_address(char *group_address);
void handleKNXAutomaticUpdate();
void storeKNXTable(int id_knx_gateway);
int storeKNXLine(int i,MYSQL *conn);
void KNXHandleMaxMin(int i,double value);
int updateKnxLine(int id);

#endif /* KNX_H */
