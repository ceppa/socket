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
	int req_value;			// requested value, after possible po_delay
	int value_valid;				// current value to send to board
	int value;				// current value to send to board
	int value_real;			// last value successfully sent to board
	bool printer;
	bool sms;
	char msg_l[80];			// message when going 0
	char msg_h[80];			// message when going 1
	bool msg_is_event;
	time_t start_time;		// start timestamp 
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
void setSingleOutputDefault(int idRow);
void setOutputDefault(int systemId);
int setOutput(int id,int value);
int activateOutput(int id);
int deactivateOutput(int id);
void activateOutputImmediate(int id);
void deactivateOutputImmediate(int id);
void getDigitalOutTimings(struct digitalOutLine *digital_out_line,
		int *po_delay_rem,int *pon_time_rem,int *on_time_rem,int *poff_time_rem);

#endif /* DIGITAL_OUT_H */

