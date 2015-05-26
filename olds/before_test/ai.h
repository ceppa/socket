#ifndef AI_H
#define AI_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include "common.h"
#include "digital.h"

struct ai_output_node
{
	int id_digital_out;
	struct ai_output_node *next;
};

struct ai_input_node
{
	int id_digital;
	struct ai_output_node *output_nodes;
	struct ai_input_node *next;
};

struct ai_sottosistema
{
	int id_ai_sottosistema;
	char descrizione[30];
	int n_in;
	struct ai_input_node **ai_input_nodes;
};


struct ai_system_node
{
	int id_sistema;
	char descrizione[30];
	bool enabled;
	bool active;
	int ss_active;
	int n_ss;
	struct ai_input_node *input_nodes;
	struct ai_sottosistema *ss_nodes;
} *ai_sistemi;

int AI_SISTEMI;
int NUM_AI_SISTEMI;
int SHMAISYSTEMS;

struct ai_input_node *addInputNode(struct ai_input_node *old, 
	int id_digital);
struct ai_output_node *addOutputNode(struct ai_output_node *old,
	int id_digital_out);
void addNode(int id_sistema,int id_digital,int id_digital_out,
		int id_ai_sottosistema);

int loadIntrusione();
int ai_system_can_start(int id_sistema,int *locking_ids);
void free_system_nodes();
void printAI();
void doAntiIntrusione();
int get_active_ai();
int get_active_ai_ss(int id_sistema);// output mask
int set_active_ai(int id_sistema,bool on);
int set_active_ai_ss(int id_sistema,int id_ss,bool on);

#endif /* AI_H */
