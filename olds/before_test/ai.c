#include "ai.h"

struct ai_input_node *addInputNode(struct ai_input_node *old,
	int id_digital)
{
	struct ai_input_node *newNode;
	newNode=(struct ai_input_node *)malloc(sizeof(struct ai_input_node));
	newNode->id_digital=id_digital;
	newNode->output_nodes=NULL;
	newNode->next=old;
	return newNode;
}

struct ai_output_node *addOutputNode(struct ai_output_node *old,
	int id_digital_out)
{
	struct ai_output_node *newNode;
	newNode=(struct ai_output_node *)malloc(sizeof(struct ai_output_node));
	newNode->id_digital_out=id_digital_out;
	newNode->next=old;
	return newNode;
}


void addNode(int id_sistema,int id_digital,int id_digital_out,
		int id_ai_sottosistema)
{
	register int i,j;
	struct ai_system_node *system_temp;
	struct ai_input_node *input_temp;
	struct ai_output_node *output_temp;

	i=0;
	while((i<NUM_AI_SISTEMI)&&(ai_sistemi[i].id_sistema!=id_sistema))
		i++;

	if(i==NUM_AI_SISTEMI)
		return;

	system_temp=&ai_sistemi[i];


	input_temp=system_temp->input_nodes;
	while(input_temp && (input_temp->id_digital!=id_digital))
		input_temp=input_temp->next;
	if(!input_temp)
	{
		input_temp=addInputNode(system_temp->input_nodes,id_digital);
		system_temp->input_nodes=input_temp;
	}

	output_temp=input_temp->output_nodes;
	while(output_temp && (output_temp->id_digital_out!=id_digital_out))
		output_temp=output_temp->next;
	if(!output_temp)
	{
		output_temp=addOutputNode(input_temp->output_nodes,id_digital_out);
		input_temp->output_nodes=output_temp;
	}


	if(id_ai_sottosistema)
	{
		for(i=0;i<system_temp->n_ss;i++)
			if((system_temp->ss_nodes[i].id_ai_sottosistema==-1)||
					(system_temp->ss_nodes[i].id_ai_sottosistema==id_ai_sottosistema))
				break;

	


		if((i<system_temp->n_ss)&&
			(system_temp->ss_nodes[i].id_ai_sottosistema==id_ai_sottosistema))
		{
			for(j=0;j<system_temp->ss_nodes[i].n_in;j++)
				if((system_temp->ss_nodes[i].ai_input_nodes[j]==0)||
					(system_temp->ss_nodes[i].ai_input_nodes[j]==input_temp))
					break;
			if(j<system_temp->ss_nodes[i].n_in)
				system_temp->ss_nodes[i].ai_input_nodes[j]=input_temp;
		}			
	}
}


int loadIntrusione()
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,j,k,n;
	int idRow;
	int out;
	int n_ss;
	int id_sistema;
	char descrizione[30];
	unsigned char enabled;
	int id_digital;
	int id_digital_out;
	int id_ai_sottosistema;
	
// sistemi
	state = mysql_query(connection, "SELECT ai_sistemi.id_sistema,"
			"ai_sistemi.descrizione,ai_sistemi.enabled,"
			"count(ai_sottosistemi.id_ai_sottosistema) AS n_ss "
			"FROM ai_sistemi LEFT JOIN ai_sottosistemi "
			"ON ai_sistemi.id_sistema=ai_sottosistemi.id_ai_sistema "
			"GROUP BY ai_sistemi.id_sistema");

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);
	NUM_AI_SISTEMI=mysql_num_rows(result);

	printf("get_shared_memory_segment: ai_sistemi\n");
	ai_sistemi=(struct ai_system_node *)get_shared_memory_segment
		(NUM_AI_SISTEMI * sizeof(struct system), &SHMAISYSTEMS, "/dev/zero");
	if(!ai_sistemi)
		die("ai_sistemi - get_shared_memory_segment\n");

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		id_sistema=atoi(row[0]);
		if(row[1])
			strcpy(descrizione,row[1]);
		else
			strcpy(descrizione,"");
		enabled=atoi(row[2]);
		n_ss=atoi(row[3]);
		ai_sistemi[idRow].id_sistema=id_sistema;
		strcpy(ai_sistemi[idRow].descrizione,descrizione);
		ai_sistemi[idRow].enabled=enabled;
		ai_sistemi[idRow].n_ss=n_ss;
		ai_sistemi[idRow].ss_active=0;
		ai_sistemi[idRow].active=0;
		ai_sistemi[idRow].ss_nodes=(struct ai_sottosistema *)malloc(n_ss*sizeof(struct ai_sottosistema));
		for(i=0;i<n_ss;i++)
			ai_sistemi[idRow].ss_nodes[i].id_ai_sottosistema=-1;
		idRow++;
	}
	mysql_free_result(result);

