#ifndef IRRIGAZIONE_H
#define IRRIGAZIONE_H

#include <sys/time.h>
#include <time.h>
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "systems.h"
#include "common.h"

struct irrigazioneCircuitiLine
{
	int id_digital_out;
	int id_irrigazione;
	int circuito;
	int durata;
	int validita;

	time_t tempo_on_cur;
};
struct irrigazioneCircuitiLine *irrigazioneCircuitiTable;

struct irrigazioneLine
{
	int id_irrigazione;
	int ora_start;
	int ripetitivita;
	int tempo_off;
	int num_circuiti;
	int id_digital_out;
	char msg_avvio_automativo[80];
	char msg_arresto_automatico[80];
	char msg_avvio_manuale[80];
	char msg_arresto_manuale[80];
	char msg_arresto_pioggia[80];
	char msg_pioggia_noinizio[80];
	char msg_avvio_pompa[80];
	char msg_arresto_pompa[80];

	int day;
	int attivo_forzato;
//	int ora_started;
	time_t ora_started;
	int current_circuito;
	int wday_started;
	int current_time;
	int count;	//if count>0 => attivo
};
struct irrigazioneLine *irrigazioneTable;



int SHMIRRIGAZIONEID;
int SHMIRRIGAZIONECIRCUITIID;
int IRRIGAZIONECIRCUITS;
int IRRIGAZIONESYSTEMS;

void initializeIrrigazioneCircuitiTables();
void updateIrrigazioneCircuiti(int fd,int id_irrigazione);
int id_irrigazioneToId(int id_irrigazione);
int circuito_irrigazioneToId(int id_irrigazione,int circuito_irrigazione);
int loadIrrigazioneTables(int id_irrigazione,int circuito);
void resetIrrigazioneValues(int i);
void resetIrrigazioneCircuitiValues(int i);
void set_irrigazione(int fd,char *c);
void get_irrigazione(int fd,char *c);
void irrigazione_attiva(int id_irrigazione,char *msg);
void irrigazione_disattiva(int id_irrigazione,char *msg);
void get_irrigazione_rt(int fd,char *c);
void reset_irrigazione(int id_irrigazione);
void irrigazione_attiva_pompa(int id_irrigazione);
void irrigazione_disattiva_pompa(int id_irrigazione);
int circuiti_attivi(int id_irrigazione);
void doIrrigazione();
void irrigazione_attiva_circuito(int id_irrigazione,int id_circuito);
void irrigazione_prossimo(int i,int wday);

#endif /* IRRIGAZIONE_H */
