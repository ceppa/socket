#include "knx.h"

void doKNX(int l)
{
	int out;
	struct msgform msg;

	do
	{
		if(knxGateways[l].enabled)
		{
			checkKnxReload(l);
			if((knxGateways[l].status!='0')||(knxGateways[l].sockfd==-1))
			{
				knxGateways[l].status='1'; //connecting
				my_printf("trying to connect to knx %s %d\n",
					knxGateways[l].address,knxGateways[l].port);

				knxGateways[l].sockfd=doConnectKnx(l);
				if(knxGateways[l].sockfd!=-1)
					my_printf("address %s, port %d, knx connected\n",
						knxGateways[l].address, knxGateways[l].port);

			}
			if(knxGateways[l].sockfd!=-1)
			{
				out=operateKnx(l,2);
		
				if(out==-1)
				{
					handleMessage(KNX_IN,0,knxGateways[l].id_knx_gateway
						,0,"ERROR OPERATING, SLEEPING 5 SECONDS",0,ALARM_AS_ALARM);
		
					knxGatewayDisconnected(l);
					close(knxGateways[l].sockfd);
					knxGateways[l].sockfd=-1;

					my_printf("error in knx %s %d\nsleeping 5 seconds\n",
						knxGateways[l].address,knxGateways[l].port);
					sleep(5);
				}
				else
				{
					if(knxGateways[l].status!='0')
						handleMessage(KNX_IN,0,knxGateways[l].id_knx_gateway
							,0,"OPERATING OK",0,ALARM_BACK_IN_RANGE);
					knxGateways[l].status='0';	//connected
					storeKNXTable(knxGateways[l].id_knx_gateway);
				}
			}
		}
		else
			knxGateways[l].status='d';
		usleep(500000);	//intervallo interrogazione scheda
	}
	while(1);
}

int doConnectKnx(int knxGatewayId)
/*--------------------------------------------
 * gestisce la connessione/disconnessione dei gateway KNX
 * riprova ogni 10 secondi
 * int knxGatewayId - identificatore gateway [0-NUMKNXGATEWAYS-1]
 * -----------------------------------------*/
{
	int out;
	int attempt=0;
	char sendBuf[50];
	int received=0,numbytes;
	struct msgform msg;

	do
	{
		out=0;
		if(knxGateways[knxGatewayId].enabled==0)
			out=-1;
		else
			out=socketConnect(knxGateways[knxGatewayId].address,knxGateways[knxGatewayId].port);
		if(out!=-1)
		{
			attempt=0;
			if (!send(out,"connect \r",10,0)) 
				out=-1;
			else
			{
//				if(recv(out, sendBuf, 50,0)<1)
//					out=-1;
				if(receive(out, sendBuf, 50,"connect",2)<1)
					out=-1;
				else
				{
					strcpy(sendBuf,"dts %00010111 \r");
	
					if (!send(out,sendBuf,strlen(sendBuf),0)) 
						out=-1;
				}
			}
		}
		else
		{
			if(knxGateways[knxGatewayId].enabled)
			{
				attempt++;
				my_printf("%s %d: attempt %d failed\n",
					knxGateways[knxGatewayId].address,
					knxGateways[knxGatewayId].port,attempt);
			}

			if(attempt>=MAXATTEMPTS)
			{
				if(knxGateways[knxGatewayId].status!='3')
					handleMessage(KNX_IN,0,
						knxGateways[knxGatewayId].id_knx_gateway,0,
						"KNX GATEWAY DISCONNECTED",0,ALARM_AS_ALARM);
				knxGateways[knxGatewayId].status='3';	//failed after maxattempts
				attempt=0;
			}
			knxGatewayDisconnected(knxGatewayId);
			sleep(5);
		}
	}
	while(out<0);

	knxGateways[knxGatewayId].recvBufReceived=0;
	knxGateways[knxGatewayId].failures=0;
	handleMessage(0,KNX_IN,knxGateways[knxGatewayId].id_knx_gateway,0,
		"KNX GATEWAY CONNECTED",0,ALARM_BACK_IN_RANGE);

	return out;
}