// sottosistemi
	state = mysql_query(connection, "SELECT ai_sistemi.id_sistema, "
		"ai_sottosistemi.id_ai_sottosistema, ai_sottosistemi.descrizione, "
		"COUNT( ai_sottosistemi_input.id_ai_input ) as c "
		"FROM ai_sistemi LEFT JOIN ai_sottosistemi "
		"ON ai_sistemi.id_sistema = ai_sottosistemi.id_ai_sistema "
		"LEFT JOIN ai_sottosistemi_input "
		"ON ai_sottosistemi.id_ai_sottosistema = "
			"ai_sottosistemi_input.id_ai_sottosistema "
		"WHERE ai_sottosistemi.id_ai_sottosistema IS NOT NULL "
		"GROUP BY ai_sottosistemi.id_ai_sottosistema");

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		id_sistema=atoi(row[0]);
		id_ai_sottosistema=atoi(row[1]);
		if(row[2])
			strcpy(descrizione,row[2]);
		else
			strcpy(descrizione,"");
		n=atoi(row[3]);
		
		for(i=0;i<NUM_AI_SISTEMI;i++)
			if(ai_sistemi[i].id_sistema==id_sistema)
				break;

		if(i<NUM_AI_SISTEMI)
		{
			for(k=0;k<ai_sistemi[i].n_ss;k++)
				if(ai_sistemi[i].ss_nodes[k].id_ai_sottosistema==-1)
					break;
			
			if(k<ai_sistemi[i].n_ss)
			{
				ai_sistemi[i].ss_nodes[k].id_ai_sottosistema=id_ai_sottosistema;
				strcpy(ai_sistemi[i].ss_nodes[k].descrizione,descrizione);
				ai_sistemi[i].ss_nodes[k].n_in=n;
				ai_sistemi[i].ss_nodes[k].ai_input_nodes=
					(struct ai_input_node **)malloc(n*sizeof(struct ai_input_node *));
				for(j=0;j<n;j++)
					ai_sistemi[i].ss_nodes[k].ai_input_nodes[j]=0;
			}
		}
	}

// canali

	state = mysql_query(connection, "SELECT ai_sistemi.id_sistema, "
			"ai_input.id_digital, ai_output.id_digital_out, "
			"ai_sottosistemi_input.id_ai_sottosistema "
			"FROM ai_sistemi "
			"LEFT JOIN ai_input "
				"ON ai_sistemi.id_sistema = ai_input.id_sistema "
			"LEFT JOIN ai_output "
				"ON ai_input.id_ai_input = ai_output.id_ai_input "
			"LEFT JOIN ai_sottosistemi "
				"ON ai_sistemi.id_sistema = ai_sottosistemi.id_ai_sistema "
			"LEFT JOIN ai_sottosistemi_input "
				"ON ai_input.id_ai_input = ai_sottosistemi_input.id_ai_input "
				"AND ai_sottosistemi.id_ai_sottosistema = ai_sottosistemi_input.id_ai_sottosistema ");

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}

	result = mysql_store_result(connection);
	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		id_sistema=atoi(row[0]);
		id_digital=atoi(row[1]);
		id_digital_out=atoi(row[2]);
		id_ai_sottosistema=atoi(row[3]?row[3]:"0");
		addNode(id_sistema,id_digital,id_digital_out,id_ai_sottosistema);
		idRow++;
	}
	mysql_free_result(result);
	printAI();
	return(0);
}

int ai_system_can_start(int id_sistema,int *locking_ids)
{
	// locking_ids deve essere di dimensione DIGITALCHANNELS
	// per essere sicuri di non avere overflow

	int id;
	char *out;
	struct ai_input_node *input_temp;
	int n=0;
	register int i;

	for(i=0;i<NUM_AI_SISTEMI;i++)
		if(ai_sistemi[i].id_sistema==id_sistema)
			break;
	if(i==NUM_AI_SISTEMI)
		return -1;

	input_temp=ai_sistemi[i].input_nodes;
	while(input_temp!=NULL)
	{
		id=id_digitalToId(input_temp->id_digital);
		if(digitalTable[id].value==digitalTable[id].on_value)
		{
			locking_ids[n]=input_temp->id_digital;
			n++;
		}
		input_temp=input_temp->next;
	}
	locking_ids[n]=0;
	return n;
}

void free_system_nodes()
{
	register int i,j;
	struct ai_input_node *it,*input_temp;
	struct ai_output_node *ot,*output_temp;

	for(i=0;i<NUM_AI_SISTEMI;i++)
	{
		input_temp=ai_sistemi[i].input_nodes;
		while(input_temp)
		{
			it=input_temp;
			input_temp=input_temp->next;

			output_temp=it->output_nodes;
			while(output_temp)
			{
				ot=output_temp;
				output_temp=output_temp->next;
				free(ot);
			}
			free(it);
		}

		for(j=0;j<ai_sistemi[i].n_ss;j++)
			free(ai_sistemi[i].ss_nodes[j].ai_input_nodes);

		free(ai_sistemi[i].ss_nodes);
	}
}

