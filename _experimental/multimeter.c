#include "multimeter.h"

void doMultimeter(int l)
{
	int i,out;
	struct msgform msg;

	i=0;
	do
	{
		if(multimeters[l].enabled)
		{
			checkMultimeterReload(l);
			if((multimeters[l].status!='0')||(multimeters[l].sockfd==-1))
			{
				multimeters[l].status='1'; //connecting
				my_printf("trying to connect to multimeters %s %d\n",
					multimeters[l].address,multimeters[l].port);

				multimeters[l].sockfd=doConnectMultimeter(l);
				if(multimeters[l].sockfd!=-1)
					my_printf("address %s, port %d, multimeter connected\n",multimeters[l].address, multimeters[l].port);
			}
			if(multimeters[l].sockfd!=-1)
			{
				out=operateMeter(l,2,i);
				if(out==-1)
				{
					handleMessage(MULTIMETER,0,multimeters[l].multimeter_num,
						0,"ERROR OPERATING, SLEEPING 5 SECONDS",0,ALARM_AS_ALARM);
		
					multimeterDisconnected(l);
					close(multimeters[l].sockfd);
					multimeters[l].sockfd=-1;
	
					my_printf("error in multimeter %s %d\nsleeping 5 seconds\n",
						multimeters[l].address,multimeters[l].port);
					sleep(5);
				}
				else
				{
					if(multimeters[l].status!='0')
						handleMessage(MULTIMETER,0,multimeters[l].multimeter_num,
							0,"OPERATING OK",0,ALARM_BACK_IN_RANGE);
					multimeters[l].status='0';	//operating ok
					storeMultimeterTable(multimeters[l].multimeter_num);
				}
				i=(i+1)%2;
			}
		}
		else
			multimeters[l].status='d';
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
//		my_printf("trying to connect to %s\n",multimeters[multimeterId].address);
		out=0;
		if(multimeters[multimeterId].enabled==0)
			out=-1;
		else
		{
			my_printf("trying to connect to %s port %d\n",
				multimeters[multimeterId].address,multimeters[multimeterId].port);
			out=socketConnect(multimeters[multimeterId].address,multimeters[multimeterId].port);
		}
		if(out!=-1)
			attempt=0;
		else
		{
			if(multimeters[multimeterId].enabled)
			{
				attempt++;
/*				my_printf("%s %d: attempt %d failed\n",
					multimeters[multimeterId].address,
					multimeters[multimeterId].port,attempt);
*/
			}
			if(attempt>MAXATTEMPTS)
			{
				if(multimeters[multimeterId].status!='3')
					handleMessage(MULTIMETER,0,
						multimeters[multimeterId].multimeter_num,0,
						"MULTIMETER DISCONNECTED",0,ALARM_AS_ALARM);
				multimeters[multimeterId].status='3';	//failed after maxattempts
				attempt=0;
			}
			multimeterDisconnected(multimeterId);
			sleep(10);
		}
	}
	while(out<0);

	multimeters[multimeterId].failures=0;
	handleMessage(MULTIMETER,0,multimeters[multimeterId].multimeter_num,0,"MULTIMETER CONNECTED",0,ALARM_BACK_IN_RANGE);

	return out;
}

