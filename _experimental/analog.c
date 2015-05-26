#include "analog.h"

void storeAnalogTable(int device_num)
{
	int i;
	MYSQL *conn=mysqlConnect();
	if(!conn)
		return;

	for(i=0;i<ANALOGCHANNELS;i++)
	{
		if(((device_num==0)||(analogTable[i].device_num==device_num))
			&&(systems[systemNumToId(device_num,NUMSYSTEMS)].enabled)
			&&(analogTable[i].enabled)
			&&(analogTable[i].id_analog!=-1)
			&&(analogTable[i].value_valid)
			&&(time(NULL)-analogTable[i].stored_time>=analogTable[i].record_data_time))
		{
			storeAnalogLine(i,conn);
			usleep(1000);
		}
	}
	mysql_close(conn);
}

int storeAnalogLine(int i,MYSQL *conn)
{
	char query[255];

	sprintf(query,"call insertHistory(%d,%d,%d,%f,%d,'%s')",
		ANALOG,
		analogTable[i].device_num,analogTable[i].id_analog,
		analogTable[i].value_eng,
		analogTable[i].value,analogTable[i].unit);
						
	if(mysql_query(conn,query)!=0)
	{
		my_printf("%s\n%s\n",query,mysql_error(conn));
		return -1;
	}
	else
	{
		analogTable[i].stored_time=time(NULL);
		return 0;
	}
}


