#include "scenari.h"

void initializeScenariPresenzeTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella scenariPresenze
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<SCENARIPRESENZECOUNT;i++)
	{
		scenariPresenzeTable[i].id_digital_out=-1;
		resetScenariPresenzeValues(i);
	}
}

void initializeScenariBgBnTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella scenariPresenze
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<SCENARIBGBNCOUNT;i++)
	{
		scenariBgBnTable[i].id_digital_out=-1;
		resetScenariBgBnValues(i);
	}
}


void resetScenariPresenzeValues(int i)
{
	scenariPresenzeTable[i].attivo=0;
	scenariPresenzeTable[i].ciclico=0;
	scenariPresenzeTable[i].tempo_on=0;
	scenariPresenzeTable[i].tempo_off=0;
	scenariPresenzeTable[i].tempo_on=0;
	scenariPresenzeTable[i].tempo_off=0;
}

void resetScenariBgBnValues(int i)
{
	scenariBgBnTable[i].attivo=0;
	scenariBgBnTable[i].ritardo=0;
}

int id_digitalOutToId_scenariPresenze(int id_digital_out)
{
	int i;
	
	for(i=0;i<SCENARIPRESENZECOUNT;i++)
		if(scenariPresenzeTable[i].id_digital_out==id_digital_out)
			break;
	return((i<SCENARIPRESENZECOUNT)?i:-1);
}

int id_digitalOutToId_scenariBgBn(int id_digital_out,bool bg)
{
	int i;
	
	for(i=0;i<SCENARIBGBNCOUNT;i++)
		if((scenariBgBnTable[i].id_digital_out==id_digital_out) && (scenariBgBnTable[i].bg==bg))
			break;
	return((i<SCENARIBGBNCOUNT)?i:-1);
}

char checkBuonGiornoAttivo()
{
	int i,j;
	time_t inizio;
	int out=0;

	if(*buonGiornoAttivo)
	{
		inizio=time(NULL);
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				if(inizio - digitalOutTable[j].start_time <= digitalOutTable[j].po_delay+digitalOutTable[j].on_time)
					out++;
				else
					digitalOutTable[j].value=digitalOutTable[j].req_value;
				if(digitalOutTable[j].value_real != digitalOutTable[j].req_value)
					out++;
			}
		}
	}
	return (out>0?1:0);
}

char checkBuonaNotteAttivo()
{
	int i,j;
	time_t inizio;
	int out=0;

	if(*buonaNotteAttivo)
	{
		inizio=time(NULL);
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg==0)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				if(inizio - digitalOutTable[j].start_time <= digitalOutTable[j].po_delay+digitalOutTable[j].on_time)
					out++;
				else
					digitalOutTable[j].value=digitalOutTable[j].req_value;
				if(digitalOutTable[j].value_real != digitalOutTable[j].req_value)
					out++;
			}
		}
	}
	return (out>0?1:0);
}


int loadScenariBgBnTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	int out;
	MYSQL *conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return (1);
	}

	
	state = mysql_query(conn, "SELECT id_digital_out,attivo,ritardo,"
							"giorno "
							"FROM scenari_giorno_notte "
							"ORDER BY id");
	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);
	SCENARIBGBNCOUNT=mysql_num_rows(result);

	if(!onlyupdate)
	{
		if(SCENARIBGBNCOUNT)
		{
			my_printf("get_shared_memory_segment: scenariBgBnTable\n");
			scenariBgBnTable=(struct scenariBgBnLine *)get_shared_memory_segment(SCENARIBGBNCOUNT * sizeof(struct scenariPresenzeLine), &SHMSCENARIBGBNID, "/dev/zero");
			if(!scenariBgBnTable)
				die("scenariBgBnTable - get_shared_memory_segment\n");
		}
		else
			scenariBgBnTable=0;

		my_printf("get_shared_memory_segment: buonGiornoAttivo\n");
		buonGiornoAttivo=(char *)get_shared_memory_segment(1, &SHMSCENARIBGATTIVOID, "/dev/zero");
		if(!buonGiornoAttivo)
			die("buonGiornoAttivo - get_shared_memory_segment\n");

		my_printf("get_shared_memory_segment: buonaNotteAttivo\n");
		buonaNotteAttivo=(char *)get_shared_memory_segment(1, &SHMSCENARIBNATTIVOID, "/dev/zero");
		if(!buonaNotteAttivo)
			die("buonaNotteAttivo - get_shared_memory_segment\n");
	}
	initializeScenariBgBnTable();
	*buonGiornoAttivo=0;
	*buonaNotteAttivo=0;

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		scenariBgBnTable[idRow].id_digital_out=atoi(row[0]);
		scenariBgBnTable[idRow].attivo=atoi(row[1]);
		scenariBgBnTable[idRow].ritardo=atoi(row[2]);
		scenariBgBnTable[idRow].bg=atoi(row[3]);
		idRow++;
	}
	mysql_free_result(result);
	mysql_close(conn);
	return(0);
}