int operateMultimeter(int multimeterId, int timeout, int flip)
/*--------------------------------------------
 * scrive su socket, attende risposta, aggiorna tabelle
 * int sockfd 	- descrittore del socket
 * int multimeterId - identificatore device [0-NUMMULTIMETRI-1]
 * int timeout	- timeout di attesa (tipicamente 2 secondi)
 * -----------------------------------------*/
{
	char errorBuf[255];
	int sockfd;
	time_t inizio;
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
	int raw_length;

	int ora_ini;
	int ora_fine;
	long double reading;

	unsigned char in_bytes_length;
	unsigned char *in_bytes;
	unsigned char out_ch;
	int step;

	sockfd=multimeters[multimeterId].sockfd;
	if(flip==0)
	{
		in_bytes_length=multimeters[multimeterId].in_bytes_1_length;
		in_bytes=multimeters[multimeterId].in_bytes_1;
		out_ch=multimeters[multimeterId].out_ch_1;
		recvLen=multimeters[multimeterId].out_ch_1_bytes;
		step=0;
	}
	else
	{
		in_bytes_length=multimeters[multimeterId].in_bytes_2_length;
		in_bytes=multimeters[multimeterId].in_bytes_2;
		out_ch=multimeters[multimeterId].out_ch_2;
		recvLen=multimeters[multimeterId].out_ch_2_bytes;
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
		my_printf("error in sending\n");
		out=-1;
	}
	else
	{
		recvBuf=(unsigned char *)malloc(recvLen);
		received=0;
		inizio=time(NULL);
		do
		{
			if ((numbytes=recv(sockfd, &recvBuf[received], recvLen-received, 0)) > 0)
				received+=numbytes;
			if(numbytes==0)
			{
				usleep(1000);
			}
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
				j=3;
				for(i=0;i<out_ch;i++)
				{
					readingId=deviceToid[3][multimeterId][i+step];
					raw_length=readingTable[readingId].raw_length;

					if((j+raw_length<=recvLen)
						&&(readingTable[readingId].id_reading!=-1)
							&&(readingTable[readingId].enabled==1))
					{
						reading=0;

//						readingTable[readingId].raw_length=4;
						memcpy(readingTable[readingId].raw,&recvBuf[j],raw_length);
						reading=convertBufferToFloat(readingTable[readingId].raw,raw_length,readingTable[readingId].unsign,readingTable[readingId].res);

//gestione allarmi
						mmHandleAlarms(readingId,reading);
						
//fine gestione allarmi
						readingTable[readingId].value_valid=TRUE;
						readingTable[readingId].value=reading;

//gestione max e min del giorno
						mmHandleMaxMin(readingId,reading);
//						my_printf("reading: %d value: %d\n",readingId,readingTable[readingId].value);

					}
					else
					{
						my_printf("i+raw_length=%d - recvLen=%d - id_reading=%d - enabled=%d\n",
							(j+raw_length),recvLen,readingTable[readingId].id_reading,readingTable[readingId].enabled);
					}
					j+=raw_length;
				}
			}
		}
		else
		{
			for(i=0;i<received;i++)
				sprintf(&errorBuf[3*i],"%02x ",recvBuf[i]);
			recvBuf[3*received]='\0';
			multimeters[multimeterId].failures++;
			my_printf("received: %d - recvLen:%d - failures:%d\nReceiced: %s\n",
				received,recvLen,multimeters[multimeterId].failures,errorBuf);
			
			if(multimeters[multimeterId].failures==3)
				out=-1;
		}
		free(recvBuf);
	}
	return(out);
}