int operateKnx(int knxGatewayId, int timeout)
{
	int sockfd;
	time_t inizio,curtime;
	int out=0;
	int numbytes;
	int sendLen=512;
	unsigned char *recvBuf;
	char *sendBuf;
	char source[20];
	char dest[20];
	unsigned int value;
	unsigned int outvalue;
	int i;
	int recvBufReceived;
	int lineLength;
	unsigned char tempLine[255];
	int messageType;
	char *messageText;
	int is_sms;
	double value_eng;
	struct tm *curts;
	struct tm *mints;
	struct tm *maxts;
	bool tostore;

	sockfd=knxGateways[knxGatewayId].sockfd;
	curtime=time(NULL);
	curts=localtime(&curtime);
	

	// legge da knxGateway
	// aggiorna valori

	// receive

	recvBuf=knxGateways[knxGatewayId].recvBuf;
	recvBufReceived=knxGateways[knxGatewayId].recvBufReceived;

	numbytes=recv(sockfd, &recvBuf[recvBufReceived], 
			knxRecvLen-recvBufReceived, MSG_DONTWAIT);

	if((numbytes==-1)&& (errno!=EAGAIN))
	{
		perror("knx");
		return -1;
	}

	if(numbytes>0)
		recvBufReceived+=numbytes;

	knxGateways[knxGatewayId].recvBufReceived=recvBufReceived;

/*	if(recvBufReceived)
	{
		printf("recvBufReceived %d\n",recvBufReceived);
		dumpBuffer(recvBuf,recvBufReceived);
	}*/
	while(lineLength=getResponseLine(recvBuf,recvBufReceived))
	{
		memcpy(tempLine,recvBuf,lineLength);
		tempLine[lineLength-2]='\0';
/*		printf("tempLineLength %d\n",lineLength);
		dumpBuffer(tempLine,lineLength);
		printf("%s\n",tempLine);
*/

		switch(tempLine[2])
		{
			case 'i':
				my_printf("%s - %d - %s non trovato\n",tempLine,lineLength,dest);

				if(parseKnxOutput(tempLine,source,dest,&value)!=-1)
				{
					i=group_addressToId(dest);
					if(i!=-1)
					{
						strcpy(knxTable[i].group_address,dest);
						strcpy(knxTable[i].physical_address,source);

//messaggi e allarmi
						if((knxTable[i].data_type=='B')	//solo binari
							&&(((knxTable[i].value==1)
								&&(value==0)
								&&(strlen(knxTable[i].hi_low_msg)))
							||((knxTable[i].value==0)
								&&(value==1)
								&&(strlen(knxTable[i].low_hi_msg)))))
						{
							is_sms=((knxTable[i].is_alarm)&&(value==knxTable[i].alarm_transition));
							messageType=-1;
							if(knxTable[i].is_alarm)
								messageType=(value==knxTable[i].alarm_transition?ALARM_AS_ALARM:ALARM_BACK_IN_RANGE);
						
							messageText=(value==1?knxTable[i].low_hi_msg:knxTable[i].hi_low_msg);

							handleMessage(
									KNX_IN,
									knxTable[i].id_knx_line,
									knxTable[i].id_knx_gateway,
									0,
									messageText,
									is_sms,
									messageType);
						}
//fine messaggi e allarmi
						knxTable[i].value=value;
						knxTable[i].value_valid=TRUE;
						switch(knxTable[i].data_type)
						{
							case 'F':
								value_eng=intToDouble(value);
								break;
							case 'C':
								value_eng=(double)my_round(((double)(value))*100/255);
								break;
							case 'B':
								value_eng=(value&1);
								break;
							default:
								value_eng=value;
								break;
						}
						knxTable[i].value_eng=value_eng;

//gestione max e min del giorno
						KNXHandleMaxMin(i,value_eng);
						my_printf("aggiornato %s da %s, valore %d\n",dest,source,value);
					}
					else
					{
						my_printf("%s - %d - %s non trovato\n",tempLine,lineLength,dest);
					}

				}
				break;
			default:
				if((strlen(tempLine)>0)&&(tempLine[0]!='>'))
					my_printf("%s\n",tempLine);
				break;
		}
		memcpy(recvBuf,&recvBuf[lineLength],recvBufReceived-lineLength);
		recvBufReceived-=lineLength;
		knxGateways[knxGatewayId].recvBufReceived=recvBufReceived;
	}
	
	handleKNXAutomaticUpdate();
	//send request value or update
	for(i=0;i<NUMKNXCHANNELS;i++)
	{
		if((knxTable[i].enabled)&&(knxTable[i].req_value!=-1))
		{
			sendBuf=(char *)malloc(sendLen);
			if(knxTable[i].input_output==1)
			{
				sprintf(sendBuf,"trs (%s) \r",
					knxTable[i].group_address);
			}
			else
			{
				switch(knxTable[i].data_type)
				{
					case 'F':
						sprintf(sendBuf,"tds (%s %d) $%x $%x\r",
							knxTable[i].group_address,
							2,
							(knxTable[i].req_value>>8),
							(knxTable[i].req_value&0xff));
						break;
					case 'B':
						sprintf(sendBuf,"tds (%s) %d \r",
							knxTable[i].group_address,
							(knxTable[i].req_value>0?1:0));
						break;
					case 'C':
						outvalue=(knxTable[i].req_value<100?knxTable[i].req_value:100);
						outvalue=(int)((float)outvalue*2.55);
						sprintf(sendBuf,"tds (%s %d) $%x \r",
							knxTable[i].group_address,
							1,
							outvalue);					
						break;
					default:
						sprintf(sendBuf,"tds (%s %d) $%x \r",
							knxTable[i].group_address,
							1,
							knxTable[i].req_value);					
						break;
				}
			}
			my_printf("richiedo: %s\n",sendBuf);
			if (!send(sockfd,sendBuf,strlen(sendBuf),0)) 
				out=-1;
			knxTable[i].req_value=-1;
			free(sendBuf);
			break;	//one update per loop!
		}
	}
	return out;
}

