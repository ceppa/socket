#ifndef ANALOG_H
#define ANALOG_H

#include <sys/time.h>
#include <time.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "systems.h"
#include "common.h"

struct analogLine
{
	int id_analog;
	char form_label[20];
	char description[50];
	char label[15];
	char sinottico[20];
	int device_num;
	int ch_num;
	int scale_zero;
	int scale_full;
	double range_zero;
	double range_full;
	double range_pend;
	bool bipolar;
	bool al_high_active;
	double al_high;
	bool al_low_active;
	double al_low;
	double offset;
	char unit[5];
	int time_delay_high;
	int time_delay_low;
	int time_delay_high_off;
	int time_delay_low_off;

	char *curve;
	bool no_linear;
	bool printer;
	bool sms;
	char msg_l[80];
	char msg_h[80];
	bool msg_is_event;
	bool enabled;
	int record_data_time;
	time_t stored_time;

	bool currently_high;
	bool currently_low;

	int value;
	int value1;
	int value2;
	int value3;
	int value4;

	bool value_valid;
	double value_eng;
	double value_eng1;
	double value_eng2;
	double value_eng3;
	double value_eng4;
	double value_max;
	time_t ts_max;
	double value_min;
	time_t ts_min;
	
	time_t time_delay_high_cur;
	time_t time_delay_low_cur;
	time_t time_delay_high_off_cur;
	time_t time_delay_low_off_cur;
};
struct analogLine *analogTable;

int ANALOGCHANNELS;
int SHMANALOGID;
void initializeAnalogTable(bool onlyupdate);
int updateAnalogChannel(int id_analog);
int id_analogToId(int id_analog);
int loadAnalogTable(bool onlyupdate);
void resetAnalogValues(int i);
void storeAnalogTable(int device_num);
int storeAnalogLine(int i,MYSQL *conn);
void dumpAnalogTable();

#endif /* ANALOG_H */