long double convertBufferToFloat(unsigned char *raw,int raw_length,int unsign,int res)
{
	long double reading=0;
	long long readingInt=0;
	unsigned long long readingUnsigned=0;
	int k;
	int i;


	k=sizeof(long long);
	if(unsign)
	{
		for(i=0;i<raw_length;i++)
			readingUnsigned|=(((unsigned long long)raw[i])<<(8*(raw_length-i-1)));
		reading=(long double)readingUnsigned;
	}
	else
	{
		if(raw[0]>>7)
		{
			for(i=0;i<k-raw_length;i++)
				readingInt|=(0xffLL<<(8*(k-i-1)));
		}
		for(i=0;i<raw_length;i++)
			readingInt|=(((unsigned long long)raw[i])<<(8*(raw_length-i-1)));
		reading=(long double)readingInt;
	}

	for(k=0;k<res;k++)
		reading=reading*10;
	for(k=res;k<0;k++)
		reading=reading/10;
	return reading;
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
	MYSQL *conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return (1);
	}

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
	state = mysql_query(conn, query);

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);

	if(!onlyupdate)
	{
		NUMMULTIMETERS=mysql_num_rows(result);
		if(NUMMULTIMETERS>0)
		{
			my_printf("get_shared_memory_segment: multimeters\n");
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
		multimeters[i].out_ch_1_bytes=5;
		multimeters[i].out_ch_2_bytes=5;

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
	mysql_close(conn);
	return(0);
}

int loadReadingTable(bool onlyupdate)
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
		if(READINGCHANNELS>0)
		{
			my_printf("get_shared_memory_segment: readingTable\n");
			readingTable=(struct readingLine *)get_shared_memory_segment(READINGCHANNELS * sizeof(struct readingLine), &SHMREADINGID, "/dev/zero");
			if(!readingTable)
				die("readingTable - get_shared_memory_segment\n");
		}
		else
			readingTable=0;
	}
	initializeReadingTable();
	state = mysql_query(conn, "SELECT mm_reading.id_reading,"
										"mm_reading.description,"
										"mm_reading.multimeter_num,"
										"mm_reading.ch_num,"
										"mm_reading.form_label,"
										"mm_reading.label,"
										"mm_reading.sinottico,"
										"mm_reading.printer,"
										"mm_reading.enabled,"
										"mm_reading.msg_is_event,"
										"al_high_value,"
										"al_high_active,"
										"al_high_msg,"
										"time_delay_high_on,"
										"time_delay_high_off,"
										"sms_high_active,"
										"al_low_value,"
										"al_low_active,"
										"al_low_msg,"
										"time_delay_low_on,"
										"time_delay_low_off,"
										"sms_low_active,"
										"unit,"
										"size,"
										"res,"
										"unsign,"
										"record_data_time "
						"FROM mm_reading JOIN mm ON mm_reading.multimeter_num=mm.multimeter_num "
						"WHERE mm.removed=0 AND mm_reading.ch_num!=0"
						" ORDER BY mm_reading.multimeter_num, mm_reading.ch_num");

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);

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
			readingTable[idRow].enabled=atoi(row[8]);
			readingTable[idRow].msg_is_event=atoi(row[9]);
			readingTable[idRow].al_high_value=atoi(row[10]);
			readingTable[idRow].al_high_active=atoi(row[11]);
			if(row[12])
				safecpy(readingTable[idRow].al_high_msg,row[12]);
			else
				safecpy(readingTable[idRow].al_high_msg,"");

			readingTable[idRow].time_delay_high_on=atoi(row[13]);
			readingTable[idRow].time_delay_high_off=atoi(row[14]);
			readingTable[idRow].sms_high_active=atoi(row[15]);
			readingTable[idRow].al_low_value=atoi(row[16]);
			readingTable[idRow].al_low_active=atoi(row[17]);
			if(row[18])
				safecpy(readingTable[idRow].al_low_msg,row[18]);
			else
				safecpy(readingTable[idRow].al_low_msg,"");

			readingTable[idRow].time_delay_low_on=atoi(row[19]);
			readingTable[idRow].time_delay_low_off=atoi(row[20]);
			readingTable[idRow].sms_low_active=atoi(row[21]);
			if(row[22])
				safecpy(readingTable[idRow].unit,row[22]);
			else
				safecpy(readingTable[idRow].unit,"");
			readingTable[idRow].raw_length=0;
			if(row[23])
				readingTable[idRow].raw_length=2*atoi(row[23]);
			readingTable[idRow].res=0;
			if(row[24])
				readingTable[idRow].res=atoi(row[24]);
			readingTable[idRow].unsign=0;
			if(row[25])
				readingTable[idRow].unsign=atoi(row[25]);
			readingTable[idRow].record_data_time=(atoi(row[26])!=-1?60*atoi(row[26]):RECORDDATATIME);
			memset(readingTable[idRow].raw,0,20);
//			readingTable[idRow].raw_length=0;


			if(readingTable[idRow].ch_num<=multimeters[i].out_ch_1)
				multimeters[i].out_ch_1_bytes+=readingTable[idRow].raw_length;
			else
				multimeters[i].out_ch_2_bytes+=readingTable[idRow].raw_length;

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
			my_printf("there's a problem with multimeter readings\n");
	}
	mysql_close(conn);
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
		readingTable[i].al_high_value=0;
		readingTable[i].al_high_active=FALSE;
		strcpy(readingTable[i].al_high_msg,"");
		readingTable[i].time_delay_high_on=0;
		readingTable[i].time_delay_high_off=0;
		readingTable[i].sms_high_active=FALSE;
		readingTable[i].al_low_value=0;
		readingTable[i].al_low_active=FALSE;
		strcpy(readingTable[i].al_low_msg,"");
		readingTable[i].time_delay_low_on=0;
		readingTable[i].time_delay_low_off=0;
		readingTable[i].sms_low_active=FALSE;
		strcpy(readingTable[i].unit,"");
		readingTable[i].record_data_time=0;
		readingTable[i].time_high_on=0;
		readingTable[i].time_high_off=0;
		readingTable[i].time_low_on=0;
		readingTable[i].time_low_off=0;
		readingTable[i].msg_is_event=FALSE;

		readingTable[i].value_max=0;
		readingTable[i].value_min=0;
		readingTable[i].ts_min=0;
		readingTable[i].ts_max=0;
		readingTable[i].stored_time=time(NULL);
		resetReadingValues(i);
	}
}

