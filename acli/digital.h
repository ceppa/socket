#ifndef DIGITAL_H
#define DIGITAL_H

#include <sys/time.h>
#include <time.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "systems.h"
#include "common.h"
#include "util.h"

struct digitalLine
{
	int id_digital;
	char form_label[20];
	char description[50];
	char label[15];
	char sinottico[20];
	int device_num;
	int ch_num;
	bool printer;
	bool sms;
	int on_value;
	int alarm_value;
	int time_delay_on;
	int time_delay_off;
	int currently_on;
	char msg_on[255];
	char msg_off[255];
	bool msg_is_event;
	bool enabled;

	bool value_valid;
	int value;
	int value1;
	int value2;
	int value3;
	int value4;
	time_t time_delay_on_cur;
	time_t time_delay_off_cur;
};
struct digitalLine *digitalTable;

int SHMPIOGGIA;
int DIGITALCHANNELS;
int SHMDIGITALID;
int *id_digital_pioggia;

void initializeDigitalTable();
void updateDigitalChannel(int id_digital);
int id_digitalToId(int id_digital);
int loadDigitalTable(bool onlyupdate);
void resetDigitalValues(int i);

#endif /* DIGITAL_H */
