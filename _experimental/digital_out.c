#include "digital_out.h"

void storeDigitalOutTable(int device_num)
{
	int i;
	MYSQL *conn=mysqlConnect();
	if(!conn)
		return;

	for(i=0;i<DIGITALOUTCHANNELS;i++)
	{
		if(((device_num==0)||(digitalOutTable[i].device_num==device_num))
			&&(systems[systemNumToId(digitalOutTable[i].device_num,NUMSYSTEMS)].enabled)
			&&(digitalOutTable[i].id_digital_out!=-1)
			&&(digitalOutTable[i].value_valid)
			&&(time(NULL)-digitalOutTable[i].stored_time>=digitalOutTable[i].record_data_time))
		{
			storeDigitalOutLine(i,conn);
		}
	}
	mysql_close(conn);
}

int storeDigitalOutLine(int i,MYSQL *conn)
{
	char query[255];

	sprintf(query,"call insertHistory(%d,%d,%d,%d,%d,'')",
		DIGITAL_OUT,
		digitalOutTable[i].device_num,
		digitalOutTable[i].id_digital_out,
		digitalOutTable[i].value,
		digitalOutTable[i].value);
	if(mysql_query(conn,query)!=0)
	{
		my_printf("%s\n%s\n",query,mysql_error(conn));
		return -1;
	}
	else
	{
		digitalOutTable[i].stored_time=time(NULL);
		return 0;
	}
}

void initializeDigitalOutTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella digitalOut
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<DIGITALOUTCHANNELS;i++)
	{
		initializeDigitalOutLine(i);
//		resetDigitalOutValues(i);
	}
}

void initializeDigitalOutLine(int i)
{
	digitalOutTable[i].id_digital_out=-1;
	strcpy(digitalOutTable[i].description,"");
	digitalOutTable[i].device_num=-1;
	digitalOutTable[i].ch_num=-1;
	digitalOutTable[i].def=-1;
	digitalOutTable[i].po_delay=-1;
	digitalOutTable[i].on_value=-1;
	digitalOutTable[i].on_time=-1;
	digitalOutTable[i].pon_time=-1;
	digitalOutTable[i].poff_time=-1;

	resetDigitalOutValues(i);
}