void resetReadingValues(int i)
{
	memset(readingTable[i].raw,0,20);
//	readingTable[i].raw_length=0;
	readingTable[i].value=-1;
	readingTable[i].value_valid=FALSE;
	readingTable[i].alarm_high_on=FALSE;
	readingTable[i].alarm_low_on=FALSE;
}

int id_readingToId(int id_reading)
{
	int i;
	
	for(i=0;i<READINGCHANNELS;i++)
		if(readingTable[i].id_reading==id_reading)
			break;
	return((i<READINGCHANNELS)?i:-1);
}

int updateMultimeterLine(int id)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i;
	char query[512];
	MYSQL *conn=mysqlConnect();
	if(!conn)
		return(-1);

	i=id_readingToId(id);
	if(i==-1)
		return(-1);

	sprintf(query,"SELECT "
							"mm_reading.description,"
							"mm_reading.form_label,"
							"mm_reading.label,"
							"mm_reading.sinottico,"
							"mm_reading.printer,"
							"mm_reading.enabled,"
							"mm_reading.msg_is_event,"
							"al_high_value,"
							"al_high_active,"
							"al_high_msg,"
							"time_delay_high_on,"
							"time_delay_high_off,"
							"sms_high_active,"
							"al_low_value,"
							"al_low_active,"
							"al_low_msg,"
							"time_delay_low_on,"
							"time_delay_low_off,"
							"sms_low_active,"
							"unit,"
							"record_data_time "
						"FROM mm_reading  "
						"WHERE mm_reading.id_reading='%d'",id);

	state = mysql_query(conn, query);

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(-11);
	}
	result = mysql_store_result(conn);
	if(mysql_num_rows(result)!=1)
		return(-1);

	while((row=mysql_fetch_row(result))!= NULL)
	{
		if(row[0])
			safecpy(readingTable[i].description,row[0]);
		else
			safecpy(readingTable[i].description,"");
		if(row[1])
			safecpy(readingTable[i].form_label,row[1]);
		else
			safecpy(readingTable[i].form_label,"");
		if(row[2])
			safecpy(readingTable[i].label,row[2]);
		else
			safecpy(readingTable[i].label,"");
		if(row[3])
			safecpy(readingTable[i].sinottico,row[3]);
		else
			safecpy(readingTable[i].sinottico,"");
		readingTable[i].printer=atoi(row[4]);
		readingTable[i].enabled=atoi(row[5]);
		readingTable[i].msg_is_event=atoi(row[6]);
		readingTable[i].al_high_value=atoi(row[7]);
		readingTable[i].al_high_active=atoi(row[8]);
		if(row[9])
			safecpy(readingTable[i].al_high_msg,row[9]);
		else
			safecpy(readingTable[i].al_high_msg,"");

		readingTable[i].time_delay_high_on=atoi(row[10]);
		readingTable[i].time_delay_high_off=atoi(row[11]);
		readingTable[i].sms_high_active=atoi(row[12]);
		readingTable[i].al_low_value=atoi(row[13]);
		readingTable[i].al_low_active=atoi(row[14]);
		if(row[15])
			safecpy(readingTable[i].al_low_msg,row[15]);
		else
			safecpy(readingTable[i].al_low_msg,"");

		readingTable[i].time_delay_low_on=atoi(row[16]);
		readingTable[i].time_delay_low_off=atoi(row[17]);
		readingTable[i].sms_low_active=atoi(row[18]);
		if(row[19])
			safecpy(readingTable[i].unit,row[19]);
		else
			safecpy(readingTable[i].unit,"");
		readingTable[i].record_data_time=(atoi(row[20])!=-1?60*atoi(row[20]):RECORDDATATIME);
	}
	mysql_free_result(result);
	mysql_close(conn);
	return(0);
}

