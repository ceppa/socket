#include "irrigazione.h"

void initializeIrrigazioneCircuitiTables()
/*--------------------------------------------
 * tutti valori iniziali su tabella irrigazione
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<IRRIGAZIONECIRCUITS;i++)
	{
		irrigazioneCircuitiTable[i].id_irrigazione=-1;
		irrigazioneCircuitiTable[i].circuito=-1;
		irrigazioneCircuitiTable[i].durata=-1;
		irrigazioneCircuitiTable[i].id_digital_out=-1;
		irrigazioneCircuitiTable[i].validita=-1;
		resetIrrigazioneCircuitiValues(i);
	}
	for(i=0;i<IRRIGAZIONESYSTEMS;i++)
	{
		irrigazioneTable[i].ora_start=-1;
		irrigazioneTable[i].ripetitivita=-1;
		irrigazioneTable[i].tempo_off=-1;
		irrigazioneTable[i].id_digital_out=-1;
		irrigazioneTable[i].num_circuiti=0;
		irrigazioneTable[i].attivo=0;
		resetIrrigazioneValues(i);
	}
}


void resetIrrigazioneCircuitiValues(int i)
{
	irrigazioneCircuitiTable[i].tempo_on_cur=0;
}

void resetIrrigazioneValues(int i)
{
	irrigazioneTable[i].count=0;
	irrigazioneTable[i].day=-1;
	irrigazioneTable[i].current_circuito=-1;
	irrigazioneTable[i].current_time=-1;
}

void updateIrrigazioneCircuiti(int fd,int id_irrigazione)
{
	char buffer[255];
	char ok[3];

	if(loadIrrigazioneTables(id_irrigazione,-1))
		strcpy(ok,"ko");
	else
		strcpy(ok,"ok");

	sprintf(buffer,"%s %s",ok,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		my_perror("send");
}

int id_irrigazioneToId(int id_irrigazione)
{
	int i;
	
	for(i=0;i<IRRIGAZIONESYSTEMS;i++)
		if(irrigazioneTable[i].id_irrigazione==id_irrigazione)
			break;
	return((i<IRRIGAZIONESYSTEMS)?i:-1);
}

int circuito_irrigazioneToId(int id_irrigazione,int circuito_irrigazione)
{
	int i;
	
	for(i=0;i<IRRIGAZIONECIRCUITS;i++)
		if((irrigazioneCircuitiTable[i].id_irrigazione==id_irrigazione)&&
				(irrigazioneCircuitiTable[i].circuito==circuito_irrigazione))
			break;
	return((i<IRRIGAZIONECIRCUITS)?i:-1);
}

int circuiti_attivi(int id_irrigazione)
/*--------------------------------------------
 * 		conta quanti circuiti saranno attivabili oggi
 * -----------------------------------------*/
{
	int out=0;
	int id_circuito;
	int i;
	time_t curtime;
	struct timeval tv;
	struct tm ts;
	int wday;

	gettimeofday(&tv, NULL); 
	curtime=tv.tv_sec;
	ts=*localtime(&curtime);
	wday=(6+ts.tm_wday)%7;

	for(i=0;i<irrigazioneTable[id_irrigazione].num_circuiti;i++)
	{
		id_circuito=circuito_irrigazioneToId(irrigazioneTable[id_irrigazione].id_irrigazione,i+1);
		if(id_circuito>=0)
		{
//			my_printf("%d %d %d %d\n",i+1,irrigazioneCircuitiTable[id_circuito].circuito,irrigazioneCircuitiTable[id_circuito].durata,irrigazioneCircuitiTable[id_circuito].validita);

			if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
				out++;
		}
	}
	return out;
}

void irrigazione_attiva_pompa(int id_irrigazione)
/*--------------------------------------------
 * 		attiva la pompa relativa al sistema id_irrigazione
 * -----------------------------------------*/
{
	int id;
	struct msgform msg;

	id=id_digitalOutToId(irrigazioneTable[id_irrigazione].id_digital_out);
	if(id!=-1)
	{
		if(digitalOutTable[id].value!=digitalOutTable[id].on_value)
		{
			handleMessage(3,0,irrigazioneTable[id_irrigazione].id_irrigazione,
				0,irrigazioneTable[id_irrigazione].msg_avvio_pompa,0,1);

			digitalOutTable[id].value=digitalOutTable[id].on_value;
			digitalOutTable[id].req_value=digitalOutTable[id].on_value;
			digitalOutTable[id].po_delay=0;
		}
	}
}