void updateDayNight(int fd)
{
	char buffer[255];
	char ok[3];

	if(loadScenariBgBnTable(1))
		strcpy(ok,"ko");
	else
		strcpy(ok,"ok");

	sprintf(buffer,"%s %s",ok,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		my_perror("send");
}

void updatePresenze(int fd)
{
	char buffer[255];
	char ok[3];

	if(loadScenariPresenzeTable(1))
		strcpy(ok,"ko");
	else
		strcpy(ok,"ok");

	sprintf(buffer,"%s %s",ok,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		my_perror("send");
}

int loadScenariPresenzeTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	int out;
	char query[255];
	MYSQL *conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return (1);
	}


	strcpy(query,"SELECT id_digital_out,attivo,ciclico,"
							"tempo_on,tempo_off,ora_ini,ora_fine "
							"FROM scenari_presenze "
							"ORDER BY id");
	state = mysql_query(conn, query);
	if( state != 0 )
	{
		my_printf("%s - %s\n",query,mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);
	SCENARIPRESENZECOUNT=mysql_num_rows(result);

	if(!onlyupdate)
	{
		if(SCENARIPRESENZECOUNT>0)
		{
			my_printf("get_shared_memory_segment: scenariPresenzeTable\n");
			scenariPresenzeTable=(struct scenariPresenzeLine *)get_shared_memory_segment(SCENARIPRESENZECOUNT * sizeof(struct scenariPresenzeLine), &SHMSCENARIPRESENZEID, "/dev/zero");
			if(!scenariPresenzeTable)
				die("scenariPresenzeTable - get_shared_memory_segment\n");
		}
		else
			scenariPresenzeTable=0;

		my_printf("get_shared_memory_segment: scenariPresenzeAttivo\n");
		scenariPresenzeAttivo=(char *)get_shared_memory_segment(2, &SHMSCENARIPRESENZEATTIVOID, "/dev/zero");
		if(!scenariPresenzeAttivo)
			die("scenariPresenzeAttivo - get_shared_memory_segment\n");

	}
	initializeScenariPresenzeTable();
	*scenariPresenzeAttivo=0;

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		scenariPresenzeTable[idRow].id_digital_out=atoi(row[0]);
		scenariPresenzeTable[idRow].attivo=atoi(row[1]);
		scenariPresenzeTable[idRow].ciclico=atoi(row[2]);
		scenariPresenzeTable[idRow].tempo_on=atoi(row[3]);
		scenariPresenzeTable[idRow].tempo_off=atoi(row[4]);
		scenariPresenzeTable[idRow].ora_ini=h2i(row[5]);
		scenariPresenzeTable[idRow].ora_fine=h2i(row[6]);
		idRow++;

	}
	mysql_free_result(result);
	mysql_close(conn);
	return(0);
}