void updateMultimeter(int multimeter_num)
{
	int state;
	int id;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i;
	MYSQL *conn=mysqlConnect();
	if(!conn)
		return;

	id=multmeterNumToId(multimeter_num,NUMMULTIMETERS);
	multimeters[id].enabled=0;

	sprintf(query,"SELECT mm.tcp_ip,mm.port,mm.enabled"
								" FROM mm "
								" WHERE mm.multimeter_num=%d",multimeter_num);

	state = mysql_query(conn,query);

	if( state != 0 )
		my_printf("%s\n",mysql_error(conn));
	result = mysql_store_result(conn);
	
	if (( row = mysql_fetch_row(result)) != NULL )
	{
		if(row[0])
			strcpy(multimeters[id].address,row[0]);
		else
			strcpy(multimeters[id].address,"");
		multimeters[id].port=atoi(row[1]);
		multimeters[id].enabled=atoi(row[2]);
		if(multimeters[id].enabled)
			multimeters[id].enabled=2;	//force restart
	}
	mysql_free_result(result);
	mysql_close(conn);
}

void checkMultimeterReload(int id)
{				
	if(multimeters[id].enabled>1)
	{
		my_printf("requested multimeter reload, address %s port %d\n",
						multimeters[id].address,multimeters[id].port);
		close(multimeters[id].sockfd);
		multimeters[id].sockfd=-1;
		multimeters[id].enabled=1;
		multimeters[id].status='1';
	}
}

void storeMultimeterTable(int multimeter_num)
{
	int i;
	MYSQL *conn=mysqlConnect();
	if(conn==NULL)
	{
		printf("%s\n",mysql_error(conn));
		return;
	}

	for(i=0;i<READINGCHANNELS;i++)
	{
		if((multimeters[multmeterNumToId(multimeter_num,NUMMULTIMETERS)].enabled)
			&&(readingTable[i].multimeter_num==multimeter_num)
			&&(readingTable[i].enabled)
			&&(readingTable[i].id_reading!=-1)
			&&(readingTable[i].value_valid)
			&&(time(NULL)-readingTable[i].stored_time>=readingTable[i].record_data_time))
		{
			storeMultimeterLine(i,conn);
			usleep(1000);
		}
	}
	mysql_close(conn);
}

int storeMultimeterLine(int i,MYSQL *conn)
{
	char query[255];

	sprintf(query,"call insertHistory(%d,%d,%d,%f,%d,'%s')",
		MULTIMETER,
		readingTable[i].multimeter_num,
		readingTable[i].id_reading,
		readingTable[i].value,
		-1,
		readingTable[i].unit);
	if(mysql_query(conn,query)!=0)
	{
		my_printf("%s\n%s\n",query,mysql_error(conn));
		return -1;
	}
	else
	{
		readingTable[i].stored_time=time(NULL);
		return 0;
	}
}


void mmHandleMaxMin(int readingId,double reading)
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

	ts_min=readingTable[readingId].ts_min;
	tm_min=localtime(&ts_min);
	d_min=tm_min->tm_mday;
	m_min=tm_min->tm_mon;
	y_min=tm_min->tm_year;

	ts_max=readingTable[readingId].ts_max;
	tm_max=localtime(&ts_max);
	d_max=tm_max->tm_mday;
	m_max=tm_max->tm_mon;
	y_max=tm_max->tm_year;
	if((reading>readingTable[readingId].value_max)
			||(d_cur!=d_max)
			||(m_cur!=m_max)
			||(y_cur!=y_max))
	{
		tostore=TRUE;
		readingTable[readingId].ts_max=ts_cur;
		readingTable[readingId].value_max=reading;
	}

	if((reading<readingTable[readingId].value_min)
			||(d_cur!=d_min)
			||(m_cur!=m_min)
			||(y_cur!=y_min))
	{
		tostore=TRUE;
		readingTable[readingId].ts_min=ts_cur;
		readingTable[readingId].value_min=reading;
	}
	if(tostore)
	{
		conn=mysqlConnect();
		if(!conn)
			return;
		storeMultimeterLine(readingId,conn);
		mysql_close(conn);
	}
}