void knxGatewayDisconnected(int knxGatewayId)
/*--------------------------------------------
 * scrive su knxTable -1
 * chiamata quando non "sento" la scheda
 * int knxGatewayId - identificativo device [0-NUMKNXGATEWAYS-1]
 * -----------------------------------------*/
{
	int i,j,id;

	knxGateways[knxGatewayId].failures=0;
	if((knxGateways[knxGatewayId].enabled)
			&&(knxGateways[knxGatewayId].status!='3'))
		knxGateways[knxGatewayId].status='2';
	for(j=0;j<knxGateways[knxGatewayId].ch_in;j++)
	{
		id=deviceToid[4][knxGatewayId][j];
		resetKnxValue(id);
	}
	for(j=0;j<knxGateways[knxGatewayId].ch_out;j++)
	{
		id=deviceToid[5][knxGatewayId][j];
		resetKnxValue(id);
	}
}


int id_knx_gatewayToId(int id_knx_gateway)
{
	int i;
	
	for(i=0;i<NUMKNXGATEWAYS;i++)
		if(knxGateways[i].id_knx_gateway==id_knx_gateway)
			break;
	return((i<NUMKNXGATEWAYS)?i:-1);
}

int loadKnx(bool reload)
{
	MYSQL_RES *result;
	MYSQL_RES *resultLines;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	char query[512];
	char querylines[512];
	MYSQL *conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return (1);
	}

	strcpy(query,"SELECT knx_gateway.id,"
					"knx_gateway.address,"
					"knx_gateway.port,"
					"knx_gateway.physical_address,"
					"knx_gateway.type,"
					"knx_gateway.description,"
					"knx_gateway.enabled "
					" FROM knx_gateway"
					" ORDER BY knx_gateway.id");
	state = mysql_query(conn, query);

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);



	strcpy(querylines,"SELECT knx_in_out.id,"
					"knx_in_out.id_knx_gateway,"
					"knx_in_out.input_output,"
					"knx_in_out.group_address,"
					"knx_in_out.data_type,"
					"knx_in_out.description,"
					"knx_in_out.enabled,"
					"knx_in_out.hi_low_msg,"
					"knx_in_out.low_hi_msg,"
					"knx_in_out.is_alarm,"
					"knx_in_out.alarm_transition,"
					"knx_in_out.sms,"
					"knx_in_out.record_data_time "
					" FROM knx_in_out"
					" ORDER BY knx_in_out.id");
	state = mysql_query(conn, querylines);

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	resultLines = mysql_store_result(conn);

	if(reload)
	{
		if(shmdt(knxGateways))
			my_perror("shmdt");
		shmctl(SHMKNXGATEWAYSID, IPC_RMID, &shmid_struct);

		if(shmdt(knxTable))
			my_perror("shmdt");
		shmctl(SHMKNXTABLEID, IPC_RMID, &shmid_struct);		
	}
	NUMKNXGATEWAYS=mysql_num_rows(result);
	NUMKNXCHANNELS=mysql_num_rows(resultLines);
	if(NUMKNXGATEWAYS&&NUMKNXCHANNELS)
	{
		my_printf("get_shared_memory_segment: knxGateways\n");
		knxGateways=(struct knxGateway *)get_shared_memory_segment(NUMKNXGATEWAYS * sizeof(struct knxGateway), &SHMKNXGATEWAYSID, "/dev/zero");
		if(!knxGateways)
			die("knxGateways - get_shared_memory_segment\n");
		my_printf("get_shared_memory_segment: knxTable\n");
		knxTable=(struct knxLine *)get_shared_memory_segment(NUMKNXCHANNELS * sizeof(struct knxLine), &SHMKNXTABLEID, "/dev/zero");
		if(!knxTable)
			die("knxTable - get_shared_memory_segment\n");
	}
	else
	{
		knxTable=0;
		knxGateways=0;
	}

	i=0;

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		knxGateways[i].id_knx_gateway=atoi(row[0]);
		if(row[1])
			strcpy(knxGateways[i].address,row[1]);
		else
			strcpy(knxGateways[i].address,"");
		knxGateways[i].port=atoi(row[2]);
		if(row[3])
			strcpy(knxGateways[i].physical_address,row[3]);
		else
			strcpy(knxGateways[i].physical_address,"");

		knxGateways[i].type=atoi(row[4]);
		if(row[5])
			strcpy(knxGateways[i].description,row[5]);
		else
			strcpy(knxGateways[i].description,"");
		
		knxGateways[i].enabled=atoi(row[6]);

		if(!reload)
		{
			if(knxGateways[i].enabled)
				knxGateways[i].status='1';
			else
				knxGateways[i].status='d';
		}
		knxGateways[i].ch_in=0;
		knxGateways[i].ch_out=0;
		knxGateways[i].failures=0;
		i++;
	}
	mysql_free_result(result);

	i=0;
	while( ( row = mysql_fetch_row(resultLines)) != NULL )
	{
		knxTable[i].id_knx_line=atoi(row[0]);
		knxTable[i].id_knx_gateway=atoi(row[1]);
		knxTable[i].input_output=atoi(row[2]);
		if(row[3])
			strcpy(knxTable[i].group_address,row[3]);
		else
			strcpy(knxTable[i].group_address,"");
		fix_group_address(knxTable[i].group_address);

		if(row[4])
			knxTable[i].data_type=row[4][0];
		else
			knxTable[i].data_type=0;
		if(row[5])
			strcpy(knxTable[i].description,row[5]);
		else
			strcpy(knxTable[i].description,"");
		knxTable[i].enabled=atoi(row[6]);

		if((knxTable[i].input_output)&&(knxTable[i].data_type=='B'))
		{
			if(row[7])
				strcpy(knxTable[i].hi_low_msg,row[7]);
			else
				strcpy(knxTable[i].hi_low_msg,"");
			if(row[8])
				strcpy(knxTable[i].low_hi_msg,row[8]);
			else
				strcpy(knxTable[i].low_hi_msg,"");
			knxTable[i].is_alarm=atoi(row[9]);
			knxTable[i].alarm_transition=atoi(row[10]);
			knxTable[i].sms=atoi(row[11]);
			knxTable[i].record_data_time=(atoi(row[12])!=-1?60*atoi(row[12]):RECORDDATATIME);
		}
		else
		{
			strcpy(knxTable[i].hi_low_msg,"");
			strcpy(knxTable[i].low_hi_msg,"");
			knxTable[i].is_alarm=0;
			knxTable[i].alarm_transition=0;
			knxTable[i].sms=0;
		}
		k=id_knx_gatewayToId(knxTable[i].id_knx_gateway);
		if(k!=-1)
		{
			if(knxTable[i].input_output==1)
				knxGateways[k].ch_in++;
			else
				knxGateways[k].ch_out++;
		}
		strcpy(knxTable[i].physical_address,"");
		knxTable[i].value=-1;
		knxTable[i].value_valid=FALSE;
		knxTable[i].value_max=0;
		knxTable[i].ts_max=0;
		knxTable[i].value_min=0;
		knxTable[i].ts_min=0;
		knxTable[i].stored_time=time(NULL);
		knxTable[i].value_eng=-1;
		knxTable[i].req_value=(knxTable[i].input_output?1:-1);
		i++;
	}
	mysql_free_result(resultLines);
	mysql_close(conn);
	return(0);
}

