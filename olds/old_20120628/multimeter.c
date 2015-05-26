#include "multimeter.h"

void doMultimeter(int l)
{
	int i,out;
	struct msgform msg;

	if(multimeters[l].enabled)
		multimeters[l].status='1'; //connecting
	printf("trying to connect to %s port %d\n",
		multimeters[l].address,multimeters[l].port);
	multimeters[l].sockfd=doConnectMultimeter(l);
	printf("address %s, port %d connected\n",multimeters[l].address, multimeters[l].port);

	i=0;
	do
	{
		out=operateMultimeter(multimeters[l].sockfd,multmeterNumToId(multimeters[l].multimeter_num,NUMMULTIMETERS),2,i);

		if(out==-1)
		{
			formatMessage(msg.mtext,0,0,multimeters[l].multimeter_num,0,"MULTIMETER DISCONNECTED");
			msg.mtype=1;
			msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);

			multimeterDisconnected(l);
			close(multimeters[l].sockfd);
			multimeters[l].sockfd=-1;
			if(multimeters[l].enabled)
			{
				printf("error in %s %d\nsleeping 5 seconds\n",multimeters[l].address,multimeters[l].port);
				sleep(5);
				printf("trying to connect\n");
			}
			multimeters[l].sockfd=doConnectMultimeter(l);
			printf("address %s port %d connected\n",multimeters[l].address, multimeters[l].port);
		}
		else
			multimeters[l].status='0';	//connected
		i=(i+1)%2;
		usleep(500000);	//intervallo interrogazione scheda
	}
	while(1);
}

int doConnectMultimeter(int multimeterId)
/*--------------------------------------------
 * gestisce la connessione/disconnessione dei multimetri
 * riprova ogni 10 secondi
 * int multimeterId - identificatore multimetro [0-NUMMULTIMETERS-1]
 * -----------------------------------------*/
{
	int out;
	int attempt=0;
	struct msgform msg;	

	do
	{
//		printf("trying to connect to %s\n",multimeters[multimeterId].address);
		out=0;
		if(multimeters[multimeterId].enabled==0)
			out=-1;
		else
			out=socketConnect(multimeters[multimeterId].address,multimeters[multimeterId].port);
		if(out!=-1)
			attempt=0;
		else
		{
			if(multimeters[multimeterId].enabled)
			{
				attempt++;
/*				printf("%s %d: attempt %d failed\n",
					multimeters[multimeterId].address,
					multimeters[multimeterId].port,attempt);
*/
			}
			if(attempt>MAXATTEMPTS)
			{
				multimeters[multimeterId].status='3';	//failed after maxattempts
				attempt=0;
			}
			multimeterDisconnected(multimeterId);
			sleep(10);
		}
	}
	while(out<0);

	multimeters[multimeterId].failures=0;
	formatMessage(msg.mtext,0,0,multimeters[multimeterId].multimeter_num,0,"MULTIMETER CONNECTED");
	msg.mtype=1;
	msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);

	return out;
}

