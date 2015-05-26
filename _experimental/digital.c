#include "digital.h"


void storeDigitalTable(int device_num)
{
	int i;
	MYSQL *conn=mysqlConnect();
	if(!conn)
		return;

	for(i=0;i<DIGITALCHANNELS;i++)
	{
		if(((device_num==0)||(digitalTable[i].device_num==device_num))
			&&(systems[systemNumToId(digitalTable[i].device_num,NUMSYSTEMS)].enabled)
			&&(digitalTable[i].enabled)
			&&(digitalTable[i].id_digital!=-1)
			&&(digitalTable[i].value_valid)
			&&(time(NULL)-digitalTable[i].stored_time>=digitalTable[i].record_data_time))
		{
			storeDigitalLine(i,conn);
			usleep(1000);
		}
	}
	mysql_close(conn);
}


int storeDigitalLine(int i,MYSQL *conn)
{
	char query[255];

	sprintf(query,"call insertHistory(%d,%d,%d,%d,%d,'')",
		DIGITAL,
		digitalTable[i].device_num,
		digitalTable[i].id_digital,
		digitalTable[i].value,
		digitalTable[i].value);
	if(mysql_query(conn,query)!=0)
	{
		my_printf("%s\n%s\n",query,mysql_error(conn));
		return -1;
	}
	else
	{
		digitalTable[i].stored_time=time(NULL);
		return 0;
	}
}
void initializeDigitalTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella digital
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<DIGITALCHANNELS;i++)
	{
		digitalTable[i].id_digital=-1;
		strcpy(digitalTable[i].form_label,"");
		strcpy(digitalTable[i].description,"");
		strcpy(digitalTable[i].label,"");
		strcpy(digitalTable[i].sinottico,"");
		digitalTable[i].device_num=-1;
		digitalTable[i].ch_num=-1;
		digitalTable[i].printer=0;
		digitalTable[i].sms=0;
		digitalTable[i].time_delay_on=0;
		digitalTable[i].time_delay_off=0;
		digitalTable[i].alarm_value=-1;
		digitalTable[i].on_value=-1;
		strcpy(digitalTable[i].msg_on,"");
		strcpy(digitalTable[i].msg_off,"");
		digitalTable[i].msg_is_event=0;
		digitalTable[i].enabled=0;

		resetDigitalValues(i);
	}
}

void updateDigitalChannel(int id_digital)
{
	int state;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i,j;
	int deviceId;
	MYSQL *conn=mysqlConnect();

	sprintf(query,"SELECT form_label,description,label,"
							"sinottico,printer,"
							"time_delay_on,time_delay_off,"
							"alarm_value,msg_on,msg_off,enabled,"
							"device_num,ch_num,sms,msg_is_event,on_value,record_data_time "
						"FROM digital "
						"WHERE id_digital=%d",id_digital);

	state = mysql_query(conn,query);

	if( state != 0 )
		my_printf("%s\n",mysql_error(conn));
	result = mysql_store_result(conn);
	

	if (( row = mysql_fetch_row(result)) != NULL )
	{
		j=id_digitalToId(id_digital);//source

		deviceId=systemNumToId(atoi(row[10]),NUMSYSTEMS);
		if(atoi(row[11])>0)
			i=deviceToid[1][deviceId][atoi(row[11])-1];//dest
		else 
			i=-1;
			
			
		if(i!=-1) //destination index exists
		{
			digitalTable[i].id_digital=id_digital;
			if(j!=-1) //source index exists
			{
				if(systems[deviceId].sockfd!=-1)	//dest device active
				{
					digitalTable[i].value_valid=digitalTable[j].value_valid;
					digitalTable[i].value=digitalTable[j].value;
					digitalTable[i].value1=digitalTable[j].value1;
					digitalTable[i].value2=digitalTable[j].value2;
					digitalTable[i].value3=digitalTable[j].value3;
					digitalTable[i].value4=digitalTable[j].value4;
					digitalTable[i].time_delay_on_cur=digitalTable[j].time_delay_on_cur;
					digitalTable[i].time_delay_off_cur=digitalTable[j].time_delay_off_cur;
					digitalTable[i].record_data_time=digitalTable[j].record_data_time;
				}
				else	//dest device not active
					resetDigitalValues(i);

				if(j!=i)
				{
					digitalTable[j].id_digital=-1;
					digitalTable[j].enabled=0;
					resetDigitalValues(j);
				}
			}	//source index does not exist
			else 
				resetDigitalValues(i);
				
			
			if(row[0])
				safecpy(digitalTable[i].form_label,row[0]);
			else
				strcpy(digitalTable[i].form_label,"");
			if(row[1])
				safecpy(digitalTable[i].description,row[1]);
			else
				strcpy(digitalTable[i].description,"");
			if(row[2])
				safecpy(digitalTable[i].label,row[2]);
			else
				strcpy(digitalTable[i].label,"");
			if(row[3])
				safecpy(digitalTable[i].sinottico,row[3]);
			else
				strcpy(digitalTable[i].sinottico,"");
	
			digitalTable[i].printer=atoi(row[4]);
			digitalTable[i].time_delay_on=atoi(row[5]);
			digitalTable[i].time_delay_off=atoi(row[6]);
			digitalTable[i].alarm_value=atoi(row[7]);
			if(row[8])
				safecpy(digitalTable[i].msg_on,row[8]);
			else
				strcpy(digitalTable[i].msg_on,"");
			if(row[9])
				safecpy(digitalTable[i].msg_off,row[9]);
			else
				strcpy(digitalTable[i].msg_off,"");
			digitalTable[i].enabled=atoi(row[10]);
			digitalTable[i].device_num=atoi(row[11]);
			digitalTable[i].ch_num=atoi(row[12]);
			digitalTable[i].sms=atoi(row[13]);
			digitalTable[i].msg_is_event=atoi(row[14]);
			digitalTable[i].on_value=atoi(row[15]);
			digitalTable[i].record_data_time=(atoi(row[16])!=-1?60*atoi(row[16]):RECORDDATATIME);
		}
		else //canale di destinazione=0
		{
			if(j!=-1) //era associato
			{
				digitalTable[j].id_digital=-1;
				digitalTable[j].enabled=0;
				resetDigitalValues(j);
			}
		}
	}	
	mysql_free_result(result);
	mysql_close(conn);
}