void irrigazione_disattiva_pompa(int id_irrigazione)
/*--------------------------------------------
 * 		disattiva la pompa relativa al sistema id_irrigazione
 * -----------------------------------------*/
{
	int id;
	struct msgform msg;

	id=id_digitalOutToId(irrigazioneTable[id_irrigazione].id_digital_out);
	if(id!=-1)
	{
		if(digitalOutTable[id].value!=1-digitalOutTable[id].on_value)
		{
			handleMessage(3,0,irrigazioneTable[id_irrigazione].id_irrigazione,
				0,irrigazioneTable[id_irrigazione].msg_arresto_pompa,0,1);

			digitalOutTable[id].value=1-digitalOutTable[id].on_value;
			digitalOutTable[id].req_value=1-digitalOutTable[id].on_value;
			digitalOutTable[id].po_delay=0;
		}
	}
}


void irrigazione_attiva(int id_irrigazione,char *message)
/*--------------------------------------------
 * 		inizializza irrigazione per sistema id_irrigazione
 * 		usa irrigazioneTable e irrigazioneCircuitiTable
 * -----------------------------------------*/
{
	int id_circuito;
	time_t curtime;
	int minutes;
//	int seconds;
	time_t seconds;
	struct timeval tv;
	struct tm ts;
	int day,wday;
	struct msgform msg;

	if(circuiti_attivi(id_irrigazione))
	{
		handleMessage(3,0,irrigazioneTable[id_irrigazione].id_irrigazione,0,message,0,1);
		irrigazione_attiva_pompa(id_irrigazione); //se già attiva non fa nulla

		reset_irrigazione(id_irrigazione);	// azzera i circuiti
		gettimeofday(&tv, NULL); 
		curtime=tv.tv_sec;
		ts=*localtime(&curtime);
		minutes=ts.tm_hour*60+ts.tm_min;
//		seconds=minutes*60+ts.tm_sec;
		seconds=time(NULL);
		day=ts.tm_mday+32*ts.tm_mon+384*ts.tm_year;
		wday=(6+ts.tm_wday)%7;

		irrigazioneTable[id_irrigazione].day=day;
		irrigazioneTable[id_irrigazione].ora_started=seconds;
		irrigazioneTable[id_irrigazione].count=0;
		irrigazioneTable[id_irrigazione].wday_started=wday;

		irrigazioneTable[id_irrigazione].current_circuito=1;
		id_circuito=circuito_irrigazioneToId(irrigazioneTable[id_irrigazione].id_irrigazione,1);

		if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
			irrigazioneTable[id_irrigazione].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
		else
			irrigazioneTable[id_irrigazione].current_time=0;
	}
}

void reset_irrigazione(int id_irrigazione)
/*--------------------------------------------
 * 		azzera le uscite corrispondenti ai
 * 		circuiti di irrigazione
 * -----------------------------------------*/
{
	int i,j;

	for(i=0;i<IRRIGAZIONECIRCUITS;i++)
	{
		if(irrigazioneCircuitiTable[i].id_irrigazione==irrigazioneTable[id_irrigazione].id_irrigazione)
		{
			j=id_digitalOutToId(irrigazioneCircuitiTable[i].id_digital_out);
			if(j!=-1)
			{
				digitalOutTable[j].value=1-digitalOutTable[j].on_value;
				digitalOutTable[j].req_value=1-digitalOutTable[j].on_value;
				digitalOutTable[j].po_delay=0;
			}
		}
	}
}