int operateMultimeter(int sockfd,int multimeterId, int timeout, int flip)
/*--------------------------------------------
 * scrive su socket, attende risposta, aggiorna tabelle
 * int sockfd 	- descrittore del socket
 * int multimeterId - identificatore device [0-NUMMULTIMETRI-1]
 * int timeout	- timeout di attesa (tipicamente 2 secondi)
 * -----------------------------------------*/
{
	bool headerok;
	int sendLen;
	int out=0;
	int received;
	int recvLen;
	int i;
	int j;
	int k;
	int numbytes;
	unsigned char *recvBuf;
	int readingId;

	int ora_ini;
	int ora_fine;
	int reading;

	time_t inizio,curtime;
	int minutes;
	struct timeval tv;
	struct tm ts;

	unsigned char in_bytes_length;
	unsigned char *in_bytes;
	unsigned char out_ch;
	int step;

	if(flip==0)
	{
		in_bytes_length=multimeters[multimeterId].in_bytes_1_length;
		in_bytes=multimeters[multimeterId].in_bytes_1;
		out_ch=multimeters[multimeterId].out_ch_1;
		step=0;
	}
	else
	{
		in_bytes_length=multimeters[multimeterId].in_bytes_2_length;
		in_bytes=multimeters[multimeterId].in_bytes_2;
		out_ch=multimeters[multimeterId].out_ch_2;
		step=multimeters[multimeterId].out_ch_1;
	}

	if(multimeters[multimeterId].enabled>2)	//try to reload
	{
		multimeters[multimeterId].status='1';	//loading
		inizio=time(NULL);
		do
		{
			usleep(5000);
		}
		while((multimeters[multimeterId].enabled>2)&&(time(NULL)-inizio<timeout));
		if(multimeters[multimeterId].enabled>2)
			multimeters[multimeterId].enabled-=3;
	}
	if(multimeters[multimeterId].enabled!=1)
	{
		if(multimeters[multimeterId].enabled>1)
			multimeters[multimeterId].enabled=1;
		else
			multimeters[multimeterId].status='d';
		return -1;
	}

	if (!send(sockfd,in_bytes,in_bytes_length,0)) 
	{
		printf("error in sending\n");
		out=-1;
	}
	else
	{
		recvLen=4*out_ch+5;
		recvBuf=(unsigned char *)malloc(recvLen);
		received=0;
		inizio=time(NULL);
		do
		{
			if ((numbytes=recv(sockfd, &recvBuf[received], recvLen-received, 0)) > 0)
				received+=numbytes;
			if(numbytes==0)
				usleep(1000);
		}
		while((received<recvLen)&&(time(NULL)-inizio<timeout));

		if(received==recvLen)
		{
			multimeters[multimeterId].failures=0;
			headerok=1;
			for(i=0;i<multimeters[multimeterId].header_out_length;i++)
				headerok&=(recvBuf[i]==multimeters[multimeterId].header_out[i]);
			if(headerok)
			{
				for(i=0;i<out_ch;i++)
				{
					readingId=deviceToid[3][multimeterId][i+step];

					if((3+4*i+3<(recvLen-1))
						&&(readingTable[readingId].id_reading!=-1)
							&&(readingTable[readingId].enabled==1))
					{
						reading=0;

						for(j=0;j<4;j++)
							reading+=(recvBuf[3+4*i+j] << (8*(3-j)));


						readingTable[readingId].value=reading;
//						printf("reading: %d value: %d\n",readingId,readingTable[readingId].value);
					}
					else
					{
						printf("3+4*i+3=%d - recvLen=%d - id_reading=%d - enabled=%d\n",(3+4*i+3),recvLen,readingTable[i].id_reading,readingTable[i].enabled);
					}
				}
			}
		}
		else
		{
			multimeters[multimeterId].failures++;
			printf("received: %d - recvLen:%d - failures:%d\n",received,recvLen,multimeters[multimeterId].failures);
			if(multimeters[multimeterId].failures==3)
				out=-1;
		}
		free(recvBuf);
	}

	return(out);
}

int multmeterNumToId(int multimeterNum, int totale)
/*--------------------------------------------
 * trova l'indice in tabella in memoria multimeters 
 * relativo al valore multimeter_num in db
 * int multimeterNum - valore in db
 * totale		 - conteggio totale devices
 * -----------------------------------------*/
{
	int i=0;
	while(i<totale && multimeters[i].multimeter_num!=multimeterNum)
		i++;
	
	return(i<totale?i:-1);
}


void multimeterDisconnected(int multimeterId)
/*--------------------------------------------
 * scrive su readings -1
 * chiamata quando non "sento" la scheda
 * int multimeterId - identificativo device [0-NUMMULTIMETERS-1]
 * -----------------------------------------*/
{
	int i,j,id;

	multimeters[multimeterId].failures=0;
	if((multimeters[multimeterId].enabled)
			&&(multimeters[multimeterId].status!='3'))
		multimeters[multimeterId].status='2';
	for(j=0;j<multimeters[multimeterId].out_ch_1+multimeters[multimeterId].out_ch_2;j++)
	{
		id=deviceToid[3][multimeterId][j];
		resetReadingValues(id);
	}
}

int loadMultimeterTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	char query[512];
	
	strcpy(query,"SELECT mm.multimeter_num,"
					"mm.tcp_ip,"
					"mm.port,"
					"mm_type.in_bytes_1,"
					"mm_type.in_bytes_2,"
					"mm_type.out_ch_1,"
					"mm_type.out_ch_2,"
					"mm_type.header_in,"
					"mm_type.header_out,"
					"mm.enabled"
								" FROM mm LEFT JOIN mm_type ON mm.mm_type_id=mm_type.id"
								" WHERE mm.removed=0"
								" ORDER BY mm.multimeter_num");
	state = mysql_query(connection, query);

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

	if(!onlyupdate)
	{
		NUMMULTIMETERS=mysql_num_rows(result);
		if(NUMMULTIMETERS>0)
		{
			printf("get_shared_memory_segment: multimeters\n");
			multimeters=(struct multimeter *)get_shared_memory_segment(NUMMULTIMETERS * sizeof(struct multimeter), &SHMMULTIMETERSID, "/dev/zero");
			if(!multimeters)
				die("multimeters - get_shared_memory_segment\n");
		}
		else
			multimeters=0;
	}
	i=0;

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		multimeters[i].multimeter_num=atoi(row[0]);
		if(row[1])
			strcpy(multimeters[i].address,row[1]);
		else
			strcpy(multimeters[i].address,"");
		multimeters[i].port=atoi(row[2]);

		multimeters[i].in_bytes_1_length=(int)(strlen(row[3])/2);
		multimeters[i].in_bytes_1=(unsigned char *)malloc(multimeters[i].in_bytes_1_length);
		for(k=0;k<multimeters[i].in_bytes_1_length;k++)
			multimeters[i].in_bytes_1[k]=parse_int(&row[3][2*k]);

		multimeters[i].in_bytes_2_length=(int)(strlen(row[4])/2);
		multimeters[i].in_bytes_2=(unsigned char *)malloc(multimeters[i].in_bytes_2_length);
		for(k=0;k<multimeters[i].in_bytes_2_length;k++)
			multimeters[i].in_bytes_2[k]=parse_int(&row[4][2*k]);

		multimeters[i].out_ch_1=atoi(row[5]);
		multimeters[i].out_ch_2=atoi(row[6]);

		multimeters[i].header_in_length=(int)(strlen(row[7])/2);
		multimeters[i].header_in=(unsigned char *)malloc(multimeters[i].header_in_length);
		for(k=0;k<multimeters[i].header_in_length;k++)
			multimeters[i].header_in[k]=parse_int(&row[7][2*k]);

		multimeters[i].header_out_length=(int)(strlen(row[8])/2);
		multimeters[i].header_out=(unsigned char *)malloc(multimeters[i].header_out_length);
		for(k=0;k<multimeters[i].header_out_length;k++)
			multimeters[i].header_out[k]=parse_int(&row[8][2*k]);

		multimeters[i].enabled=atoi(row[9]);
		multimeters[i].failures=0;



		if(!onlyupdate)
		{
			if(multimeters[i].enabled)
				multimeters[i].status='1';
			else
				multimeters[i].status='d';

			READINGCHANNELS+=(multimeters[i].out_ch_1+multimeters[i].out_ch_2);
		}
		i++;
	}
	mysql_free_result(result);
	return(0);
}

int loadReadingTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;

	if(!onlyupdate)
	{
		if(READINGCHANNELS>0)
		{
			printf("get_shared_memory_segment: readingTable\n");
			readingTable=(struct readingLine *)get_shared_memory_segment(READINGCHANNELS * sizeof(struct readingLine), &SHMREADINGID, "/dev/zero");
			if(!readingTable)
				die("readingTable - get_shared_memory_segment\n");
		}
		else
			readingTable=0;
	}
	initializeReadingTable();
	state = mysql_query(connection, "SELECT mm_reading.id_reading,"
										"mm_reading.description,"
										"mm_reading.multimeter_num,"
										"mm_reading.ch_num,"
										"mm_reading.form_label,"
										"mm_reading.label,"
										"mm_reading.sinottico,"
										"mm_reading.printer,"
										"mm_reading.alarm_value,"
										"mm_reading.msg,"
										"mm_reading.enabled,"
										"mm_reading.sms,"
										"mm_reading.msg_is_event "
						"FROM mm_reading JOIN mm ON mm_reading.multimeter_num=mm.multimeter_num "
						"WHERE mm.removed=0 AND mm_reading.ch_num!=0"
						" ORDER BY mm_reading.multimeter_num, mm_reading.ch_num");

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		i=multmeterNumToId(atoi(row[2]),NUMMULTIMETERS);
		if((i!=-1) && (atoi(row[3])<=(multimeters[i].out_ch_1+multimeters[i].out_ch_2)))
		{
			if(!onlyupdate)
				deviceToid[3][i][atoi(row[3]) - 1]=idRow;
			else
				idRow=deviceToid[3][i][atoi(row[3]) - 1];

			readingTable[idRow].id_reading=atoi(row[0]);
			if(row[1])
				safecpy(readingTable[idRow].description,row[1]);
			else
				safecpy(readingTable[idRow].description,"");
			readingTable[idRow].multimeter_num=atoi(row[2]);
			readingTable[idRow].ch_num=atoi(row[3]);
			if(row[4])
				safecpy(readingTable[idRow].form_label,row[3]);
			else
				safecpy(readingTable[idRow].form_label,"");
			if(row[5])
				safecpy(readingTable[idRow].label,row[5]);
			else
				safecpy(readingTable[idRow].label,"");
			if(row[6])
				safecpy(readingTable[idRow].sinottico,row[6]);
			else
				safecpy(readingTable[idRow].sinottico,"");
			readingTable[idRow].printer=atoi(row[7]);
			readingTable[idRow].alarm_value=atoi(row[8]);
			if(row[9])
				safecpy(readingTable[idRow].msg,row[9]);
			else
				safecpy(readingTable[idRow].msg,"");
			readingTable[idRow].enabled=atoi(row[10]);
			readingTable[idRow].sms=atoi(row[11]);
			readingTable[idRow].msg_is_event=atoi(row[12]);
			idRow++;

		}
	}
	mysql_free_result(result);
	if(!onlyupdate)
	{
		for(i=0;i<NUMMULTIMETERS;i++)
			for(k=0;k<multimeters[i].out_ch_1+multimeters[i].out_ch_2;k++)
				if(deviceToid[3][i][k]==-1)
				{
					deviceToid[3][i][k]=idRow;
					idRow++;
				}
	
		if(idRow!=READINGCHANNELS)
			printf("there's a problem with multimeter readings\n");
	}
	return(0);
}

void initializeReadingTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella reading
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<READINGCHANNELS;i++)
	{
		readingTable[i].id_reading=-1;
		strcpy(readingTable[i].form_label,"");
		strcpy(readingTable[i].description,"");
		strcpy(readingTable[i].label,"");
		strcpy(readingTable[i].sinottico,"");
		readingTable[i].multimeter_num=-1;
		readingTable[i].ch_num=-1;
		readingTable[i].printer=0;
		readingTable[i].sms=0;
		readingTable[i].alarm_value=-1;
		strcpy(readingTable[i].msg,"");
		readingTable[i].msg_is_event=0;
		readingTable[i].enabled=0;

		resetReadingValues(i);
	}
}



void resetReadingValues(int i)
{
	readingTable[i].value=-1;
}


int id_readingToId(int id_reading)
{
	int i;
	
	for(i=0;i<READINGCHANNELS;i++)
		if(readingTable[i].id_reading==id_reading)
			break;
	return((i<READINGCHANNELS)?i:-1);
}