int updateKnxLine(int id)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i;
	char query[512];
	MYSQL *conn=mysqlConnect();

	i=id_knx_lineToId(id);
	if(i==-1)
		return(-1);

	sprintf(query,"SELECT "
					"knx_in_out.group_address,"
					"knx_in_out.data_type,"
					"knx_in_out.description,"
					"knx_in_out.enabled,"
					"knx_in_out.hi_low_msg,"
					"knx_in_out.low_hi_msg,"
					"knx_in_out.is_alarm,"
					"knx_in_out.alarm_transition,"
					"knx_in_out.sms,"
					"knx_in_out.record_data_time "
					" FROM knx_in_out"
					" WHERE knx_in_out.id='%d'",id);
	state = mysql_query(conn, query);

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(-1);
	}
	result = mysql_store_result(conn);
	if(mysql_num_rows(result)!=1)
		return(-1);

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		if(row[0])
			strcpy(knxTable[i].group_address,row[0]);
		else
			strcpy(knxTable[i].group_address,"");
		fix_group_address(knxTable[i].group_address);

		if(row[1])
			knxTable[i].data_type=row[1][0];
		else
			knxTable[i].data_type=0;
		if(row[2])
			strcpy(knxTable[i].description,row[2]);
		else
			strcpy(knxTable[i].description,"");
		knxTable[i].enabled=atoi(row[3]);

		if((knxTable[i].input_output)&&(knxTable[i].data_type=='B'))
		{
			if(row[4])
				strcpy(knxTable[i].hi_low_msg,row[4]);
			else
				strcpy(knxTable[i].hi_low_msg,"");
			if(row[5])
				strcpy(knxTable[i].low_hi_msg,row[5]);
			else
				strcpy(knxTable[i].low_hi_msg,"");
			knxTable[i].is_alarm=atoi(row[6]);
			knxTable[i].alarm_transition=atoi(row[7]);
			knxTable[i].sms=atoi(row[8]);
			knxTable[i].record_data_time=(atoi(row[9])!=-1?60*atoi(row[9]):RECORDDATATIME);
		}
		else
		{
			strcpy(knxTable[i].hi_low_msg,"");
			strcpy(knxTable[i].low_hi_msg,"");
			knxTable[i].is_alarm=0;
			knxTable[i].alarm_transition=0;
			knxTable[i].sms=0;
		}
	}
	mysql_free_result(result);
	mysql_close(conn);
	return(0);

}