void irrigazione_disattiva(int id_irrigazione,char *message)
/*--------------------------------------------
 * 		ferma irrigazione per sistema id_irrigazione
 * 		usa irrigazioneTable
 * -----------------------------------------*/
{
	struct msgform msg;
	int id_circuito=0,current_circuito=0;
	if(irrigazioneTable[id_irrigazione].current_circuito>0)
	{
		if(irrigazioneTable[id_irrigazione].current_circuito<=irrigazioneTable[id_irrigazione].num_circuiti)
		{
			id_circuito=circuito_irrigazioneToId(irrigazioneTable[id_irrigazione].id_irrigazione,irrigazioneTable[id_irrigazione].current_circuito);
			current_circuito=irrigazioneTable[id_irrigazione].current_circuito;
		}
		handleMessage(3,id_circuito,irrigazioneTable[id_irrigazione].id_irrigazione,
			current_circuito,message,0,1);

	}
	reset_irrigazione(id_irrigazione);
	irrigazioneTable[id_irrigazione].count=0;
	irrigazioneTable[id_irrigazione].current_circuito=0;
	irrigazioneTable[id_irrigazione].current_time=0;
	irrigazioneTable[id_irrigazione].attivo=0;
	irrigazioneTable[id_irrigazione].wday_started=-1;

	irrigazione_disattiva_pompa(id_irrigazione);
}