void updateDigitalOutChannel(int fd,int id_digital_out)
{
	int state;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i,j;
	int systemId;
	char buffer[255];
	char ok[3];
	MYSQL *conn=mysqlConnect();

	sprintf(query,"SELECT description,device_num,ch_num,`default`,po_delay,on_time,pon_time,poff_time,"
						"record_data_time "
						"FROM digital_out WHERE id_digital_out=%d",id_digital_out);

	state = mysql_query(conn,query);
	my_printf("%s %d\n",query,state);

	if( state != 0 )
		my_printf("%s\n",mysql_error(conn));
	result = mysql_store_result(conn);
	
	strcpy(ok,"ko");
	if (( row = mysql_fetch_row(result)) != NULL )
	{
		j=id_digitalOutToId(id_digital_out);//source index
		systemId=systemNumToId(atoi(row[1]),NUMSYSTEMS);

		if(atoi(row[2])>0)
			i=deviceToid[2][systemId][atoi(row[2])-1];//dest index
		else 
			i=-1;
			
			
		if(i!=-1) //destination index exists
		{
			digitalOutTable[i].id_digital_out=id_digital_out;
			if(row[0])
				safecpy(digitalOutTable[i].description,row[0]);
			else
				safecpy(digitalOutTable[i].description,"");
			digitalOutTable[i].device_num=atoi(row[1]);
			digitalOutTable[i].ch_num=atoi(row[2]);
			digitalOutTable[i].def=atoi(row[3]);
			digitalOutTable[i].po_delay=atoi(row[4]);
			digitalOutTable[i].on_time=atoi(row[5]);
			digitalOutTable[i].pon_time=atoi(row[6]);
			digitalOutTable[i].poff_time=atoi(row[7]);
			digitalOutTable[i].record_data_time=(atoi(row[8])!=-1?60*atoi(row[8]):RECORDDATATIME);

			if(j!=-1) //source index exists 
			{
				if(systems[systemId].sockfd!=-1)	//dest device active
				{
					digitalOutTable[i].start_time=digitalOutTable[j].start_time;
					digitalOutTable[i].value_valid=digitalOutTable[j].value_valid;
					digitalOutTable[i].value=digitalOutTable[j].value;
					digitalOutTable[i].value_real=digitalOutTable[j].value_real;
					digitalOutTable[i].def=digitalOutTable[j].def;
					digitalOutTable[i].po_delay=digitalOutTable[j].po_delay;
					digitalOutTable[i].on_time=digitalOutTable[j].on_time;
					digitalOutTable[i].pon_time=digitalOutTable[j].pon_time;
					digitalOutTable[i].poff_time=digitalOutTable[j].poff_time;
					digitalOutTable[i].req_value=digitalOutTable[j].req_value;
					digitalOutTable[i].record_data_time=digitalOutTable[j].record_data_time;
				}
				else //dest device not active
					initializeDigitalOutLine(i);

				if(i!=j)
				{
					digitalOutTable[j].id_digital_out=-1;
					initializeDigitalOutLine(j);
				}
			}
			else //source index does not exist
			{	
				resetDigitalOutValues(i);
				if(systems[systemId].sockfd!=-1)
					setSingleOutputDefault(i);
			}

			my_printf("%d %s %d %d\n",digitalOutTable[i].id_digital_out,digitalOutTable[i].description,
						digitalOutTable[i].device_num,digitalOutTable[i].ch_num);
		}
		else //canale di destinazione=0
		{
			if(j!=-1) //era associato
				initializeDigitalOutLine(j);
		}
		strcpy(ok,"ok");
	}
	mysql_free_result(result);	
	mysql_close(conn);
	sprintf(buffer,"%s %s",ok,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		my_perror("send");
}

int id_digitalOutToId(int id_digital_out)
{
	int i;
	
	for(i=0;i<DIGITALOUTCHANNELS;i++)
		if(digitalOutTable[i].id_digital_out==id_digital_out)
			break;
	return((i<DIGITALOUTCHANNELS)?i:-1);
}

int loadDigitalOutTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	MYSQL *conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return (1);
	}

	if(!onlyupdate)
	{
		if(DIGITALOUTCHANNELS>0)
		{
			my_printf("get_shared_memory_segment: digitalOutTable\n");
			digitalOutTable=(struct digitalOutLine *)get_shared_memory_segment(DIGITALOUTCHANNELS * sizeof(struct digitalOutLine), &SHMDIGITALOUTID, "/dev/zero");
			if(!digitalOutTable)
				die("digitalOutTable - get_shared_memory_segment\n");
		}
		else
			digitalOutTable=0;
	}
	initializeDigitalOutTable();
	state = mysql_query(conn, "SELECT digital_out.id_digital_out,"
							"digital_out.description,"
							"digital_out.device_num,"
							"digital_out.ch_num,"
							"digital_out.`default`,"
							"digital_out.po_delay,"
							"digital_out.on_value,"
							"digital_out.on_time,"
							"digital_out.pon_time,digital_out.poff_time,"
							"digital_out.printer,digital_out.msg_l,digital_out.msg_h,"
							"digital_out.sms,digital_out.msg_is_event,digital_out.record_data_time "
						"FROM digital_out JOIN system ON digital_out.device_num=system.device_num "
						"WHERE system.removed=0 AND digital_out.ch_num!=0"
						" ORDER BY digital_out.device_num,digital_out.ch_num");

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		i=systemNumToId(atoi(row[2]),NUMSYSTEMS);
		if((i!=-1) && (atoi(row[3])<=systems[i].out_ch_d))
		{
			if(!onlyupdate)
				deviceToid[2][i][atoi(row[3]) - 1]=idRow;
			else
				idRow=deviceToid[2][i][atoi(row[3]) - 1];

			digitalOutTable[idRow].id_digital_out=atoi(row[0]);
			if(row[1])
				safecpy(digitalOutTable[idRow].description,row[1]);
			else
				safecpy(digitalOutTable[idRow].description,"");
			digitalOutTable[idRow].device_num=atoi(row[2]);
			digitalOutTable[idRow].ch_num=atoi(row[3]);
			digitalOutTable[idRow].def=atoi(row[4]);
			digitalOutTable[idRow].po_delay=atoi(row[5]);
			digitalOutTable[idRow].on_time=atoi(row[7]);
			digitalOutTable[idRow].pon_time=atoi(row[8]);
			digitalOutTable[idRow].poff_time=atoi(row[9]);
			digitalOutTable[idRow].printer=atoi(row[10]);
			if(row[11])
				safecpy(digitalOutTable[idRow].msg_l,row[11]);
			else
				safecpy(digitalOutTable[idRow].msg_l,"");
			if(row[12])
				safecpy(digitalOutTable[idRow].msg_h,row[12]);
			else
				safecpy(digitalOutTable[idRow].msg_h,"");
			digitalOutTable[idRow].req_value=digitalOutTable[idRow].def;
			digitalOutTable[idRow].value_real=0;

// non setto il valore di default, lo faccio dopo la connessione

/*
			if((digitalOutTable[idRow].po_delay > 0)
				&&(digitalOutTable[idRow].def==digitalOutTable[idRow].on_value))
			{
				digitalOutTable[idRow].value=1-digitalOutTable[idRow].req_value;
				digitalOutTable[idRow].start_time=time(NULL);
			}
			else
				digitalOutTable[idRow].value=digitalOutTable[idRow].def;
*/
			digitalOutTable[idRow].on_value=atoi(row[6]);
			digitalOutTable[idRow].sms=atoi(row[13]);
			digitalOutTable[idRow].msg_is_event=atoi(row[14]);
			digitalOutTable[idRow].record_data_time=(atoi(row[15])!=-1?60*atoi(row[15]):RECORDDATATIME);
			idRow++;
		}
	}
	mysql_free_result(result);
	if(!onlyupdate)
	{
		for(i=0;i<NUMSYSTEMS;i++)
			for(k=0;k<systems[i].out_ch_d;k++)
				if(deviceToid[2][i][k]==-1)
				{
					deviceToid[2][i][k]=idRow;
					idRow++;
				}
	
		if(idRow!=DIGITALOUTCHANNELS)
			my_printf("there's a problem with digital out channels\n");
	}
	mysql_close(conn);
	return(0);
}

