#ifndef DIGITAL_OUT_H
#define DIGITAL_OUT_H

#include <sys/time.h>
#include <time.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "systems.h"
#include "common.h"

struct digitalOutLine
{
	int id_digital_out;
	char description[50];
	int device_num;
	int ch_num;
	int def;				// default value
	int on_value;			// on value
	int po_delay;			// poweron delay
	int on_time;			// relay impulse length (0=no impulse means no releay involved)
	int pon_time;			// time taken to fully power on
	int poff_time;			// time taken to fully power off
	int id_digital;			// associate digital input for relay value
	int value;				// current value
	int value1;				// past value
	int req_value;			// requested value
	bool printer;
	bool sms;
	char msg_l[80];			// message when going 0
	char msg_h[80];			// message when going 1
	bool msg_is_event;
	time_t start_time;		// start timestamp 
	time_t act_time;
};
struct digitalOutLine *digitalOutTable;

int DIGITALOUTCHANNELS;
int SHMDIGITALOUTID;

void initializeDigitalOutTable();
void initializeDigitalOutLine(int i);
void updateDigitalOutChannel(int fd,int id_digital_out);
int id_digitalOutToId(int id);
int loadDigitalOutTable(bool onlyupdate);
void resetDeviceDigitalOut(int device_num);
void resetDigitalOutValues(int i);
void setOutputDefault(int systemId);
int setOutput(int id,int value);

#endif /* DIGITAL_OUT_H */