int operateMeter(int l,int timeout,int flip)
{
	if(multimeters[l].in_bytes_1_length==5)
		return operatePowerMeter(l, timeout);
	else
		return operateMultimeter(l, timeout, flip);
}

int operatePowerMeter(int multimeterId, int timeout)
/*--------------------------------------------
 * scrive su socket, attende risposta, aggiorna tabelle
 * int multimeterId - identificatore device [0-NUMMULTIMETRI-1]
 * int timeout	- timeout di attesa (tipicamente 2 secondi)
 * -----------------------------------------*/
{
	int sockfd;
	time_t inizio;
	int sendLen;
	int out=0;
	int received;
	int recvLen;
	int i;
	int j;
	int k;
	int numbytes;
	unsigned char recvBuf[255];

	int ch_num;
	int readingId;

	int ora_ini;
	int ora_fine;

	unsigned char init_bytes_length;
	unsigned char *init_bytes;
	unsigned char in_bytes_length;
	unsigned char *in_bytes;
	unsigned char out_ch,out_char;
	int offset;

	bool continua;

	FILE *pFile;

	sockfd=multimeters[multimeterId].sockfd;

	init_bytes_length=multimeters[multimeterId].in_bytes_1_length;
	init_bytes=multimeters[multimeterId].in_bytes_1;

	in_bytes_length=multimeters[multimeterId].in_bytes_2_length;
	in_bytes=multimeters[multimeterId].in_bytes_2;

	out_ch=multimeters[multimeterId].out_ch_1+multimeters[multimeterId].out_ch_2;

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

	if (!send(sockfd,init_bytes,init_bytes_length,0)) 
	{
		my_printf("error in sending\n");
		return -1;
	}
	else
	{
		inizio=time(NULL);
		do
		{
			numbytes=recv(sockfd, recvBuf, 255, 0);
			if(numbytes<=0)
				usleep(1000);
		}
		while((numbytes!=1)&&(time(NULL)-inizio<timeout));
		if(numbytes!=1)
		{
			multimeters[multimeterId].failures++;
			my_printf("failure during initialization, numbytes=%d\n",numbytes);
			usleep(1000000);
			if(multimeters[multimeterId].failures==3)
				return -1;
			else
				return 0;
		}

		ch_num=0;
		continua=TRUE;
		in_bytes[1]|=0x20;
		in_bytes[in_bytes_length-2]=pmDoCheckSum(in_bytes,1,in_bytes_length-3);

		while((continua)&&(ch_num<out_ch))	// finchè non trovo 0x0f
		{
			usleep(1000);

			if (!send(sockfd,in_bytes,in_bytes_length,0)) 
			{
				my_printf("error in sending\n");
				return -1;
			}
			received=pmRecvReply(sockfd,recvBuf,255,2);
			if(received==0)
			{
				multimeters[multimeterId].failures++;
				if(multimeters[multimeterId].failures==3)
					return -1;
				else
					return 0;
			}
			offset=19;

			pFile = fopen (multimeters[multimeterId].address, "ab");
			fwrite (recvBuf , sizeof(unsigned char), received, pFile);
			fclose (pFile);

			while((offset<received)&&(recvBuf[offset]!=0x1f)&&(recvBuf[offset]!=0x0f))
			{
				readingId=deviceToid[3][multimeterId][ch_num];

				out_char=pmParseData(recvBuf,received,&offset,readingId);
				ch_num++;
				if((out_char==0)||(ch_num>=out_ch))
				{
					multimeters[multimeterId].failures++;
					if(multimeters[multimeterId].failures==3)
						return -1;
					else
						return 0;
				}
			}
			continua=(out_char==0x1f);
			in_bytes[1]=(in_bytes[1]&0xdf)|(0x20-(in_bytes[1]&0x20));
			in_bytes[in_bytes_length-2]=pmDoCheckSum(in_bytes,1,in_bytes_length-3);
		}
	}
	return(out);
}


