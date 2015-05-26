#include "ceiabi.h"

void doCEIABI(int l)
{
	int out;
	struct msgform msg;

	do
	{
		if(CEIABIGateways[l].enabled)
		{
			checkCEIABIReload(l);
			if((CEIABIGateways[l].status!='0')||(CEIABIGateways[l].sockfd==-1))
			{
				CEIABIGateways[l].status='1'; //connecting
				my_printf("trying to connect to CEIABI %s %d\n",
					CEIABIGateways[l].address,CEIABIGateways[l].port);

				CEIABIGateways[l].sockfd=doConnectCEIABI(l);
				if(CEIABIGateways[l].sockfd!=-1)
					my_printf("address %s, port %d, CEIABI connected\n",
						CEIABIGateways[l].address, CEIABIGateways[l].port);

			}
			if(CEIABIGateways[l].sockfd!=-1)
			{
				out=operateCEIABI(CEIABIGateways[l].sockfd,l,1);
		
				if(out==-1)
				{
					handleMessage(CEIABI,0,CEIABIGateways[l].id_CEIABI_gateway
						,0,"ERROR OPERATING, SLEEPING 5 SECONDS",0,ALARM_AS_ALARM);
		
					CEIABIGatewayDisconnected(l);
					close(CEIABIGateways[l].sockfd);
					CEIABIGateways[l].sockfd=-1;

					my_printf("error in CEIABI %s %d\nsleeping 5 seconds\n",
						CEIABIGateways[l].address,CEIABIGateways[l].port);
					sleep(5);
				}
				else
				{
					if(CEIABIGateways[l].status!='0')
						handleMessage(CEIABI,0,CEIABIGateways[l].id_CEIABI_gateway
							,0,"OPERATING OK",0,ALARM_BACK_IN_RANGE);
					CEIABIGateways[l].status='0';	//connected
				}
			}
		}
		else
			CEIABIGateways[l].status='d';
		usleep(500000);	//intervallo interrogazione scheda
	}
	while(1);
}

int doConnectCEIABI(int id)
/*--------------------------------------------
 * gestisce la connessione/disconnessione dei gateway CEIABI
 * riprova ogni 10 secondi
 * int id - identificatore gateway [0-NUMCEIABIGATEWAYS-1]
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
		if(CEIABIGateways[id].enabled==0)
			out=-1;
		else
			out=socketConnect(CEIABIGateways[id].address,CEIABIGateways[id].port);
		if(out!=-1)
			attempt=0;
		else
		{
			if(CEIABIGateways[id].enabled)
			{
				attempt++;
				my_printf("%s %d: attempt %d failed\n",
					CEIABIGateways[id].address,
					CEIABIGateways[id].port,attempt);
			}

			if(attempt>=MAXATTEMPTS)
			{
				if(CEIABIGateways[id].status!='3')
					handleMessage(CEIABI,0,
						CEIABIGateways[id].id_CEIABI_gateway,0,
						"CEIABI GATEWAY DISCONNECTED",0,ALARM_AS_ALARM);
				CEIABIGateways[id].status='3';	//failed after maxattempts
				attempt=0;
			}
			CEIABIGatewayDisconnected(id);
			sleep(5);
		}
	}
	while(out<0);

	CEIABIGateways[id].recvBufReceived=0;
	CEIABIGateways[id].failures=0;
	handleMessage(0,CEIABI,CEIABIGateways[id].id_CEIABI_gateway,0,
		"CEIABI GATEWAY CONNECTED",0,ALARM_BACK_IN_RANGE);

	return out;
}

int operateCEIABI(int sockfd,int id, int timeout)
{
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
	int crclength;
	int sendlength;
	BYTE buffer[512];
	BYTE bufout[512];
	BYTE msg[4]={0x00,0x49,0x00,0x01};
	WORD crc;
	int l,buflen,received;
	int lout=0;

	l=sizeof(msg);
	buflen=sizeof(buffer);
	crc=CEIABI_crc16(msg,l);
	bzero(buffer,buflen);
	buffer[0]=0x02;
	for(i=0;i<l;i++)
		buffer[1+i]=msg[i];

	crclength=putcrc(&buffer[1+l],crc);
	buffer[1+l+crclength]=0x03;
	sendlength=2+l+crclength;

	my_printf("input:\n");
	for(i=0;i<sendlength;i++)
		my_printf("%02x ",buffer[i]);
	my_printf("\n");
	if (!send(sockfd,buffer,sendlength,0)) 
		return(-1);


	bzero(buffer,buflen);
	received=0;
	numbytes=0;
	inizio=time(NULL);
	do
	{
		numbytes=recv(sockfd, &buffer[received], buflen-received, MSG_DONTWAIT);
		if(numbytes>0)
			received+=numbytes;
	}
	while(time(NULL)-inizio<timeout);

	if(received>0)
	{
		my_printf("output %d\n",received);
		while(lout=getCEIABILine(buffer,bufout,&received))
		{	
			out=1;

			if(lout>8)
			{
				
			}

			my_printf("line: %d\n",lout);
			for(i=0;i<lout;i++)
				my_printf("%02x ",bufout[i]);
			my_printf("\n");
			lout=cleanCEIABILine(bufout,lout);
			for(i=0;i<lout;i++)
				my_printf("%02x ",bufout[i]);
			my_printf("\n");
			my_printf("end of line:\n");
		}
	}
	else
		my_printf("%d\n",received);
	return(out);
}



void CEIABIGatewayDisconnected(int id)
/*--------------------------------------------
 * scrive su knxTable -1
 * chiamata quando non "sento" la scheda
 * int knxGatewayId - identificativo device [0-NUMKNXGATEWAYS-1]
 * -----------------------------------------*/
{
	int i,j;

	CEIABIGateways[id].failures=0;
	if((CEIABIGateways[id].enabled)
			&&(CEIABIGateways[id].status!='3'))
		CEIABIGateways[id].status='2';
}