int setKnx(int id,double value)
{
	int i;
	i=id_knx_lineToId(id);

	if(i==-1)
		return -1;

	if(knxTable[i].data_type!='F')
		knxTable[i].req_value=(unsigned int)value;
	else
		knxTable[i].req_value=doubleToInt(value);
	return 1;
}

void resetKnxValue(int i)
{
	knxTable[i].value=-1;
	knxTable[i].value_valid=FALSE;
	knxTable[i].value_eng=-1;
	knxTable[i].req_value=(knxTable[i].input_output?1:-1);
}

int group_addressToId(char *group_address)
{
	register int i;
	for(i=0;i<NUMKNXCHANNELS;i++)
		if(strcmp(knxTable[i].group_address,group_address)==0)
			break;
	return((i<NUMKNXCHANNELS)?i:-1);
}


int id_knx_lineToId(int id_knx_line)
{
	int i;
	
	for(i=0;i<NUMKNXCHANNELS;i++)
		if(knxTable[i].id_knx_line==id_knx_line)
			break;
	return((i<NUMKNXCHANNELS)?i:-1);
}

int parseKnxOutput(char *string,char *source,char *dest,unsigned int *data)
{
	int l;
	char *c,*d,*e,*f,*g;
	char lb[4];
	int out=-1;
	strcpy(source,"");
	strcpy(dest,"");
	*data=0;
	register int i;
	unsigned char v;
	
	l=strlen(string);
	if(c=strchr(string,'('))
	{
		c++;
		if(d=strchr(c,')'))
		{
			if(e=strchr(c,' '))
			{
				memcpy(source,c,e-c);
				source[e-c]='\0';
				e++;
				if(f=strchr(e,' '))
				{
					memcpy(dest,e,f-e);
					dest[f-e]='\0';
					f++;
					memcpy(lb,f,d-f);
					lb[d-f]='\0';
					l=knxValToInt(lb);
					for(d++;(*d)!=0;d++)
						if(*d!=' ')
							break;

					for(i=0;i<(l-1);i++)
					{
						if(c=strchr(d,' '))
						{
							*c='\0';

							v=knxValToInt(d);
							(*data)=((*data)<<8)+v;
							d=c+1;
						}
						else
							return(-1);
					}
					v=knxValToInt(d);
					(*data)=((*data)<<8)+v;
					out=1;
				}
			}
		}
	}
}