void mmHandleAlarms(int readingId,double reading)
{
	static char tempmessage[255];

	if(readingTable[readingId].al_high_active==TRUE)	//sforamento alto
	{
		if(readingTable[readingId].alarm_high_on==FALSE)	//allarme non è ancora attivo
		{
			if(reading>=readingTable[readingId].al_high_value)	//valore alto
			{
				if(readingTable[readingId].value<readingTable[readingId].al_high_value)	//primo sforamento
					readingTable[readingId].time_high_on=time(NULL);

				if(time(NULL)-readingTable[readingId].time_high_on >= readingTable[readingId].time_delay_high_on)	//passato delay
				{
					readingTable[readingId].alarm_high_on=TRUE;
					handleMessage(
							MULTIMETER,
							readingTable[readingId].id_reading,
							readingTable[readingId].multimeter_num,
							0,
							readingTable[readingId].al_high_msg,
							readingTable[readingId].sms_high_active,
							ALARM_AS_ALARM);
					readingTable[readingId].time_high_on=0;
				}
			}
			else
				readingTable[readingId].time_high_on=0;
		}
		if(readingTable[readingId].alarm_high_on==TRUE)	//allarme è attivo
		{
			if(reading<readingTable[readingId].al_high_value)	//valore nella norma
			{
				if(readingTable[readingId].value>=readingTable[readingId].al_high_value)	//primo valore nella norma
					readingTable[readingId].time_high_off=time(NULL);

				if(time(NULL)-readingTable[readingId].time_high_off >= readingTable[readingId].time_delay_high_off)	//passato delay
				{
					readingTable[readingId].alarm_high_on=FALSE;
					sprintf(tempmessage,"%s - value in range: %f %s",
						readingTable[readingId].description,
						readingTable[readingId].value,
						readingTable[readingId].unit);

					handleMessage(
							MULTIMETER,
							readingTable[readingId].id_reading,
							readingTable[readingId].multimeter_num,
							0,
							tempmessage,
							readingTable[readingId].sms_high_active,
							ALARM_BACK_IN_RANGE);
					readingTable[readingId].time_high_off=0;
				}
			}
			else
				readingTable[readingId].time_high_off=0;
		}
	}

	if(readingTable[readingId].al_low_active==TRUE)	//sforamento basso
	{
		if(readingTable[readingId].alarm_low_on==FALSE)	//allarme non è ancora attivo
		{
			if(reading<=readingTable[readingId].al_low_value)	//valore basso
			{
				if(readingTable[readingId].value>readingTable[readingId].al_low_value)	//primo sforamento
					readingTable[readingId].time_low_on=time(NULL);

				if(time(NULL)-readingTable[readingId].time_low_on >= readingTable[readingId].time_delay_low_on)	//passato delay
				{
					readingTable[readingId].alarm_low_on=TRUE;
					handleMessage(
							MULTIMETER,
							readingTable[readingId].id_reading,
							readingTable[readingId].multimeter_num,
							0,
							readingTable[readingId].al_low_msg,
							readingTable[readingId].sms_low_active,
							ALARM_AS_ALARM);
					readingTable[readingId].time_low_on=0;
				}
			}
			else
				readingTable[readingId].time_low_on=0;
		}
		if(readingTable[readingId].alarm_low_on==TRUE)	//allarme è attivo
		{
			if(reading>readingTable[readingId].al_low_value)	//valore nella norma
			{
				if(readingTable[readingId].value<=readingTable[readingId].al_low_value)	//primo valore nella norma
					readingTable[readingId].time_low_off=time(NULL);

				if(time(NULL)-readingTable[readingId].time_low_off >= readingTable[readingId].time_delay_low_off)	//passato delay
				{
					readingTable[readingId].alarm_low_on=FALSE;
					sprintf(tempmessage,"%s - value in range: %f %s",
						readingTable[readingId].description,
						readingTable[readingId].value,
						readingTable[readingId].unit);

					handleMessage(
							MULTIMETER,
							readingTable[readingId].id_reading,
							readingTable[readingId].multimeter_num,
							0,
							tempmessage,
							readingTable[readingId].sms_low_active,
							ALARM_BACK_IN_RANGE);
					readingTable[readingId].time_low_off=0;
				}
			}
			else
				readingTable[readingId].time_low_off=0;
		}
	}

}

unsigned char pmDoCheckSum(unsigned char *buf,int from, int to)
{
	register int i;

	unsigned char out;
	unsigned long temp=0;
	
	for(i=from;i<=to;i++)
		temp+=buf[i];
	out=temp&0xff;
}

