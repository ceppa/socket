#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "common.h"
#include "analog.h"
#include "digital.h"
#include "digital_out.h"
#include "scenari.h"
#include <stdio.h>

struct system
{
	int device_num;
	char address[14];
	int port;
	unsigned char type;
	unsigned char in_num_bytes;
	unsigned char in_ch_an;
	unsigned char in_ch_d;
	unsigned char in_eos;
	unsigned char out_num_bytes;
	unsigned char out_ch_d;
	unsigned char out_ch_per_byte;
	unsigned char out_sos;
	unsigned char out_eos;
	bool enabled;
	char status;
	int sockfd;
};
struct system *systems;

int NUMSYSTEMS;
int SHMSYSTEMSID;

void doSystem(int k);
int operate(int sockfd,int deviceId, int timeout);
int doConnect(int systemId);
int systemNumToId(int systemNum,int totale);
void systemDisconnected(int systemId);
void sendResetSignal(int systemId);
void updateSystem(int device_num);
int loadSystemTable(bool onlyupdate);

#endif /* SYSTEMS_H */