int loadIrrigazioneTables(int id_irrigazione,int circuito)
{
	// circuito=0 => crea tabelle irrigazione e irrigazione_circuiti
	// id_irrigazione>0,circuito>0 => aggiorna circuito su irrigazione_circuiti
	// id_irrigazione>0,circuito<0 => aggiorna id_irrigazione su irrigazione
	// id_irrigazione<0,circuito<0 => aggiorna completamente tabelle

	char query[1024];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	int id_row_irrigazione;

	if(circuito==0)
		id_irrigazione=0;
	if(id_irrigazione==0)
		circuito=0;

	if(id_irrigazione<0)
		circuito=-1;

	if(circuito==0)
	{
		sprintf(query,"SELECT id AS id_irrigazione "
						"FROM irrigazione");
		state = mysql_query(connection,query);	
		if( state != 0 )
		{
			my_printf("%s\n",mysql_error(connection));
			return(1);
		}
		result = mysql_store_result(connection);

		IRRIGAZIONESYSTEMS=mysql_num_rows(result);
		if(IRRIGAZIONESYSTEMS>0)
		{
			my_printf("get_shared_memory_segment: irrigazioneTable\n");
			irrigazioneTable=(struct irrigazioneLine *)get_shared_memory_segment(IRRIGAZIONESYSTEMS * sizeof(struct irrigazioneLine), &SHMIRRIGAZIONEID, "/dev/zero");
			if(!irrigazioneTable)
				die("irrigazioneTable - get_shared_memory_segment\n");
		}
		else
			irrigazioneTable=0;

		idRow=0;
		while((row = mysql_fetch_row(result)) != NULL )
		{
			irrigazioneTable[idRow].id_irrigazione=atoi(row[0]);
			idRow++;
		}
		mysql_free_result(result);

		sprintf(query,"SELECT count(*) as num_circuiti "
						"FROM irrigazione_circuiti ");
		state = mysql_query(connection,query);	
		if( state != 0 )
		{
			my_printf("%s\n",mysql_error(connection));
			return(1);
		}
		result = mysql_store_result(connection);
		if((row = mysql_fetch_row(result)) != NULL )
			IRRIGAZIONECIRCUITS=atoi(row[0]);
		mysql_free_result(result);

		if(IRRIGAZIONECIRCUITS>0)
		{
			my_printf("get_shared_memory_segment: irrigazioneCircuitiTable\n");
			irrigazioneCircuitiTable=(struct irrigazioneCircuitiLine *)get_shared_memory_segment(IRRIGAZIONECIRCUITS * sizeof(struct irrigazioneCircuitiLine), &SHMIRRIGAZIONECIRCUITIID, "/dev/zero");
			if(!irrigazioneCircuitiTable)
				die("irrigazioneCircuitiTable - get_shared_memory_segment\n");
		}
		else
			irrigazioneCircuitiTable=0;
			
	}

	if((circuito<=0)&&(id_irrigazione<=0))
	{
		initializeIrrigazioneCircuitiTables();
		sprintf(query,"SELECT irrigazione.id as id_irrigazione,"
							"irrigazione_circuiti.circuito,"
							"irrigazione.ora_start,"
							"irrigazione_circuiti.durata,"
							"irrigazione_circuiti.validita,"
							"irrigazione.ripetitivita,"
							"irrigazione.tempo_off,"
							"irrigazione_circuiti.id_digital_out,"
							"irrigazione.id_digital_out,"
							"irrigazione.msg_avvio_automativo,"
							"irrigazione.msg_arresto_automatico,"
							"irrigazione.msg_avvio_manuale,"
							"irrigazione.msg_arresto_manuale,"
							"irrigazione.msg_arresto_pioggia,"
							"irrigazione.msg_pioggia_noinizio,"
							"irrigazione.msg_avvio_pompa,"
							"irrigazione.msg_arresto_pompa "
						"FROM irrigazione LEFT JOIN irrigazione_circuiti "
						"ON irrigazione.id=irrigazione_circuiti.id_irrigazione "
						"ORDER BY irrigazione.id,irrigazione_circuiti.circuito");

		state = mysql_query(connection,query);	
		if( state != 0 )
		{
			my_printf("%s\n%s\n",query,mysql_error(connection));
			return(1);
		}
		result = mysql_store_result(connection);
	
		idRow=0;
		while( ( row = mysql_fetch_row(result)) != NULL )
		{
			id_irrigazione=id_irrigazioneToId(atoi(row[0]));
			irrigazioneTable[id_irrigazione].id_irrigazione=atoi(row[0]);
			irrigazioneCircuitiTable[idRow].id_irrigazione=atoi(row[0]);
			irrigazioneCircuitiTable[idRow].circuito=atoi(row[1]);
			irrigazioneTable[id_irrigazione].ora_start=atoi(row[2]);
			irrigazioneCircuitiTable[idRow].durata=atoi(row[3]);
			irrigazioneCircuitiTable[idRow].validita=atoi(row[4]);
			irrigazioneTable[id_irrigazione].ripetitivita=atoi(row[5]);
			irrigazioneTable[id_irrigazione].tempo_off=atoi(row[6]);
			irrigazioneCircuitiTable[idRow].id_digital_out=atoi(row[7]);
			irrigazioneTable[id_irrigazione].id_digital_out=atoi(row[8]);

			strcpy(irrigazioneTable[id_irrigazione].msg_avvio_automativo,row[9]);
			strcpy(irrigazioneTable[id_irrigazione].msg_arresto_automatico,row[10]);
			strcpy(irrigazioneTable[id_irrigazione].msg_avvio_manuale,row[11]);
			strcpy(irrigazioneTable[id_irrigazione].msg_arresto_manuale,row[12]);
			strcpy(irrigazioneTable[id_irrigazione].msg_arresto_pioggia,row[13]);
			strcpy(irrigazioneTable[id_irrigazione].msg_pioggia_noinizio,row[14]);
			strcpy(irrigazioneTable[id_irrigazione].msg_avvio_pompa,row[15]);
			strcpy(irrigazioneTable[id_irrigazione].msg_arresto_pompa,row[16]);

			irrigazioneTable[id_irrigazione].count=0;
			irrigazioneTable[id_irrigazione].attivo=0;
			irrigazioneTable[id_irrigazione].current_circuito=0;
			irrigazioneTable[id_irrigazione].current_time=0;
			irrigazioneTable[id_irrigazione].wday_started=-1;

			irrigazioneTable[id_irrigazione].num_circuiti++;
			irrigazioneCircuitiTable[idRow].tempo_on_cur=0;
			idRow++;
		}
		mysql_free_result(result);
	}
	else //id_irrigazione > 0
	{
		id_row_irrigazione=id_irrigazioneToId(id_irrigazione);
		if(circuito>0)
		{
			sprintf(query,"SELECT irrigazione_circuiti.durata,"
							"irrigazione_circuiti.validita,"
							"irrigazione_circuiti.id_digital_out "
						"FROM irrigazione_circuiti "
						"WHERE id_irrigazione=%d and circuito=%d",id_irrigazione,circuito);
			state = mysql_query(connection,query);	
			if( state != 0 )
			{
				my_printf("%s\n",mysql_error(connection));
				return(1);
			}
			result = mysql_store_result(connection);
		
			idRow=circuito_irrigazioneToId(id_irrigazione,circuito);
			if( ( row = mysql_fetch_row(result)) != NULL )
			{
				irrigazioneCircuitiTable[idRow].durata=atoi(row[0]);
				irrigazioneCircuitiTable[idRow].validita=atoi(row[1]);
				irrigazioneCircuitiTable[idRow].id_digital_out=atoi(row[2]);
				irrigazioneCircuitiTable[idRow].tempo_on_cur=0;
			}
			mysql_free_result(result);
		}
		else
		{
			sprintf(query,"SELECT irrigazione.ora_start,"
							"irrigazione.ripetitivita,"
							"irrigazione.tempo_off,"
							"count(irrigazione_circuiti.id_irrigazione),"
							"irrigazione.id_digital_out "
						"FROM irrigazione LEFT JOIN irrigazione_circuiti "
						"ON irrigazione.id=irrigazione_circuiti.id_irrigazione "
						"WHERE id=%d "
						"GROUP BY irrigazione_circuiti.id_irrigazione",id_irrigazione);
			state = mysql_query(connection,query);	
			if( state != 0 )
			{
				my_printf("%s\n",mysql_error(connection));
				return(1);
			}
			result = mysql_store_result(connection);
		
			idRow=id_row_irrigazione;
			if( ( row = mysql_fetch_row(result)) != NULL )
			{
				irrigazioneTable[idRow].ora_start=atoi(row[0]);
				irrigazioneTable[idRow].ripetitivita=atoi(row[1]);
				irrigazioneTable[idRow].tempo_off=atoi(row[2]);
				irrigazioneTable[idRow].num_circuiti=atoi(row[3]);
				irrigazioneTable[idRow].id_digital_out=atoi(row[4]);
				irrigazioneTable[idRow].attivo=0;
			}
			mysql_free_result(result);


			sprintf(query,"SELECT irrigazione_circuiti.circuito,"
							"irrigazione_circuiti.durata,"
							"irrigazione_circuiti.validita,"
							"irrigazione_circuiti.id_digital_out "
						"FROM irrigazione_circuiti "
						"WHERE id_irrigazione=%d "
						"ORDER BY irrigazione_circuiti.circuito",id_irrigazione);
			state = mysql_query(connection,query);	
			if( state != 0 )
			{
				my_printf("%s\n",mysql_error(connection));
				return(1);
			}
			result = mysql_store_result(connection);
		
			while( ( row = mysql_fetch_row(result)) != NULL )
			{
				idRow=circuito_irrigazioneToId(id_irrigazione,atoi(row[0]));
				irrigazioneCircuitiTable[idRow].durata=atoi(row[1]);
				irrigazioneCircuitiTable[idRow].validita=atoi(row[2]);
				irrigazioneCircuitiTable[idRow].id_digital_out=atoi(row[3]);
				irrigazioneCircuitiTable[idRow].tempo_on_cur=0;
			}
			mysql_free_result(result);
			
		}
		if(circuiti_attivi(id_row_irrigazione)==0)
			irrigazione_disattiva(id_row_irrigazione,irrigazioneTable[id_row_irrigazione].msg_arresto_manuale);
	}
	return(0);
}