int id_digitalToId(int id_digital)
{
	int i;
	
	for(i=0;i<DIGITALCHANNELS;i++)
		if(digitalTable[i].id_digital==id_digital)
			break;
	return((i<DIGITALCHANNELS)?i:-1);
}

int loadDigitalTable(bool onlyupdate)
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
		return (-1);
	}

	if(!onlyupdate)
	{
		if(DIGITALCHANNELS>0)
		{
			my_printf("get_shared_memory_segment: digitalTable\n");
			digitalTable=(struct digitalLine *)get_shared_memory_segment(DIGITALCHANNELS * sizeof(struct digitalLine), &SHMDIGITALID, "/dev/zero");
			if(!digitalTable)
				die("digitalTable - get_shared_memory_segment\n");
		}
		else
			digitalTable=0;
		my_printf("get_shared_memory_segment: id_digital_pioggia\n");
		id_digital_pioggia=(int *)get_shared_memory_segment(sizeof(int), &SHMPIOGGIA, "/dev/zero");
		if(!id_digital_pioggia)
			die("id_digital_pioggia - get_shared_memory_segment\n");
	}

	initializeDigitalTable();
	
	state = mysql_query(conn, "SELECT digital.id_digital,digital.form_label,"
							"digital.description,digital.label,digital.sinottico,"
							"digital.device_num,digital.ch_num,digital.printer,"
							"digital.time_delay_on,digital.time_delay_off,"
							"digital.alarm_value,digital.msg_on,digital.msg_off,"
							"digital.enabled,digital.sms,digital.msg_is_event,on_value, "
							"digital.record_data_time "
						"FROM digital JOIN system ON digital.device_num=system.device_num "
						"WHERE system.removed=0 AND digital.ch_num!=0"
						" ORDER BY digital.device_num, digital.ch_num");
	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);

	*id_digital_pioggia=-1;


	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		i=systemNumToId(atoi(row[5]),NUMSYSTEMS);
		if((i!=-1) && (atoi(row[6])<=systems[i].in_ch_d))
		{
			if(!onlyupdate)
				deviceToid[1][i][atoi(row[6]) - 1]=idRow;
			else
				idRow=deviceToid[1][i][atoi(row[6]) - 1];

			digitalTable[idRow].id_digital=atoi(row[0]);
			if(row[1])
				safecpy(digitalTable[idRow].form_label,row[1]);
			else
				safecpy(digitalTable[idRow].form_label,"");
			if(row[2])
				safecpy(digitalTable[idRow].description,row[2]);
			else
				safecpy(digitalTable[idRow].description,"");
			if(row[3])
				safecpy(digitalTable[idRow].label,row[3]);
			else
				safecpy(digitalTable[idRow].label,"");
			if(row[4])
				safecpy(digitalTable[idRow].sinottico,row[4]);
			else
				safecpy(digitalTable[idRow].sinottico,"");
			digitalTable[idRow].device_num=atoi(row[5]);
			digitalTable[idRow].ch_num=atoi(row[6]);

			digitalTable[idRow].printer=atoi(row[7]);

			digitalTable[idRow].time_delay_on=atoi(row[8]);
			digitalTable[idRow].time_delay_off=atoi(row[9]);
			digitalTable[idRow].alarm_value=atoi(row[10]);
			if(row[11])
				safecpy(digitalTable[idRow].msg_on,row[11]);
			else
				safecpy(digitalTable[idRow].msg_on,"");
			if(row[12])
				safecpy(digitalTable[idRow].msg_off,row[12]);
			else
				safecpy(digitalTable[idRow].msg_off,"");
			digitalTable[idRow].enabled=atoi(row[13]);
			digitalTable[idRow].sms=atoi(row[14]);
			digitalTable[idRow].msg_is_event=atoi(row[15]);
			digitalTable[idRow].on_value=atoi(row[16]);
			digitalTable[idRow].record_data_time=(atoi(row[17])!=-1?60*atoi(row[17]):RECORDDATATIME);
			if(strstr(strtoupper(digitalTable[idRow].label),"PIOGGIA"))
				*id_digital_pioggia=idRow;
			idRow++;
		}
	}
	mysql_free_result(result);

	if(!onlyupdate)
	{
		for(i=0;i<NUMSYSTEMS;i++)
			for(k=0;k<systems[i].in_ch_d;k++)
				if(deviceToid[1][i][k]==-1)
				{
					deviceToid[1][i][k]=idRow;
					idRow++;
				}
	
		if(idRow!=DIGITALCHANNELS)
			my_printf("there's a problem with digital channels\n");
	}
	mysql_close(conn);
	return(0);
}



void resetDigitalValues(int i)
{
	digitalTable[i].currently_on=0;
	digitalTable[i].time_delay_on_cur=0;
	digitalTable[i].time_delay_off_cur=0;
	digitalTable[i].value_valid=TRUE;
	digitalTable[i].record_data_time=RECORDDATATIME;
	digitalTable[i].stored_time=time(NULL);
	digitalTable[i].value=-1;
	digitalTable[i].value1=-1;
	digitalTable[i].value2=-1;
	digitalTable[i].value3=-1;
	digitalTable[i].value4=-1;
}