unsigned char knxValToInt(char *in)
{
	register int i;
	int out=0;
	in=trim(in);

	switch(in[0])
	{
		case '$':
			in[0]='\0';
			in+=strlen(&in[1]);
			out=HextoDec(in);
			break;
		case '%':
			for(i=1;i<strlen(in);i++)
				out=((out<<1)+(in[i]=='1'?1:0));
			break;
		default:
			out=atoi(in);
			break;
	}
	return out;
}

int HextoDec(char *hex)
{
    if (*hex==0) 
		return 0;
    return HextoDec(hex-1)*16 + xtod(*hex) ; 
}
 
void updateKnx(int id_knx_gateway)
{
	int state;
	int id;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i;
	MYSQL *conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return;
	}

	id=id_knx_gatewayToId(id_knx_gateway);
	if(id==-1)
		return;
	knxGateways[id].enabled=0;

	sprintf(query,"SELECT knx_gateway.address,knx_gateway.port,knx_gateway.enabled"
								" FROM knx_gateway "
								" WHERE knx_gateway.id=%d",id_knx_gateway);

	state = mysql_query(conn,query);

	if( state != 0 )
	{
		my_printf("%s - %s\n",query,mysql_error(conn));
		return;
	}
	result = mysql_store_result(conn);
	
	if (( row = mysql_fetch_row(result)) != NULL )
	{
		if(row[0])
			strcpy(knxGateways[id].address,row[0]);
		else
			strcpy(knxGateways[id].address,"");
		knxGateways[id].port=atoi(row[1]);
		knxGateways[id].enabled=atoi(row[2]);
		if(knxGateways[id].enabled)
			knxGateways[id].enabled=2;	//force restart
	}
	mysql_free_result(result);
	mysql_close(conn);
}

void checkKnxReload(int id)
{				
	if(knxGateways[id].enabled>1)
	{
		my_printf("requested knx reload, address %s port %d\n",
						knxGateways[id].address,knxGateways[id].port);
		close(knxGateways[id].sockfd);
		knxGateways[id].sockfd=-1;
		knxGateways[id].enabled=1;
		knxGateways[id].status='1';
	}
}


int getResponseLine(unsigned char *buf, int len)
{
	int i;
	for(i=0;i<len;i++)
		if(buf[i]==0x0d)
			break;
	return (i<len?i+1:0);
}