void set_irrigazione(int fd,char *c)
{
/*	set valore irrigazione
 * id_irrigazione,circuito,ora_start,ripetitivita,tempo_off,durata,validita
 */

	char query[255];
	char s1[127],s2[127];
	int state;
	char *pch;
	int id_irrigazione,circuito,ora_start,durata,validita,ripetitivita,tempo_off,conta;
	int id;

	id_irrigazione=0;
	conta=0;
	circuito=0;
	ora_start=0;
	durata=0;
	validita=0;
	ripetitivita=0;
	tempo_off=0;
	
	pch = strtok (c," ");
	while (pch != NULL)
	{
		switch(conta)
		{
			case 0:
				id_irrigazione=atoi(pch);
				break;
			case 1:
				circuito=atoi(pch);
				break;
			case 2:
				ora_start=atoi(pch);
				break;
			case 3:
				ripetitivita=atoi(pch);
				break;
			case 4:
				tempo_off=atoi(pch);
				break;
			case 5:
				durata=atoi(pch);
				break;
			case 6:
				validita=atoi(pch);
				break;
		}
		conta++;
		pch = strtok (NULL, " ");
	}

	if(circuito>0)
	{
		id=circuito_irrigazioneToId(id_irrigazione,circuito);
		if(id!=-1)
		{
			sprintf(s1,"%d %d %d\n",circuito,durata,validita);
			sprintf(query,"UPDATE irrigazione_circuiti SET durata=%d,validita=%d "
						"WHERE circuito=%d AND id_irrigazione=%d"
						,durata,validita,circuito,id_irrigazione);


			state = mysql_query(connection, query);
			if(state==0)
				loadIrrigazioneTables(id_irrigazione,circuito);
			sprintf(s2,"%d %d %d\n",irrigazioneCircuitiTable[id].circuito,
				irrigazioneCircuitiTable[id].durata,
				irrigazioneCircuitiTable[id].validita);
		}
	}
	else
	{
		id=id_irrigazioneToId(id_irrigazione);
		if(id!=-1)
		{
			sprintf(s1,"%d %d %d\n",ora_start,ripetitivita,tempo_off);
			sprintf(query,"UPDATE irrigazione SET ora_start=%d,ripetitivita=%d,tempo_off=%d "
							"WHERE id=%d"
						,ora_start,ripetitivita,tempo_off,id_irrigazione);

			state = mysql_query(connection, query);
			if(state==0)
				loadIrrigazioneTables(id_irrigazione,-1);
			sprintf(s2,"%d %d %d\n",irrigazioneTable[id].ora_start,
				irrigazioneTable[id].ripetitivita,
				irrigazioneTable[id].tempo_off);
		}

	}
};