void doScenariPresenze()
{
	int id_pres;
	int id;
	int id_digital_out;
	int ora_ini;
	int ora_fine;
	int tempo_on;
	int tempo_off;
	int minutes;
	struct digitalOutLine *digital_out_line;
	time_t curtime;
	struct timeval tv;
	struct tm ts;

	// metto simulazione presenze
	if(*scenariPresenzeAttivo==1)
	{
		gettimeofday(&tv, NULL); 
		curtime=tv.tv_sec;
		ts=*localtime(&curtime);
		minutes=ts.tm_hour*60+ts.tm_min;

		for(id_pres=0;id_pres<SCENARIPRESENZECOUNT;id_pres++)
		{
			id_digital_out=scenariPresenzeTable[id_pres].id_digital_out;
			if(id_digital_out!=-1)
			{
				id=id_digitalOutToId(id_digital_out);
				if((id!=-1)&&(scenariPresenzeTable[id_pres].attivo))
				{
					digital_out_line=&digitalOutTable[id];
					ora_ini=scenariPresenzeTable[id_pres].ora_ini;
					ora_fine=scenariPresenzeTable[id_pres].ora_fine;
		
					if((((ora_fine-minutes+1440)%1440) < ((ora_fine-ora_ini+1440)%1440))
							&&(((minutes-ora_ini+1440)%1440) <= ((ora_fine-ora_ini+1440)%1440)))
					{
						tempo_on=scenariPresenzeTable[id_pres].tempo_on;
						tempo_off=scenariPresenzeTable[id_pres].tempo_off;
						if(scenariPresenzeTable[id_pres].ciclico)
						{
							if((minutes - ora_ini) % (tempo_on + tempo_off) < tempo_on)
								digital_out_line->value=digital_out_line->on_value;
							else
								digital_out_line->value=1-digital_out_line->on_value;
						}
						else
							digital_out_line->value=digital_out_line->on_value;
					}
					else
						digital_out_line->value=1-digital_out_line->on_value;
				}
			}
		}
	}
}


void bg_attiva()
{
	register int i,j;

	*buonaNotteAttivo=0;
	for(i=0;i<SCENARIBGBNCOUNT;i++)
	{
		if((scenariBgBnTable[i].bg==0)&&(scenariBgBnTable[i].attivo))
		{
			j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
			digitalOutTable[j].start_time=0;
			digitalOutTable[j].req_value=digitalOutTable[j].value_real;
		}
	}

	*buonGiornoAttivo=1;
	for(i=0;i<SCENARIBGBNCOUNT;i++)
	{
		if((scenariBgBnTable[i].bg)&&(scenariBgBnTable[i].attivo))
		{
			j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
			digitalOutTable[j].start_time=time(NULL);
			digitalOutTable[j].req_value=digitalOutTable[j].on_value;
			digitalOutTable[j].po_delay=scenariBgBnTable[i].ritardo;
		}
	}
}

void bn_attiva()
{
	register int i,j;

	*buonGiornoAttivo=0;
	for(i=0;i<SCENARIBGBNCOUNT;i++)
	{
		if((scenariBgBnTable[i].bg)&&(scenariBgBnTable[i].attivo))
		{
			j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
			digitalOutTable[j].start_time=0;
			digitalOutTable[j].req_value=digitalOutTable[j].value_real;
		}
	}

	*buonaNotteAttivo=1;
	for(i=0;i<SCENARIBGBNCOUNT;i++)
	{
		if((scenariBgBnTable[i].bg==0)&&(scenariBgBnTable[i].attivo))
		{
			j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
			digitalOutTable[j].start_time=time(NULL);
			digitalOutTable[j].req_value=1-digitalOutTable[j].on_value;
			digitalOutTable[j].po_delay=scenariBgBnTable[i].ritardo;
		}
	}
}

void bg_disattiva()
{
	register int i,j;

	*buonGiornoAttivo=0;
	for(i=0;i<SCENARIBGBNCOUNT;i++)
	{
		if((scenariBgBnTable[i].bg)&&(scenariBgBnTable[i].attivo))
		{
			j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
			digitalOutTable[j].start_time=0;
			digitalOutTable[j].req_value=digitalOutTable[j].value_real;
		}
	}
}

void bn_disattiva()
{
	register int i,j;

	*buonaNotteAttivo=0;
	for(i=0;i<SCENARIBGBNCOUNT;i++)
	{
		if((scenariBgBnTable[i].bg==0)&&(scenariBgBnTable[i].attivo))
		{
			j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
			digitalOutTable[j].start_time=0;
			digitalOutTable[j].req_value=digitalOutTable[j].value_real;
		}
	}
}