void printAI()
{
	register int i;
	struct ai_system_node *system_temp;
	struct ai_input_node *input_temp;
	struct ai_output_node *output_temp;

	for(i=0;i<NUM_AI_SISTEMI;i++)
	{
		system_temp=&ai_sistemi[i];
		printf("id: %d desc: %s n_ss: %d\n",
			system_temp->id_sistema,
			system_temp->descrizione,
			system_temp->n_ss);
		input_temp=system_temp->input_nodes;
		while(input_temp)
		{
			printf("\tin: %d\n",input_temp->id_digital);
			output_temp=input_temp->output_nodes;
			while(output_temp)
			{
				printf("\t\tout: %d\n",output_temp->id_digital_out);
				output_temp=output_temp->next;
			}
			input_temp=input_temp->next;
		}
	}
}

void doAntiIntrusione()
{
	register int i,j,k;
	struct ai_system_node *system_node;
	struct ai_input_node *input_nodes;
	struct ai_output_node *output_nodes;
	struct ai_sottosistema *ss_nodes;
	int id_digital,id_digital_out;

	for(i=0;i<NUM_AI_SISTEMI;i++)
	{
		system_node=&ai_sistemi[i];
		if(system_node->active)
		{
			input_nodes=system_node->input_nodes;
			while(input_nodes)
			{
				id_digital=id_digitalToId(input_nodes->id_digital);
				if(digitalTable[id_digital].value==digitalTable[id_digital].on_value)
				{
					output_nodes=input_nodes->output_nodes;
					while(output_nodes)
					{
						activateOutput(output_nodes->id_digital_out);
						output_nodes=output_nodes->next;
					}
				}
				input_nodes=input_nodes->next;
			}
		}
		else
		{
			if(system_node->ss_active)
			{
				ss_nodes=system_node->ss_nodes;
				for(j=0;j<system_node->n_ss;j++)
				{
					if(system_node->ss_active & j)
					{
						for(k=0;k<ss_nodes->n_in;k++)
						{
							input_nodes=ss_nodes->ai_input_nodes[k];
							id_digital=id_digitalToId(input_nodes->id_digital);
							if(digitalTable[id_digital].value==digitalTable[id_digital].on_value)
							{
								output_nodes=input_nodes->output_nodes;
								while(output_nodes)
								{
									activateOutput(output_nodes->id_digital_out);
									output_nodes=output_nodes->next;
								}
							}
						}
						
					}
				}
			}
		}
	}
}

int get_active_ai()
{
	register int i;
	int out=0;

	for(i=0;i<NUM_AI_SISTEMI;i++)
		if(ai_sistemi[i].active)
			out+=(1<<ai_sistemi[i].id_sistema);
	return out;
}

int get_active_ai_ss(int id_sistema)
{
	register int i,j;
	int out=0;

	for(i=0;i<NUM_AI_SISTEMI;i++)
		if(ai_sistemi[i].id_sistema==id_sistema)
			break;
	if(i<NUM_AI_SISTEMI)
	{
		for(j=0;j<ai_sistemi[i].n_ss;j++)
			if(j & ai_sistemi[i].ss_active)
				out+=(1<<ai_sistemi[i].ss_nodes[j].id_ai_sottosistema);
	}
	else
		out -1;
	return out;
}

int set_active_ai(int id_sistema,bool on)
{
	register int i;
	int out;

	for(i=0;i<NUM_AI_SISTEMI;i++)
		if(ai_sistemi[i].id_sistema==id_sistema)
			break;
	if(i<NUM_AI_SISTEMI)
	{
		ai_sistemi[i].active=on;
		ai_sistemi[i].ss_active=0;
		out=1;
	}
	else
		out=0;
	return out;
}

int set_active_ai_ss(int id_sistema,int id_ss,bool on)
{
	register int i,j;
	int out;

	for(i=0;i<NUM_AI_SISTEMI;i++)
		if(ai_sistemi[i].id_sistema==id_sistema)
			break;
	if(i<NUM_AI_SISTEMI)
	{
		ai_sistemi[i].active=0;
		for(j=0;j<ai_sistemi[i].n_ss;j++)
			if(id_ss & ai_sistemi[i].ss_nodes[j].id_ai_sottosistema)
			{
				if(on)
					ai_sistemi[i].ss_active+=(1<<j);
				else
				{
					if(ai_sistemi[i].ss_active&(1<<j))
						ai_sistemi[i].ss_active-=(1<<j);
				}
			}
		out=1;
	}
	else
		out=0;
	return out;
}