void get_irrigazione(int fd,char *c)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[511];
	char buffer[255];
	int state;

	strcpy(query,"SELECT irrigazione.id,"
				"irrigazione_circuiti.circuito,"
				"irrigazione.ora_start,"
				"irrigazione.ripetitivita,"
				"irrigazione.tempo_off,"
				"irrigazione_circuiti.durata,"
				"irrigazione_circuiti.validita"
				" FROM irrigazione LEFT JOIN irrigazione_circuiti"
				" ON irrigazione.id=irrigazione_circuiti.id_irrigazione");
	if(atoi(c)>0)
		sprintf(query,"%s WHERE irrigazione.id=%d",query,atoi(c));


	state = mysql_query(connection, query);

	if( state != 0 )
	{
		my_perror(mysql_error(connection));
		return;
	}

	result = mysql_store_result(connection);

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		sprintf(buffer,"%d %d %d %d %d %d %d\n",
			atoi(row[0]),atoi(row[1]),atoi(row[2]),atoi(row[3]),
			atoi(row[4]),atoi(row[5]),atoi(row[6]));
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			my_perror("send");
	}
	mysql_free_result(result);
	strcpy(buffer,TERMINATOR);
	if (send(fd, buffer,strlen(buffer), 0) == -1)
		my_perror("send");
	return;
};