int id_CEIABI_gatewayToId(int id_CEIABI_gateway)
{
	int i;
	
	for(i=0;i<NUMCEIABIGATEWAYS;i++)
		if(CEIABIGateways[i].id_CEIABI_gateway==id_CEIABI_gateway)
			break;
	return((i<NUMCEIABIGATEWAYS)?i:-1);
}

int loadCEIABI(bool reload)
{
	MYSQL_RES *result;
	MYSQL_RES *resultLines;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	char query[5120];
	MYSQL *conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}


	strcpy(query,"SELECT ceiabi_gateway.id,"
					"ceiabi_gateway.address,"
					"ceiabi_gateway.port,"
					"ceiabi_gateway.description,"
					"ceiabi_gateway.enabled"
					" FROM ceiabi_gateway"
					" ORDER BY ceiabi_gateway.id");

	state = mysql_query(conn, query);

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);

	if(reload)
	{
		if(shmdt(CEIABIGateways))
			my_perror("shmdt");
		shmctl(SHMCEIABIGATEWAYSID, IPC_RMID, &shmid_struct);
	}
	NUMCEIABIGATEWAYS=mysql_num_rows(result);
	if(NUMCEIABIGATEWAYS)
	{
		my_printf("get_shared_memory_segment: CEIABIGateways\n");
		CEIABIGateways=(struct CEIABIGateway *)get_shared_memory_segment(NUMCEIABIGATEWAYS * sizeof(struct CEIABIGateway), &SHMCEIABIGATEWAYSID, "/dev/zero");
		if(!CEIABIGateways)
			return -1;
	}
	else
		CEIABIGateways=0;

	i=0;

	while((row=mysql_fetch_row(result))!=NULL)
	{
		CEIABIGateways[i].id_CEIABI_gateway=atoi(row[0]);
		if(row[1])
			strcpy(CEIABIGateways[i].address,row[1]);
		else
			strcpy(CEIABIGateways[i].address,"");
		CEIABIGateways[i].port=atoi(row[2]);
		if(row[3])
			strcpy(CEIABIGateways[i].description,row[3]);
		else
			strcpy(CEIABIGateways[i].description,"");
		
		CEIABIGateways[i].enabled=atoi(row[4]);

		if(!reload)
		{
			if(CEIABIGateways[i].enabled)
				CEIABIGateways[i].status='1';
			else
				CEIABIGateways[i].status='d';
		}
		CEIABIGateways[i].failures=0;
		CEIABIGateways[i].in_ch=0;
		i++;
	}
	mysql_free_result(result);

	return(0);
}
 