void resetDeviceDigitalOut(int systemId)
{
	int j,id;
	for(j=0;j<systems[systemId].out_ch_d;j++)
	{
		id=deviceToid[2][systemId][j];
		resetDigitalOutValues(id);
	}
}

void resetDigitalOutValues(int i)
{
	digitalOutTable[i].value=-1;
	digitalOutTable[i].value_valid=FALSE;
	digitalOutTable[i].stored_time=time(NULL);
	digitalOutTable[i].value_real=-1;
	digitalOutTable[i].req_value=-1;
}

void setSingleOutputDefault(int idRow)
{
/*--------------------------------------------
 * imposta valori di default su id idRow
 * int idRow - identificativo canale [0-DIGITALOUTCHANNELS-1]
 * -----------------------------------------*/
	if((digitalOutTable[idRow].id_digital_out!=-1)
			&&(digitalOutTable[idRow].value==-1))
	{
		digitalOutTable[idRow].req_value=digitalOutTable[idRow].def;
		if(digitalOutTable[idRow].req_value==digitalOutTable[idRow].on_value)
		{
			digitalOutTable[idRow].start_time=time(NULL);
			if(digitalOutTable[idRow].po_delay>0)
				digitalOutTable[idRow].value=1-digitalOutTable[idRow].def;
			else
				digitalOutTable[idRow].value=digitalOutTable[idRow].def;
		}
		else
		{
			digitalOutTable[idRow].start_time=time(NULL);
			digitalOutTable[idRow].value=digitalOutTable[idRow].req_value;
		}
		digitalOutTable[idRow].value_valid=TRUE;

	}
}


void setOutputDefault(int systemId)
{
/*--------------------------------------------
 * imposta valori di default su sistema systemId
 * chiamata dopo connessione
 * int systemId - identificativo device [0-NUMSYSTEMS-1]
 * -----------------------------------------*/

	int i;
	int idRow;

	for(i=0;i<systems[systemId].out_ch_d;i++)
	{
		idRow=deviceToid[2][systemId][i];
		setSingleOutputDefault(idRow);
	}
}

