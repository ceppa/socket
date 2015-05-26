#ifndef MULTIMETER_H
#define MULTIMETER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "common.h"

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
	bool sms;
	int alarm_value;
	char msg[80];
	bool msg_is_event;
	bool enabled;

	int value;
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
int operateMultimeter(int sockfd,int multimeterId, int timeout,int flip);
int multmeterNumToId(int multimeterNum,int totale);
void multimeterDisconnected(int multimeterId);
int loadMultimeterTable(bool onlyupdate);
int loadReadingTable(bool onlyupdate);
void initializeReadingTable();
void resetReadingValues(int i);
int id_readingToId(int id_reading);

#endif /* MULTIMETER_H */
