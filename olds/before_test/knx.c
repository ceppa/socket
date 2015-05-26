#include "knx.h"

void doKNX(int l)
{
	int i,out;
	struct msgform msg;

	if(knxGateways[l].enabled)
		knxGateways[l].status='1'; //connecting
	printf("trying to connect to knx %s %d\n",
		knxGateways[l].address,knxGateways[l].port);

	knxGateways[l].sockfd=doConnectKnx(l);
	printf("address %s, port %d, knx connected\n",knxGateways[l].address, knxGateways[l].port);

	i=0;
	do
	{
		out=operateKnx(knxGateways[l].sockfd,l,2);

		if(out==-1)
		{
			handleMessage(0,0,knxGateways[l].id_knx_gateway,0,"GATEWAY DISCONNECTED",0,-1);

			knxGatewayDisconnected(l);
			close(knxGateways[l].sockfd);
			knxGateways[l].sockfd=-1;
			if(knxGateways[l].enabled)
			{
				printf("error in %s %d\nsleeping 5 seconds\n",knxGateways[l].address,knxGateways[l].port);
				sleep(5);
				printf("trying to connect\n");
			}
			knxGateways[l].sockfd=doConnectKnx(l);
			printf("address %s port %d, knx connected\n",knxGateways[l].address, knxGateways[l].port);
		}
		else
			knxGateways[l].status='0';	//connected
		i=(i+1)%2;
		usleep(50000);	//intervallo interrogazione scheda
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
		if(out==-1)
		{
			if(knxGateways[knxGatewayId].enabled)
			{
				attempt++;
				printf("%s %d: attempt %d failed\n",
					knxGateways[knxGatewayId].address,
					knxGateways[knxGatewayId].port,attempt);
			}

			if(attempt>MAXATTEMPTS)
			{
				knxGateways[knxGatewayId].status='3';	//failed after maxattempts
				attempt=0;
			}
			knxGatewayDisconnected(knxGatewayId);
			sleep(5);
		}
	}
	while(out<0);

	knxGateways[knxGatewayId].failures=0;
	handleMessage(0,0,knxGateways[knxGatewayId].id_knx_gateway,0,"GATEWAY CONNECTED",0,-1);

	return out;
}

int operateKnx(int sockfd,int knxGatewayId, int timeout)
{
	time_t inizio,curtime;
	int out=0;
	int numbytes;
	int recvLen=512;
	int sendLen=512;
	unsigned char *recvBuf;
	char *sendBuf;
	int received=0;
	char source[20];
	char dest[20];
	unsigned int value;
	int i;

	// legge da knxGateway
	// aggiorna valori

	recvBuf=(unsigned char *)malloc(recvLen);

	if(knxGateways[knxGatewayId].enabled>2)	//try to reload
	{
		knxGateways[knxGatewayId].status='1';	//loading
		inizio=time(NULL);
		do
		{
			usleep(5000);
		}
		while((knxGateways[knxGatewayId].enabled>2)&&(time(NULL)-inizio<timeout));
		if(knxGateways[knxGatewayId].enabled>2)
			knxGateways[knxGatewayId].enabled-=3;
	}
	if(knxGateways[knxGatewayId].enabled!=1)
	{
		if(knxGateways[knxGatewayId].enabled>1)
			knxGateways[knxGatewayId].enabled=1;
		else
			knxGateways[knxGatewayId].status='d';
		return -1;
	}

	//receive
	received=0;
	do
	{
		numbytes=recv(sockfd, &recvBuf[received], recvLen-received, MSG_DONTWAIT);
		if(numbytes>0)
			received+=numbytes;
	}
	while((received>0)&&(recvBuf[received-1]!=0x0d));
	if(received>0)
	{
		recvBuf[received]='\0';
		switch(recvBuf[2])
		{
			case 'i':
				if(parseKnxOutput(recvBuf,source,dest,&value)!=-1)
				{
					i=group_addressToId(dest);
					if(i!=-1)
					{
						strcpy(knxTable[i].group_address,dest);
						strcpy(knxTable[i].physical_address,source);
						knxTable[i].value=value;
						if(knxTable[i].data_type=='F')
							knxTable[i].value_eng=intToDouble(value);
						else
							knxTable[i].value_eng=value;
						printf("aggiornato %s da %s, valore %d\n",dest,source,value);
					}
					else
						printf("%s non trovato\n",dest);

				}
				break;
			default:
				printf("%s\n",recvBuf);
				break;

		}
	}
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
				if(knxTable[i].data_type!='F')
					sprintf(sendBuf,"tds (%s %d) $%x \r",
						knxTable[i].group_address,
						1,
						knxTable[i].req_value);
				else
					sprintf(sendBuf,"tds (%s %d) $%x $%x\r",
						knxTable[i].group_address,
						2,
						(knxTable[i].req_value>>8),
						(knxTable[i].req_value&0xff));
			}
			printf("richiedo: %s\n",sendBuf);
			if (!send(sockfd,sendBuf,strlen(sendBuf),0)) 
				out=-1;
			knxTable[i].req_value=-1;
			free(sendBuf);
			break;	//one update per loop!
		}
	}
	free(recvBuf);
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
	
	strcpy(query,"SELECT knx_gateway.id,"
					"knx_gateway.address,"
					"knx_gateway.port,"
					"knx_gateway.physical_address,"
					"knx_gateway.type,"
					"knx_gateway.description,"
					"knx_gateway.enabled"
					" FROM knx_gateway"
					" ORDER BY knx_gateway.id");
	state = mysql_query(connection, query);

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);



	strcpy(querylines,"SELECT knx_in_out.id,"
					"knx_in_out.id_knx_gateway,"
					"knx_in_out.input_output,"
					"knx_in_out.group_address,"
					"knx_in_out.data_type,"
					"knx_in_out.description,"
					"knx_in_out.enabled"
					" FROM knx_in_out"
					" ORDER BY knx_in_out.id");
	state = mysql_query(connection, querylines);

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	resultLines = mysql_store_result(connection);


	if(reload)
	{
		if(shmdt(knxGateways))
			perror("shmdt");
		shmctl(SHMKNXGATEWAYSID, IPC_RMID, &shmid_struct);

		if(shmdt(knxTable))
			perror("shmdt");
		shmctl(SHMKNXTABLEID, IPC_RMID, &shmid_struct);		
	}
	NUMKNXGATEWAYS=mysql_num_rows(result);
	NUMKNXCHANNELS=mysql_num_rows(resultLines);
	if(NUMKNXGATEWAYS&&NUMKNXCHANNELS)
	{
		printf("get_shared_memory_segment: knxGateways\n");
		knxGateways=(struct knxGateway *)get_shared_memory_segment(NUMKNXGATEWAYS * sizeof(struct knxGateway), &SHMKNXGATEWAYSID, "/dev/zero");
		if(!knxGateways)
			die("knxGateways - get_shared_memory_segment\n");
		printf("get_shared_memory_segment: knxTable\n");
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
		if(row[4])
			knxTable[i].data_type=row[4][0];
		else
			knxTable[i].data_type=0;
		if(row[5])
			strcpy(knxTable[i].description,row[5]);
		else
			strcpy(knxTable[i].description,"");
		knxTable[i].enabled=atoi(row[6]);

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
		knxTable[i].value_eng=-1;
		knxTable[i].req_value=-1;
		i++;
	}
	mysql_free_result(resultLines);
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
	knxTable[i].value_eng=-1;
	knxTable[i].req_value=-1;
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
 