void get_irrigazione_rt(int fd,char *c)
{
	char buffer[255];
	int i,id_irrigazione,id_circuito;

	if(atoi(c)>0)
	{
		id_irrigazione=id_irrigazioneToId(atoi(c));
		if(id_irrigazione!=-1)
		{
			sprintf(buffer,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
				"id_irr","ora_st","ripet",
				"num_cir","day","attivo","started",
				"cur_c","cur_t","count");
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				my_perror("send");
			sprintf(buffer,"%d\t%d\t%d\t%d\t%d\t%d\t%ld\t%d\t%d\t%d\n",
				irrigazioneTable[id_irrigazione].id_irrigazione,
				irrigazioneTable[id_irrigazione].ora_start,
				irrigazioneTable[id_irrigazione].ripetitivita,
				irrigazioneTable[id_irrigazione].num_circuiti,
				irrigazioneTable[id_irrigazione].day,
				irrigazioneTable[id_irrigazione].attivo,
				(long)irrigazioneTable[id_irrigazione].ora_started,
				irrigazioneTable[id_irrigazione].current_circuito,
				irrigazioneTable[id_irrigazione].current_time,
				irrigazioneTable[id_irrigazione].count);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				my_perror("send");

			sprintf(buffer,"%s\t%s\t%s\n",
				"circ","durata","valid");
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				my_perror("send");

			for(i=0;i<irrigazioneTable[id_irrigazione].num_circuiti;i++)
			{
				id_circuito=circuito_irrigazioneToId(irrigazioneTable[id_irrigazione].id_irrigazione,i+1);
				if(id_circuito!=-1)
				{
					sprintf(buffer,"%d\t%d\t%d\n",
						irrigazioneCircuitiTable[id_circuito].circuito,
						irrigazioneCircuitiTable[id_circuito].durata,
						irrigazioneCircuitiTable[id_circuito].validita);
					if (send(fd, buffer,strlen(buffer), 0) == -1)
						my_perror("send");
				}
			}
		}
			
	}
	else
	{
		for(i=0;i<IRRIGAZIONESYSTEMS;i++)
		{
			sprintf(buffer,"%d %d %d %d %d %d %ld %d %d %d\n",
				irrigazioneTable[i].id_irrigazione,
				irrigazioneTable[i].ora_start,
				irrigazioneTable[i].ripetitivita,
				irrigazioneTable[i].num_circuiti,
				irrigazioneTable[i].day,
				irrigazioneTable[i].attivo,
				(long)irrigazioneTable[i].ora_started,
				irrigazioneTable[i].current_circuito,
				irrigazioneTable[i].current_time,
				irrigazioneTable[i].count);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				my_perror("send");
		}
	}
	strcpy(buffer,TERMINATOR);
	if (send(fd, buffer,strlen(buffer), 0) == -1)
		my_perror("send");
}