void updateCEIABI(int id_CEIABI_gateway)
{
	int state;
	int id;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i;
	MYSQL *conn;

	id=id_CEIABI_gatewayToId(id_CEIABI_gateway);
	if(id==-1)
	return;

	conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return;
	}
	CEIABIGateways[id].enabled=0;

	sprintf(query,"SELECT ceiabi_gateway.address,knx_gateway.port,knx_gateway.enabled"
								" FROM ceiabi_gateway "
								" WHERE ceiabi_gateway.id=%d",id_CEIABI_gateway);

	state = mysql_query(conn,query);

	if( state != 0 )
		my_printf("%s\n",mysql_error(conn));
	result = mysql_store_result(conn);
	
	if (( row = mysql_fetch_row(result)) != NULL )
	{
		if(row[0])
			strcpy(CEIABIGateways[id].address,row[0]);
		else
			strcpy(CEIABIGateways[id].address,"");
		CEIABIGateways[id].port=atoi(row[1]);
		CEIABIGateways[id].enabled=atoi(row[2]);
		if(CEIABIGateways[id].enabled)
			CEIABIGateways[id].enabled=2;	//force restart
	}
	mysql_free_result(result);
	mysql_close(conn);
}

void checkCEIABIReload(int id)
{				
	if(CEIABIGateways[id].enabled>1)
	{
		my_printf("requested CEIABI reload, address %s port %d\n",
						CEIABIGateways[id].address,CEIABIGateways[id].port);
		close(CEIABIGateways[id].sockfd);
		CEIABIGateways[id].sockfd=-1;
		CEIABIGateways[id].enabled=1;
		CEIABIGateways[id].status='1';
	}
}


WORD CEIABI_crc16(BYTE *Buffer,WORD Len)
{
	WORD d0,d1,d2;
	d0=0xffff;
	for(d1=0;d1<Len;d1++)
	{
		d2=Buffer[d1] & 0xff;
		d0^=d2;
		for(d2=0;d2<8;d2++)
		{
			if(d0 & 1)
			{
				d0 >>= 1;
				d0 ^= 0xA001;
			}
			else
			d0 >>= 1;
		}
	}
	return (d0);
}


int putcrc(BYTE *buffer,WORD crc)
{
	int out=0;
	BYTE cur;
	cur=crc>>8;
	if((cur==0x10)||(cur==0x02)||(cur==0x03))
	{
		buffer[0]=0x10;
		out++;
	}
	buffer[out]=cur;
	out++;

	cur=crc & 0xff;
	if((cur==0x10)||(cur==0x02)||(cur==0x03))
	{
		buffer[out]=0x10;
		out++;
	}
	buffer[out]=cur;
	out++;
	return out;
}

int cleanCEIABILine(unsigned char *buf, int len)
{
	int i;

	i=0;
	while((i<len)&&(buf[i]!=0x02))
		i++;
	if(i>0)
	{
		memcpy(&buf[0],&buf[i],len-i);
		len-=i;
	}

	i=0;
	bool is_10=0;

	for(i=0;i<len;i++)
	{
		if(is_10)
		{
			memcpy(&buf[i-1],&buf[i],len-i);
			len--;
			is_10=0;
		}
		else
			is_10=(buf[i]==0x10);
	}
	return len;
}

int getCEIABILine(unsigned char *buf,unsigned char *bufout, int *len)
{
	int i;
	bool esci=0;
	bool is_10=0;
	int lout;

	bzero(bufout,sizeof(bufout));
	lout=*len;
	i=0;

	while((i<lout)&&(!esci))
	{
		if(!is_10)
		{
			is_10=(buf[i]==0x10);
			esci=(buf[i]==0x02);
		}
		else
			is_10=0;
		if(!esci)
			i++;
	}
	memcpy(bufout,&buf[i],lout-i);
	lout-=i;

	i=0;
	is_10=0;
	esci=0;
	while((i<lout)&&(!esci))
	{
		if(!is_10)
		{
			is_10=(bufout[i]==0x10);
			esci=(bufout[i]==0x03);
		}
		else
			is_10=0;
		i++;
	}
	memcpy(buf,&bufout[i],lout-i);
	*len=lout-i;

	lout=i;
	return lout;
}

DWORD CEIABI_totime(unsigned char *buf)
{
	DWORD offset=631173600;
	DWORD out;
	int i;

	bzero(&out,sizeof(out));
	for(i=0;i<4;i++)
		out=(out<<8)+buf[i];

	out+=offset;

	return out;
}