void fix_group_address(char *group_address)
{
	char *c,*d;
	int g1,g2,g3;

	if(c=strchr(group_address,'/'))
	{
		if(d=strchr(c+1,'/'))
		{
			*c='\0';
			*d='\0';
			sprintf(group_address,"%02d/%02d/%03d",atoi(group_address),
				atoi(c+1),atoi(d+1));
		}
	}
}

void handleKNXAutomaticUpdate()
{
	int ones=0;
	int i;

	for(i=0;i<NUMKNXCHANNELS;i++)
	{
		if((knxTable[i].enabled)
			&&(knxTable[i].input_output==1)
			&&(knxTable[i].req_value!=-1))
		{
			if(knxTable[i].req_value>1)
				knxTable[i].req_value--;
			ones++;
		}
	}
	if(ones==0)
		for(i=0;i<NUMKNXCHANNELS;i++)
			if((knxTable[i].enabled)
				&&(knxTable[i].input_output==1))
			knxTable[i].req_value=4;
}

void storeKNXTable(int id_knx_gateway)
{
	int i;
	MYSQL *conn=mysqlConnect();
	if(!conn)
		return;

	for(i=0;i<NUMKNXCHANNELS;i++)
	{
		if((knxGateways[id_knx_gatewayToId(id_knx_gateway)].enabled)
			&&(knxTable[i].id_knx_gateway==id_knx_gateway)
			&&(knxTable[i].enabled)
			&&(knxTable[i].id_knx_line!=-1)
			&&(knxTable[i].value_valid)
			&&(time(NULL)-knxTable[i].stored_time>=knxTable[i].record_data_time))
		{
			storeKNXLine(i,conn);
			usleep(1000);
		}
	}
	mysql_close(conn);
}

int storeKNXLine(int i,MYSQL *conn)
{
	int j;
	char query[255];

	j=(knxTable[i].input_output?KNX_IN:KNX_OUT);
	sprintf(query,"call insertHistory(%d,%d,%d,%f,%d,'')",
		j,
		knxTable[i].id_knx_gateway,
		knxTable[i].id_knx_line,
		knxTable[i].value_eng,
		knxTable[i].value);
	if(mysql_query(conn,query)!=0)
		my_printf("%s\n%s\n",query,mysql_error(conn));
	else
		knxTable[i].stored_time=time(NULL);
}

void KNXHandleMaxMin(int i,double value)
{
	time_t ts_cur,ts_min,ts_max;
	int d_cur,m_cur,y_cur,d_min,m_min,y_min,d_max,m_max,y_max;
	struct tm *tm_cur;
	struct tm *tm_min;
	struct tm *tm_max;
	bool tostore;
	MYSQL *conn;

	ts_cur=time(NULL);
	tm_cur=localtime(&ts_cur);
	d_cur=tm_cur->tm_mday;
	m_cur=tm_cur->tm_mon;
	y_cur=tm_cur->tm_year;
	tostore=FALSE;

	ts_min=knxTable[i].ts_min;
	tm_min=localtime(&ts_min);
	d_min=tm_min->tm_mday;
	m_min=tm_min->tm_mon;
	y_min=tm_min->tm_year;

	ts_max=knxTable[i].ts_max;
	tm_max=localtime(&ts_max);
	d_max=tm_max->tm_mday;
	m_max=tm_max->tm_mon;
	y_max=tm_max->tm_year;
	if((value>knxTable[i].value_max)
			||(d_cur!=d_max)
			||(m_cur!=m_max)
			||(y_cur!=y_max))
	{
		tostore=TRUE;
		knxTable[i].ts_max=ts_cur;
		knxTable[i].value_max=value;
	}

	if((value<knxTable[i].value_min)
			||(d_cur!=d_min)
			||(m_cur!=m_min)
			||(y_cur!=y_min))
	{
		tostore=TRUE;
		knxTable[i].ts_min=ts_cur;
		knxTable[i].value_min=value;
	}
	if(tostore)
	{
		MYSQL *conn=mysqlConnect();
		if(!conn)
			return;
		storeKNXLine(i,conn);
		mysql_close(conn);
	}
}
