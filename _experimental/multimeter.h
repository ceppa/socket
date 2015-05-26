#ifndef MULTIMETER_H
#define MULTIMETER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "common.h"
#include "sms.h"

struct readingLine
{
	int id_reading;
	char form_label[20];
	char description[50];
	char label[15];
	char sinottico[20];
	int multimeter_num;
	int ch_num;
	bool printer;
	bool msg_is_event;

	double al_high_value;
	bool al_high_active;
	char al_high_msg[255];
	int time_delay_high_on;
	int time_delay_high_off;
	int sms_high_active;
	double al_low_value;
	bool al_low_active;
	char al_low_msg[255];
	int time_delay_low_on;
	int time_delay_low_off;
	bool sms_low_active;
	char unit[255];
	int record_data_time; 
	time_t stored_time;
	bool enabled;
	int res;
	bool unsign;

	unsigned char raw[20];
	int raw_length;

	double value;
	bool value_valid;
	double value_max;
	time_t ts_max;
	double value_min;
	time_t ts_min;

	time_t time_high_on;
	time_t time_high_off;
	time_t time_low_on;
	time_t time_low_off;
	bool alarm_high_on;
	bool alarm_low_on;
};
struct readingLine *readingTable;


struct multimeter
{
	int multimeter_num;
	char address[14];
	int port;
	unsigned char in_bytes_1_length;
	unsigned char *in_bytes_1;
	unsigned char in_bytes_2_length;
	unsigned char *in_bytes_2;
	unsigned char header_in_length;
	unsigned char *header_in;
	unsigned char header_out_length;
	unsigned char *header_out;
	unsigned char out_ch_1;
	unsigned char out_ch_2;
	unsigned char out_ch_1_bytes;
	unsigned char out_ch_2_bytes;
	bool enabled;
	char status;
	char failures;
	int sockfd;
};
struct multimeter *multimeters;

int READINGCHANNELS;
int NUMMULTIMETERS;
int SHMMULTIMETERSID;
int SHMREADINGID;

void doMultimeter(int l);
int doConnectMultimeter(int multimeterId);
int operateMultimeter(int multimeterId, int timeout,int flip);
int operateMeter(int l,int timeout,int flip);
int operatePowerMeter(int l, int timeout);
int multmeterNumToId(int multimeterNum,int totale);
void multimeterDisconnected(int multimeterId);
int loadMultimeterTable(bool onlyupdate);
int loadReadingTable(bool onlyupdate);
void initializeReadingTable();
void resetReadingValues(int i);
int id_readingToId(int id_reading);
void updateMultimeter(int multimeter_num);
void checkMultimeterReload(int id);
void storeMultimeterTable(int multimeter_num);
int storeMultimeterLine(int i,MYSQL *conn);
void mmHandleMaxMin(int readingId,double reading);
void mmHandleAlarms(int readingId,double reading);
unsigned char pmDoCheckSum(unsigned char *buf,int from, int to);
unsigned char pmParseData(unsigned char *buf,int len,int *offsetin,int id_reading);
int pmGetDataLength(unsigned char dib);
double bcd2double(unsigned char *buf,int len,int exp);
double int2double(unsigned char *buf,int len,int exp);
int pmRecvReply(int sockfd,unsigned char *buf,int len,int timeout);
int updateMultimeterLine(int id);
long double convertBufferToFloat(unsigned char *raw,int raw_length,int unsign,int res);

#endif /* MULTIMETER_H */