int setOutput(int id,int value)
{
	int i;
	i=id_digitalOutToId(id);
	
	if(i==-1)
		return -1;

	digitalOutTable[i].req_value=value;
	digitalOutTable[i].start_time=time(NULL);

	if(digitalOutTable[i].po_delay==0)
	{
		digitalOutTable[i].value=value;
		digitalOutTable[i].value_valid=TRUE;
	}
	
	return 1;
}

int activateOutput(int id)
{
	int i;
	i=id_digitalOutToId(id);
	
	if(i==-1)
		return -1;

	if(digitalOutTable[i].req_value!=digitalOutTable[i].on_value)
	{
		digitalOutTable[i].req_value=digitalOutTable[i].on_value;
		digitalOutTable[i].start_time=time(NULL);
	}

	if(digitalOutTable[i].po_delay==0)
	{
		digitalOutTable[i].value=digitalOutTable[i].on_value;
		digitalOutTable[i].value_valid=TRUE;
	}
	return 1;
}

int deactivateOutput(int id)
{
	int i;
	i=id_digitalOutToId(id);
	
	if(i==-1)
		return -1;

	if(digitalOutTable[i].req_value!=(1-digitalOutTable[i].on_value))
	{
		digitalOutTable[i].req_value=1-digitalOutTable[i].on_value;
		digitalOutTable[i].start_time=time(NULL);
	}
	if(digitalOutTable[i].po_delay==0)
	{
		digitalOutTable[i].value=1-digitalOutTable[i].on_value;
		digitalOutTable[i].value_valid=TRUE;
	}
	return 1;
}

void activateOutputImmediate(int id)
{
	digitalOutTable[id].value_valid=TRUE;
	digitalOutTable[id].value=digitalOutTable[id].on_value;
	digitalOutTable[id].req_value=digitalOutTable[id].on_value;
	digitalOutTable[id].start_time=time(NULL);
	digitalOutTable[id].po_delay=0;
}


void deactivateOutputImmediate(int id)
{
	digitalOutTable[id].value_valid=TRUE;
	digitalOutTable[id].value=1-digitalOutTable[id].on_value;
	digitalOutTable[id].req_value=1-digitalOutTable[id].on_value;
	digitalOutTable[id].start_time=time(NULL);
	digitalOutTable[id].po_delay=0;
}


void getDigitalOutTimings(struct digitalOutLine *digital_out_line,
		int *po_delay_rem,int *pon_time_rem,int *on_time_rem,int *poff_time_rem)
{
	time_t now,elapsed;
	*po_delay_rem=digital_out_line->po_delay;
	*pon_time_rem=digital_out_line->pon_time;
	*on_time_rem=digital_out_line->on_time;
	*poff_time_rem=digital_out_line->poff_time;

	now=time(NULL);
	elapsed=now-digital_out_line->start_time;
	if(digital_out_line->value_real!=digital_out_line->req_value)		//change requested
	{
		if((digital_out_line->po_delay>0)
				&&(digital_out_line->req_value==digital_out_line->on_value))
			*po_delay_rem=digital_out_line->po_delay-elapsed;
	}
	else 															//change not requested
	{
		if(digital_out_line->value_real==digital_out_line->on_value)		// on
		{
			*pon_time_rem=digital_out_line->pon_time-elapsed;
			*on_time_rem=digital_out_line->pon_time+digital_out_line->on_time-elapsed;
		}
		else 														// off
			*poff_time_rem=digital_out_line->poff_time-elapsed;
	}

	if(((*po_delay_rem)<0)||((*po_delay_rem)>digital_out_line->po_delay))
		*po_delay_rem=digital_out_line->po_delay;
	if(((*pon_time_rem)<0)||((*pon_time_rem)>digital_out_line->pon_time))
		*pon_time_rem=digital_out_line->pon_time;
	if(((*on_time_rem)<0)||((*on_time_rem)>digital_out_line->on_time))
		*on_time_rem=digital_out_line->on_time;
	if(((*poff_time_rem)<0)||((*poff_time_rem)>digital_out_line->poff_time))
		*poff_time_rem=digital_out_line->poff_time;
}
