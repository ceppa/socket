#include "digital_out.h"

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
		resetDigitalOutValues(i);
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
	digitalOutTable[i].req_value=0;
	digitalOutTable[i].value=0;
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

	sprintf(query,"SELECT description,device_num,ch_num,`default`,"
						"po_delay,on_time,"
						"pon_time,poff_time,printer "
						"FROM digital_out WHERE id_digital_out=%d",id_digital_out);

	state = mysql_query(connection,query);
	printf("%s %d\n",query,state);

	if( state != 0 )
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
	
	strcpy(ok,"ko");
	if (( row = mysql_fetch_row(result)) != NULL )
	{
		j=id_digitalOutToId(id_digital_out);//source index
		systemId=systemNumToId(atoi(row[1]),NUMSYSTEMS);

		if(atoi(row[2])>0)
			i=deviceToid[2][systemId][atoi(row[2])-1];//dest index
		else 
			i=-1;
			
			
		if(i!=-1)	//canale di destinazione >0
		{
			digitalOutTable[i].id_digital_out=id_digital_out;
			if(j!=-1) //sorgente era associato 
			{
				if(systems[systemId].sockfd!=-1)
				{
					digitalOutTable[i].start_time=digitalOutTable[j].start_time;
					digitalOutTable[i].value=digitalOutTable[j].value;
					digitalOutTable[i].def=digitalOutTable[j].def;
					digitalOutTable[i].po_delay=digitalOutTable[j].po_delay;
					digitalOutTable[i].on_time=digitalOutTable[j].on_time;
					digitalOutTable[i].pon_time=digitalOutTable[j].pon_time;
					digitalOutTable[i].poff_time=digitalOutTable[j].poff_time;
					digitalOutTable[i].printer=digitalOutTable[j].printer;
					digitalOutTable[i].req_value=digitalOutTable[j].req_value;
				}
				else
					initializeDigitalOutLine(i);

				if(i!=j)
				{
					digitalOutTable[j].id_digital_out=-1;
					initializeDigitalOutLine(j);
					printf("%d %s %d %d\n",digitalOutTable[j].id_digital_out,digitalOutTable[j].description,
						digitalOutTable[j].device_num,digitalOutTable[j].ch_num);
				}
			}
			else //sorgente non era associato
			{	
				resetDigitalOutValues(i);
				if(systems[systemId].sockfd!=-1)
				{
					digitalOutTable[i].start_time=time(NULL);
					digitalOutTable[i].value=atoi(row[3]); //valore di default
				}
			}

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
			digitalOutTable[i].printer=atoi(row[8]);
			digitalOutTable[i].req_value=digitalOutTable[i].def;
			if(digitalOutTable[i].po_delay > 0)
			{
				digitalOutTable[i].start_time=time(NULL);
				digitalOutTable[i].value=1-digitalOutTable[i].req_value;
			}

			printf("%d %s %d %d\n",digitalOutTable[i].id_digital_out,digitalOutTable[i].description,
						digitalOutTable[i].device_num,digitalOutTable[i].ch_num);
		}
		else //canale di destinazione=0
		{
			if(j!=-1) //era associato
			{
				digitalOutTable[j].id_digital_out=-1;
				initializeDigitalOutLine(j);
				printf("%d %s %d %d\n",digitalOutTable[j].id_digital_out,digitalOutTable[j].description,
						digitalOutTable[j].device_num,digitalOutTable[j].ch_num);
			}
		}
		strcpy(ok,"ok");
	}
	mysql_free_result(result);	

	sprintf(buffer,"%s %s",ok,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		perror("send");
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

	if(!onlyupdate)
	{
		if(DIGITALOUTCHANNELS>0)
		{
			printf("get_shared_memory_segment: digitalOutTable\n");
			digitalOutTable=(struct digitalOutLine *)get_shared_memory_segment(DIGITALOUTCHANNELS * sizeof(struct digitalOutLine), &SHMDIGITALOUTID, "/dev/zero");
			if(!digitalOutTable)
				die("digitalOutTable - get_shared_memory_segment\n");
		}
		else
			digitalOutTable=0;
	}
	initializeDigitalOutTable();
	state = mysql_query(connection, "SELECT digital_out.id_digital_out,"
							"digital_out.description,"
							"digital_out.device_num,"
							"digital_out.ch_num,"
							"digital_out.`default`,"
							"digital_out.po_delay,"
							"digital_out.on_value,"
							"digital_out.on_time,"
							"digital_out.pon_time,"
							"digital_out.poff_time,"
							"digital_out.printer,"
							"digital_out.msg_l,"
							"digital_out.msg_h,"
							"digital_out.sms,"
							"digital_out.msg_is_event "
						"FROM digital_out "
							"JOIN system "
							"ON digital_out.device_num=system.device_num "
						"WHERE system.removed=0 AND digital_out.ch_num!=0"
						" ORDER BY digital_out.device_num,digital_out.ch_num");

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

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
			if((digitalOutTable[idRow].po_delay > 0)
				&&(digitalOutTable[idRow].def==digitalOutTable[idRow].on_value))
			{
				digitalOutTable[idRow].value=1-digitalOutTable[idRow].req_value;
				digitalOutTable[idRow].start_time=time(NULL);
			}
			else
				digitalOutTable[idRow].value=digitalOutTable[idRow].def;
			digitalOutTable[idRow].on_value=atoi(row[6]);
			digitalOutTable[idRow].sms=atoi(row[13]);
			digitalOutTable[idRow].msg_is_event=atoi(row[14]);
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
			printf("there's a problem with digital out channels\n");
	}
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
	digitalOutTable[i].value=0;
	digitalOutTable[i].req_value=0;
	digitalOutTable[i].start_time=time(NULL);
	digitalOutTable[i].act_time=0;
}

void setOutputDefault(int systemId)
{
	int i;
	int idRow;
	
	for(i=0;i<systems[systemId].out_ch_d;i++)
	{
		idRow=deviceToid[2][systemId][i];
		if(digitalOutTable[idRow].id_digital_out!=-1)
		{
			digitalOutTable[idRow].start_time=time(NULL);
			digitalOutTable[idRow].req_value=digitalOutTable[idRow].def;
			if(digitalOutTable[idRow].po_delay>0)
				digitalOutTable[idRow].value=1-digitalOutTable[idRow].def;
			else
				digitalOutTable[idRow].value=digitalOutTable[idRow].def;
		}
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
		digitalOutTable[i].value=value;
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
		digitalOutTable[i].value=digitalOutTable[i].on_value;
	return 1;
}
