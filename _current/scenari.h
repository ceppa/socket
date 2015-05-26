#ifndef SCENARI_H
#define SCENARI_H

#include <sys/time.h>
#include <time.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "systems.h"
#include "digital_out.h"
#include "common.h"
#include "util.h"

struct digitalOutLine;

struct scenariBgBnLine
{
	int id_digital_out;
	bool attivo;
	bool bg;
	int ritardo;
};
struct scenariBgBnLine *scenariBgBnTable;

struct scenariPresenzeLine
{
	int id_digital_out;
	bool attivo;
	bool ciclico;
	int tempo_on;		//minuti
	int tempo_off;		//minuti
	int ora_ini;		//minuti
	int ora_fine;		//minuti
};
struct scenariPresenzeLine *scenariPresenzeTable;
char *scenariPresenzeAttivo;
char *buonGiornoAttivo;
char *buonaNotteAttivo;

int SCENARIPRESENZECOUNT;
int SCENARIBGBNCOUNT;

int SHMSCENARIPRESENZEID;
int SHMSCENARIPRESENZEATTIVOID;
int SHMSCENARIBGATTIVOID;
int SHMSCENARIBNATTIVOID;
int SHMSCENARIBGBNID;

void initializeScenariPresenzeTable();
void initializeScenariBgBnTable();
int id_digitalOutToId_scenariPresenze(int id_digital_out);
int id_digitalOutToId_scenariBgBn(int id_digital_out,bool bg);
int loadScenariPresenzeTable(bool onlyupdate);
int loadScenariBgBnTable(bool onlyupdate);
void resetScenariPresenzeValues(int i);
void resetScenariBgBnValues(int i);
char checkBuonGiornoAttivo();
char checkBuonaNotteAttivo();
void updateDayNight(int fd);
void updatePresenze(int fd);
void doScenariPresenze();

#endif /* SCENARI_H */