unsigned char pmParseData(unsigned char *buf,int len,int *offsetin,int readingId)
{
	int dl=0;
	unsigned char dib,vif;
	bool bcd=0;
	bool isstring;
	int exp;
	int offset;
	int i;
	double reading;

	offset=*offsetin;
	exp=0;
	isstring=0;
	dib=buf[offset];
	dl=pmGetDataLength(dib);
	if(dl==5)
		isstring=TRUE;
	else
		bcd=((dib&0x08)>0);

	while((offset<len)&&((buf[offset]&0x80)>0))
		offset++;
	if(offset==len)
		return(0);
	offset++;
	vif=buf[offset];
	if(vif==0xfd)
	{
	
		offset++;
		vif=buf[offset];
		if((vif&0x70)==0x40)
			exp=(vif&0x0f)-9;
		if((vif&0x70)==0x50)
			exp=(vif&0x0f)-12;
	}
	else
	{
		if(vif==0xff)
		{
			offset++;
			vif=buf[offset];
			if(((vif&0x60)==0x40)||((vif&0x78)==0x60))
				exp=(vif&0x07)-3;
		}
		else
		{
			if((vif&0x78)==0)
				exp=(vif&0x07)-6;//KWH
			if((vif&0x78)==0x28)
				exp=(vif&0x07)-3;//W
		}
	}


	while((offset<len)&&((buf[offset]&0x80)>0))
		offset++;
	if(offset==len)
	{
		return(0);
	}
	offset++;
	if(isstring)
	{
		dl=buf[offset];
		offset++;
	}
	if(offset+dl>len)
		return(0);
	
	if(readingId>=0)
	{
		readingTable[readingId].raw_length=(dl<=20?dl:20);
		memcpy(readingTable[readingId].raw,&buf[offset],readingTable[readingId].raw_length);
	}

	if(isstring)
	{
	}
	else
	{
		if(bcd)
		{
			reading=bcd2double(&buf[offset],dl,exp);
		}
		else
		{
			reading=int2double(&buf[offset],dl,exp);
		}

		if(readingId>=0)
		{
			mmHandleAlarms(readingId,reading);
			readingTable[readingId].value_valid=TRUE;
			readingTable[readingId].value=reading;
			mmHandleMaxMin(readingId,reading);
		}
	}

	offset+=dl;
	*offsetin=offset;
	return buf[offset];
}

int pmGetDataLength(unsigned char dib)
{
//	5 means variable
	int l;
	l=dib&0x07;
	if(l==0x07)
		l++;
	return l;
}

double bcd2double(unsigned char *buf,int len,int exp)
{
	int i;
	int ch1,ch2;
	double out=0;
	for(i=len-1;i>=0;i--)
	{
		ch1=buf[i]>>4;

		ch2=buf[i]&0x0f;
		out=out*100+(ch1*10+ch2);
	}
	for(i=0;i<exp;i++)
		out*=10;
	for(i=exp;i<0;i++)
		out/=10;
	return out;
}

double int2double(unsigned char *buf,int len,int exp)
{
	register int i;
	long long int temp;
	double out;
	if(buf[len-1]>=0x80)
		temp=-1;
	else
		temp=0;

	memcpy(&temp,buf,len);

	out=(double)temp;
	for(i=0;i<exp;i++)
		out*=10;
	for(i=exp;i<0;i++)
		out/=10;
	return out;
}

int pmRecvReply(int sockfd,unsigned char *buf,int len,int timeout)
{
	time_t inizio;
	unsigned char l;

	int numbytes,received;
	received=0;
	inizio=time(NULL);
	do
	{
		if ((numbytes=recv(sockfd, &buf[received], len-received, 0)) > 0)
			received+=numbytes;
		if(numbytes==0)
			usleep(1000);
	}
	while((received<3)&&(time(NULL)-inizio<timeout));	

	if(received<3)
		return 0;
	if((buf[1]!=buf[2])||(buf[1]<15))
		return 0;
	l=4+buf[1];

	inizio=time(NULL);
	while((received<l)&&(time(NULL)-inizio<timeout))
	{
		if ((numbytes=recv(sockfd, &buf[received], len-received, 0)) > 0)
			received+=numbytes;
		if(numbytes==0)
			usleep(1000);
	}
	if(received<l)
		return 0;
	return received;
}