void doIrrigazione()
{
	time_t inizio,curtime;
	int minutes;
//	int seconds;
	time_t seconds;
	struct timeval tv;
	struct tm ts;
	int pioggia=0;
	char writeBuffer[MSGLENGTH];
	int i,j,id_circuito,circuito,id_irrigazione,day,wday;
	struct msgform msg;

	inizio=time(NULL);
	gettimeofday(&tv, NULL); 
	curtime=tv.tv_sec;
	ts=*localtime(&curtime);
	minutes=ts.tm_hour*60+ts.tm_min;
//	seconds=minutes*60+ts.tm_sec;
	seconds=time(NULL);
	day=ts.tm_mday+32*ts.tm_mon+384*ts.tm_year;
	wday=(6+ts.tm_wday)%7;

	if(*id_digital_pioggia!=-1)
	{
		if(digitalTable[*id_digital_pioggia].value!=-1)
			pioggia=(digitalTable[*id_digital_pioggia].value==1);
	}

	id_irrigazione=0;

	for(i=0;i<IRRIGAZIONESYSTEMS;i++)
	{
//		if(circuiti_attivi(i)>0)
		if((circuiti_attivi(i)>0)||
			(irrigazioneTable[i].wday_started!=-1))
			//può essere che è ancora attivo dal giorno prec
		{
			if(irrigazioneTable[i].wday_started!=-1)
				wday=irrigazioneTable[i].wday_started;

			if((irrigazioneTable[i].current_circuito<=0)&&	// non attivo
					(irrigazioneTable[i].attivo==0)&&	// non forzato
					(irrigazioneTable[i].day!=day)&&	// non ancora eseguito 
					(minutes-irrigazioneTable[i].ora_start>=0))	// passato ora_start
			{
				if(pioggia==0)	// non piove
					irrigazione_attiva(i,irrigazioneTable[i].msg_avvio_automativo);
				else // piove, impedisco che parta irrigazione
				{
					if(pioggia==1) // piove
					{
						irrigazioneTable[i].day=day;
						handleMessage(3,0,irrigazioneTable[i].id_irrigazione,0,
							irrigazioneTable[i].msg_pioggia_noinizio,0,1);
					}
				}
			}
			else
			{
				if((pioggia==1)&&(irrigazioneTable[i].attivo==0)&&(irrigazioneTable[i].current_circuito>0))
					irrigazione_disattiva(i,irrigazioneTable[i].msg_arresto_pioggia);
			}

			if(irrigazioneTable[i].current_circuito>0) // significa che in qualche modo (manuale o automatico) irrigazione è avviata
			{
				if(irrigazioneTable[i].current_circuito<=irrigazioneTable[i].num_circuiti)
				{
//					my_printf("%ld\n",irrigazioneTable[i].current_time-(seconds-irrigazioneTable[i].ora_started));

					id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito);
					j=id_digitalOutToId(irrigazioneCircuitiTable[id_circuito].id_digital_out);
					if(j!=-1)
					{
						if((irrigazioneTable[i].current_time-(seconds-irrigazioneTable[i].ora_started))>0) //circuito ancora attivo
						{
							if(digitalOutTable[j].value!=digitalOutTable[j].on_value)
							{
/*									formatMessage(msg.mtext,3,id_circuito,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,"Circuito attivato");
								msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);				
								storeEvent(2,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,id_circuito,"Circuito attivato");*/
							}
							irrigazione_attiva_pompa(i);	// imposto uscita a 1 anche se già lo è per essere certo che sia attiva
							digitalOutTable[j].value=digitalOutTable[j].on_value;
							digitalOutTable[j].req_value=digitalOutTable[j].on_value;
							digitalOutTable[j].po_delay=0;
						}
						else 	//circuito non più attivo
						{
/*								formatMessage(msg.mtext,3,id_circuito,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,"Circuito disattivato");
							msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);											
							storeEvent(2,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,id_circuito,"Circuito disattivato");*/
							digitalOutTable[j].value=1-digitalOutTable[j].on_value;
							digitalOutTable[j].req_value=1-digitalOutTable[j].on_value;
							digitalOutTable[j].po_delay=0;
							irrigazioneTable[i].ora_started=seconds;
							irrigazioneTable[i].current_circuito++;
							if(irrigazioneTable[i].current_circuito>irrigazioneTable[i].num_circuiti)
							{
								irrigazione_disattiva_pompa(i);	// imposto uscita pompa a 0
								irrigazioneTable[i].current_time=60*irrigazioneTable[i].tempo_off;
							}
							else
							{
								id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito);
								if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
									irrigazioneTable[i].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
								else
									irrigazioneTable[i].current_time=0;
							}
						}
					}
					else //id_digital_out non trovato
					{
						irrigazioneTable[i].current_circuito++;
						irrigazioneTable[i].ora_started=seconds;
						if(irrigazioneTable[i].current_circuito>irrigazioneTable[i].num_circuiti)
							irrigazioneTable[i].current_time=60*irrigazioneTable[i].tempo_off;
						else
						{
							id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito);
							my_printf("circuito %d %d %d\n",id_circuito,i,irrigazioneTable[i].current_circuito);
							if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
								irrigazioneTable[i].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
							else
								irrigazioneTable[i].current_time=0;
						}
					}
				}
				else 	// fine circuiti, attendo tempo_off oppure termino
				{
					irrigazione_disattiva_pompa(i);
/*					if(irrigazioneTable[i].current_circuito==1+irrigazioneTable[i].num_circuiti)
					{
						formatMessage(msg.mtext,3,0,irrigazioneTable[i].id_irrigazione,0,"Fine circuiti");
						msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);				
						storeEvent(1,irrigazioneTable[i].id_irrigazione,0,0,"Fine circuiti");
						irrigazioneTable[i].current_circuito++;
					}*/
					if(irrigazioneTable[i].count>=irrigazioneTable[i].ripetitivita)	// non ripeto più
						irrigazione_disattiva(i,irrigazioneTable[i].msg_arresto_automatico);
					else	//ripeto
					{
						if((irrigazioneTable[i].current_time-(seconds-irrigazioneTable[i].ora_started))<0) // passato tempo off
						{
							irrigazioneTable[i].count++;
							irrigazioneTable[i].ora_started=seconds;
							irrigazioneTable[i].current_circuito=1;
							id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,1);
							irrigazioneTable[i].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
						}
					}
				}
			}
		}
		
	}

}