void initializeAnalogTable(bool onlyupdate)
/*--------------------------------------------
 * tutti valori iniziali su tabella analog
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<ANALOGCHANNELS;i++)
	{
		analogTable[i].id_analog=-1;
		strcpy(analogTable[i].form_label,"");
		strcpy(analogTable[i].description,"");
		strcpy(analogTable[i].label,"");
		strcpy(analogTable[i].sinottico,"");
		analogTable[i].device_num=-1;
		analogTable[i].ch_num=-1;
		analogTable[i].scale_zero=-1;
		analogTable[i].scale_full=-1;
		analogTable[i].range_zero=-1;
		analogTable[i].range_full=-1;
		analogTable[i].bipolar=0;
		analogTable[i].al_high_active=0;
		analogTable[i].al_high=-1;
		analogTable[i].al_low_active=0;
		analogTable[i].al_low=-1;
		analogTable[i].offset=-1;
		strcpy(analogTable[i].unit,"");
		if(onlyupdate && (analogTable[i].curve!=NULL))
			free(analogTable[i].curve);
		analogTable[i].curve=NULL;
		analogTable[i].no_linear=0;
		analogTable[i].printer=0;
		analogTable[i].sms=0;
		strcpy(analogTable[i].msg_l,"");
		strcpy(analogTable[i].msg_h,"");
		analogTable[i].msg_is_event=0;
		analogTable[i].time_delay_high=0;
		analogTable[i].time_delay_low=0;
		analogTable[i].enabled=0;

		analogTable[i].ts_min=0;
		analogTable[i].ts_max=0;
		analogTable[i].value_min=0;
		analogTable[i].value_max=0;
		analogTable[i].stored_time=time(NULL);

		resetAnalogValues(i);
	}
}

void resetAnalogValues(int i)
{
	analogTable[i].time_delay_high_cur=0;
	analogTable[i].time_delay_low_cur=0;
	analogTable[i].time_delay_high_off_cur=0;
	analogTable[i].time_delay_low_off_cur=0;
	analogTable[i].currently_high=0;
	analogTable[i].currently_low=0;
	analogTable[i].record_data_time=RECORDDATATIME;
	analogTable[i].value_valid=FALSE;
	analogTable[i].value=-1;
	analogTable[i].value1=-1;
	analogTable[i].value2=-1;
	analogTable[i].value3=-1;
	analogTable[i].value4=-1;
	analogTable[i].value_eng=-1;
	analogTable[i].value_eng1=-1;
	analogTable[i].value_eng2=-1;
	analogTable[i].value_eng3=-1;
	analogTable[i].value_eng4=-1;
}

int updateAnalogChannel(int id_analog)
{
	int state;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[1023];
	int i,j;
	int systemId;
	MYSQL *conn=mysqlConnect();


	sprintf(query,"SELECT form_label,description,label,"
							"sinottico,scale_zero,scale_full,range_zero,range_full,"
							"al_high_active,al_high,al_low_active,al_low,offset,"
							"unit,time_delay_high,time_delay_low,time_delay_high_off,"
							"time_delay_low_off,printer,msg_l,msg_h,enabled,"
							"device_num,ch_num,no_linear,sms,msg_is_event,record_data_time "
						"FROM analog "
						"WHERE id_analog='%d'",id_analog);

	state = mysql_query(conn,query);

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(-1);
	}
	result = mysql_store_result(conn);
	
	if (( row = mysql_fetch_row(result)) != NULL )
	{
		j=id_analogToId(id_analog);	//source analog index
		systemId=systemNumToId(atoi(row[22]),NUMSYSTEMS);	
		if((systemId!=-1)&&(atoi(row[23])>0))
			i=deviceToid[0][systemId][atoi(row[23])-1];//destination index
		else 
			i=-1;
			
		if(i!=-1)	//destination index exists
		{
			analogTable[i].id_analog=id_analog;
			if(j!=-1)	//source index exists
			{
				if(systems[systemId].sockfd!=-1)	//dest device active
				{
					analogTable[i].value_valid=analogTable[j].value_valid;
					analogTable[i].value=analogTable[j].value;
					analogTable[i].value_max=analogTable[j].value_max;
					analogTable[i].ts_max=analogTable[j].ts_max;
					analogTable[i].value_min=analogTable[j].value_min;
					analogTable[i].ts_min=analogTable[j].ts_min;

					analogTable[i].value1=analogTable[j].value1;
					analogTable[i].value2=analogTable[j].value2;
					analogTable[i].value3=analogTable[j].value3;
					analogTable[i].value4=analogTable[j].value4;

					analogTable[i].value_eng=analogTable[j].value_eng;
					analogTable[i].value_eng1=analogTable[j].value_eng1;
					analogTable[i].value_eng2=analogTable[j].value_eng2;
					analogTable[i].value_eng3=analogTable[j].value_eng3;
					analogTable[i].value_eng4=analogTable[j].value_eng4;
	
					analogTable[i].time_delay_high_cur=analogTable[j].time_delay_high_cur;
					analogTable[i].time_delay_low_cur=analogTable[j].time_delay_low_cur;
					analogTable[i].time_delay_high_off_cur=analogTable[j].time_delay_high_off_cur;
					analogTable[i].time_delay_low_off_cur=analogTable[j].time_delay_low_off_cur;

					analogTable[i].record_data_time=analogTable[j].record_data_time;
				}
				else //dest device not active
					resetAnalogValues(i);

				if(j!=i)
				{
					analogTable[j].id_analog=-1;
					analogTable[j].enabled=0;
					resetAnalogValues(j);
				}
			}	//source index does not exist
			else
				resetAnalogValues(i);
			
			if(row[0])
				safecpy(analogTable[i].form_label,row[0]);
			else
				strcpy(analogTable[i].form_label,"");

			if(row[1])
				safecpy(analogTable[i].description,row[1]);
			else
				strcpy(analogTable[i].description,"");
			if(row[2])
				safecpy(analogTable[i].label,row[2]);
			else
				strcpy(analogTable[i].label,"");

			if(row[3])
				safecpy(analogTable[i].sinottico,row[3]);
			else
				strcpy(analogTable[i].sinottico,"");

			analogTable[i].scale_zero=atoi(row[4]);
			analogTable[i].scale_full=atoi(row[5]);
			analogTable[i].range_zero=atof(row[6]);
			analogTable[i].range_full=atof(row[7]);
			analogTable[i].al_high_active=atoi(row[8]);
			analogTable[i].al_high=atof(row[9]);
			analogTable[i].al_low_active=atoi(row[10]);
			analogTable[i].al_low=atof(row[11]);
			analogTable[i].offset=atof(row[12]);

			if(row[13])
				safecpy(analogTable[i].unit,row[13]);
			else
				strcpy(analogTable[i].unit,"");
			analogTable[i].time_delay_high=atoi(row[14]);
			analogTable[i].time_delay_low=atoi(row[15]);
			analogTable[i].time_delay_high_off=atoi(row[16]);
			analogTable[i].time_delay_low_off=atoi(row[17]);
			analogTable[i].printer=atoi(row[18]);
			if(row[19])
				safecpy(analogTable[i].msg_l,row[19]);
			else
				strcpy(analogTable[i].msg_l,"");
			if(row[20])
				safecpy(analogTable[i].msg_h,row[20]);
			else
				strcpy(analogTable[i].msg_h,"");		
			analogTable[i].enabled=atoi(row[21]);
			analogTable[i].device_num=atoi(row[22]);
			analogTable[i].ch_num=atoi(row[23]);
			analogTable[i].no_linear=atoi(row[24]);
			analogTable[i].sms=atoi(row[25]);
			analogTable[i].msg_is_event=atoi(row[26]);
			analogTable[i].record_data_time=(atoi(row[27])!=-1?60*atoi(row[27]):RECORDDATATIME);
			analogTable[i].range_pend=((double)(analogTable[i].range_full-analogTable[i].range_zero))/(analogTable[i].scale_full-analogTable[i].scale_zero);
		}
		else
		{
			if(j!=-1)
			{
				analogTable[j].id_analog=-1;
				analogTable[j].enabled=0;
				resetAnalogValues(j);
			}
		}
	}
	mysql_free_result(result);
	mysql_close(conn);
	return(0);
}

int id_analogToId(int id_analog)
{
	int i;
	
	for(i=0;i<ANALOGCHANNELS;i++)
		if(analogTable[i].id_analog==id_analog)
			break;
	return((i<ANALOGCHANNELS)?i:-1);
}

int loadAnalogTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	MYSQL *conn=mysqlConnect();

	if(!onlyupdate)
	{
		if(ANALOGCHANNELS)
		{
			my_printf("get_shared_memory_segment: analogTable\n");
			analogTable=(struct analogLine *)get_shared_memory_segment(ANALOGCHANNELS * sizeof(struct analogLine), &SHMANALOGID, "/dev/zero");
			if(!analogTable)
				die("analogTable - get_shared_memory_segment\n");
		}
		else
			analogTable=0;
	}

	initializeAnalogTable(onlyupdate);
	state = mysql_query(conn, "SELECT analog.id_analog,analog.form_label,"
							"analog.description,analog.label,analog.sinottico,"
							"analog.device_num,analog.ch_num,analog.scale_zero,"
							"analog.scale_full,analog.range_zero,analog.range_full,"
							"analog.bipolar,analog.al_high_active,analog.al_high,"
							"analog.al_low_active,analog.al_low,analog.offset,"
							"analog.unit,analog.time_delay_high,analog.time_delay_low,"
							"analog.time_delay_high_off,analog.time_delay_low_off,analog.curve,"
							"analog.no_linear,analog.printer,analog.msg_l,"
							"analog.msg_h,analog.enabled,analog.sms,analog.msg_is_event,record_data_time "
						"FROM analog JOIN system ON analog.device_num=system.device_num "
						"WHERE system.removed=0 AND analog.ch_num!=0"
						" ORDER BY analog.device_num, analog.ch_num");
	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		i=systemNumToId(atoi(row[5]),NUMSYSTEMS);
		if((i!=-1) && (atoi(row[6])<=systems[i].in_ch_an))
		{
			if(!onlyupdate)
				deviceToid[0][i][atoi(row[6]) - 1]=idRow;
			else
				idRow=deviceToid[0][i][atoi(row[6]) - 1];

			analogTable[idRow].id_analog=atoi(row[0]);
			if(row[1])
				safecpy(analogTable[idRow].form_label,row[1]);
			else
				strcpy(analogTable[idRow].form_label,"");
			if(row[2])
				safecpy(analogTable[idRow].description,row[2]);
			else
				strcpy(analogTable[idRow].description,"");
			if(row[3])
				safecpy(analogTable[idRow].label,row[3]);
			else
				strcpy(analogTable[idRow].label,"");
			if(row[4])
				safecpy(analogTable[idRow].sinottico,row[4]);
			else
				strcpy(analogTable[idRow].sinottico,"");
			analogTable[idRow].device_num=atoi(row[5]);
			analogTable[idRow].ch_num=atoi(row[6]);
			analogTable[idRow].scale_zero=atoi(row[7]);
			analogTable[idRow].scale_full=atoi(row[8]);
			analogTable[idRow].range_zero=atof(row[9]);
			analogTable[idRow].range_full=atof(row[10]);
			analogTable[idRow].range_pend=((double)(analogTable[idRow].range_full-analogTable[idRow].range_zero))/(analogTable[idRow].scale_full-analogTable[idRow].scale_zero);
			analogTable[idRow].bipolar=atoi(row[11]);
			analogTable[idRow].al_high_active=atoi(row[12]);
			analogTable[idRow].al_high=atoi(row[13]);
			analogTable[idRow].al_low_active=atoi(row[14]);
			analogTable[idRow].al_low=atoi(row[15]);
			analogTable[idRow].offset=atof(row[16]);
			if(row[17])
				safecpy(analogTable[idRow].unit,row[17]);
			else
				safecpy(analogTable[idRow].unit,"");
			analogTable[idRow].time_delay_high=atoi(row[18]);
			analogTable[idRow].time_delay_low=atoi(row[19]);
			analogTable[idRow].time_delay_high_off=atoi(row[20]);
			analogTable[idRow].time_delay_low_off=atoi(row[21]);
			if(row[22])
			{
				analogTable[idRow].curve=(char *)malloc(strlen(row[22])+1);
				safecpy(analogTable[idRow].curve,row[22]);
			}

			analogTable[idRow].no_linear=atoi(row[23]);
			analogTable[idRow].printer=atoi(row[24]);
			if(row[25])
				safecpy(analogTable[idRow].msg_l,row[25]);
			else
				safecpy(analogTable[idRow].msg_l,"");
			if(row[26])
				safecpy(analogTable[idRow].msg_h,row[26]);
			else
				safecpy(analogTable[idRow].msg_h,"");
			analogTable[idRow].enabled=atoi(row[27]);
			analogTable[idRow].sms=atoi(row[28]);
			analogTable[idRow].msg_is_event=atoi(row[29]);
			analogTable[idRow].record_data_time=(atoi(row[30])!=-1?60*atoi(row[30]):RECORDDATATIME);
		
			idRow++;
		}
	}
	mysql_free_result(result);
	
	if(!onlyupdate)
	{
		for(i=0;i<NUMSYSTEMS;i++)
			for(k=0;k<systems[i].in_ch_an;k++)
				if(deviceToid[0][i][k]==-1)
				{
					deviceToid[0][i][k]=idRow;
					idRow++;
				}
		if(idRow!=ANALOGCHANNELS)
			my_printf("there's a problem with analog channels\n");
	}
	mysql_close(conn);
	return(0);
}

void dumpAnalogTable()
{
	int i;
	for(i=0;i<ANALOGCHANNELS;i++)
		printf("description %s - device_num %d - ch_num %d - id_analog %d - enabled %d - record_data_time %d - stored_time %ld\n",
			analogTable[i].description,
			analogTable[i].device_num,
			analogTable[i].ch_num,
			analogTable[i].id_analog,
			analogTable[i].enabled,
			analogTable[i].record_data_time,
			analogTable[i].stored_time);
}
