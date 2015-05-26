#include "systems.h"

void doSystem(int k)
{
	struct msgform msg;
	int out;

	if(systems[k].enabled)
		systems[k].status='1';	//before connect
	my_printf("trying to connect to system %s port %d\n",
		systems[k].address,systems[k].port);
	systems[k].sockfd=doConnect(k);
	my_printf("address %s, port %d, type %d connected\n",systems[k].address, systems[k].port, systems[k].type);
	resetDeviceDigitalOut(k);

	do
	{
		out=operate(systems[k].sockfd,systemNumToId(systems[k].device_num,NUMSYSTEMS),2);
		if(out==-1)
		{
			handleMessage(0,0,systems[k].device_num,0,"DEVICE DISCONNECTED",0,-1);

			systemDisconnected(k);
			close(systems[k].sockfd);
			systems[k].sockfd=-1;
			if(systems[k].enabled)
			{
//				my_printf("error in %s %d type %d\nsleeping 5 seconds\n",systems[k].address,systems[k].port,systems[k].type);
				sleep(5);
//				my_printf("trying to connect\n");
			}
			systems[k].sockfd=doConnect(k);
			my_printf("address %s port %d type %d connected\n",systems[k].address, systems[k].port, systems[k].type);
		}
		else
			systems[k].status='0';	// operating ok
		usleep(1000000);	//intervallo interrogazione scheda
	}
	while(1);
}

int operate(int sockfd,int deviceId, int timeout)
/*--------------------------------------------
 * scrive su socket, attende risposta, aggiorna tabelle
 * int sockfd 	- descrittore del socket
 * int deviceId - identificatore device [0-NUMSYSTEMS-1]
 * int timeout	- timeout di attesa (tipicamente 2 secondi)
 * -----------------------------------------*/
{
	struct msgform msg;
	int sendLen;
	int out=0;
	int numbytes;
	int received;
	int recvLen;
	int i;
	int j;
	int k;
	unsigned char *recvBuf;
	unsigned char *sendBuf;
	unsigned char in_eos;
	unsigned char out_sos;
	unsigned char out_eos;
	unsigned char *out_data;
	int out_ch_per_byte;
	int in_ch_per_byte=8;
	int out_ch_d;
	int analogId;
	int digitalId;
	int in_ch_an;
	int in_ch_d;

	struct digitalOutLine *digital_out_line;
	int id_digital_out;

	bool valueislow;
	bool valueishigh;
	bool iamlow;
	bool iamhigh;

	int analogRead;
	double analogEng;

	time_t inizio,curtime;
	int minutes;
	struct timeval tv;
	struct tm ts;

	if(systems[deviceId].enabled>2)	//try to reload
	{
		systems[deviceId].status='1';
		inizio=time(NULL);
		do
		{
			usleep(10000);
		}
		while((systems[deviceId].enabled>2)&&(time(NULL)-inizio<timeout));
		if(systems[deviceId].enabled>2)
			systems[deviceId].enabled-=3;
	}
	if(systems[deviceId].enabled!=1)
	{
		if(systems[deviceId].enabled>1)
			systems[deviceId].enabled=1;
		else
			systems[deviceId].status='d';
		return -1;
	}

	sendLen=systems[deviceId].out_num_bytes;
	out_sos=systems[deviceId].out_sos;
	in_eos=systems[deviceId].in_eos;
	in_ch_an=systems[deviceId].in_ch_an;
	in_ch_d=systems[deviceId].in_ch_d;
	sendBuf=(unsigned char *)malloc(sendLen);
	sendBuf[0]=out_sos;
	if(sendLen>1)
	{
		out_ch_d=systems[deviceId].out_ch_d;

		out_eos=systems[deviceId].out_eos;
		sendBuf[sendLen-1]=out_eos;

		out_data=(unsigned char *)malloc(out_ch_d);
		/*-------------------------------------
		* in out_data va messo l'array di 1 e 0 
		* rappresentante i valori di invio
		-------------------------------------*/
		inizio=time(NULL);
		gettimeofday(&tv, NULL); 
		curtime=tv.tv_sec;
		ts=*localtime(&curtime);
		minutes=ts.tm_hour*60+ts.tm_min;

		for(i=0;i<out_ch_d;i++)
		{
			digital_out_line=&digitalOutTable[deviceToid[2][deviceId][i]];
			id_digital_out=digital_out_line->id_digital_out;
			if((id_digital_out!=-1)
					&&(digital_out_line->value!=-1))
			{
				//verifico che non ci sia un poweron delay
				if(digital_out_line->po_delay>0)
				{
					if((digital_out_line->value != digital_out_line->req_value)
							&&(inizio-digital_out_line->start_time >=digital_out_line->po_delay))
						digital_out_line->value=digital_out_line->req_value;
				}
				//fine verifica;

				//verifico che l'uscita non comandi relay con on_time
				if(digital_out_line->on_time>0)
				{
					if((digital_out_line->value==digital_out_line->on_value)
							&&(inizio-digital_out_line->start_time >=(digital_out_line->po_delay + digital_out_line->on_time)))
					{
						digital_out_line->value=1-digital_out_line->on_value;
						digital_out_line->req_value=digital_out_line->value;
					}
				}
				out_data[i]=digital_out_line->value;
			}
			else
				out_data[i]=0;


			// digital_out ha msg_h o msg_l
			if((digital_out_line->value!=digital_out_line->value1)
				&&(digital_out_line->value1!=-1))
			{
				if((digital_out_line->value==0)&&(strlen(digital_out_line->msg_l)>0))
				{
					handleMessage(
							2,
							digital_out_line->id_digital_out,
							digital_out_line->device_num,
							digital_out_line->ch_num,
							digital_out_line->msg_l,
							0,
							(digital_out_line->printer>0?0:-1));
				}
				else
				{
					if((digital_out_line->value==1)&&(strlen(digital_out_line->msg_h)>0))
					{
						handleMessage(
							2,
							digital_out_line->id_digital_out,
							digital_out_line->device_num,
							digital_out_line->ch_num,
							digital_out_line->msg_h,
							0,
							(digital_out_line->printer>0?0:-1));
					}
				}
			}
			digital_out_line->value1=digital_out_line->value;
			// fine digital_out ha msg_h o msg_l
		}


		/*------------------------------------*/

		out_ch_per_byte=systems[deviceId].out_ch_per_byte;

		for(j=0;j<sendLen-2;j++)
		{
			sendBuf[j+1]=0;
			for(k=0;k<out_ch_per_byte;k++)
				if(j*out_ch_per_byte+k < out_ch_d)
					sendBuf[j+1]+=(out_data[j*out_ch_per_byte+k] << k);
		}

		free(out_data);

	}


	if (!send(sockfd,sendBuf,sendLen,0)) 
		out=-1;
	else
	{
		recvLen=systems[deviceId].in_num_bytes;
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
			if(recvBuf[recvLen-1]==in_eos)
			{
				for(i=0;i<in_ch_an;i++)
				{
					analogId=deviceToid[0][deviceId][i];

					if((2*i+1<(recvLen-1))
						&&(analogTable[analogId].id_analog!=-1)
							&&(analogTable[analogId].enabled==1))
					{
						analogRead=0;


						for(j=0;j<2;j++)
							analogRead+=(recvBuf[2*i+j] << (8*(1-j)));


						if(analogRead<=analogTable[analogId].scale_full
							&& analogRead>=analogTable[analogId].scale_zero)
						{
							if(analogTable[analogId].no_linear)
								analogEng=getCurveValue(analogTable[analogId].curve,analogRead);
							else
								analogEng=analogTable[analogId].range_zero + 
									analogTable[analogId].offset + 
										analogTable[analogId].range_pend * ((double)(analogRead-analogTable[analogId].scale_zero));

//registro sforamenti
							valueishigh=(analogEng>analogTable[analogId].al_high);
							iamhigh=((analogTable[analogId].al_high_active)
									&& (((valueishigh)
											&& ((analogTable[analogId].time_delay_high==0)
												|| ((analogTable[analogId].time_delay_high_cur > 0)
												&& (time(NULL)-analogTable[analogId].time_delay_high_cur >= analogTable[analogId].time_delay_high))))
										||(										
										(!valueishigh) && (analogTable[analogId].currently_high)
										&& (analogTable[analogId].time_delay_high_off>0)
											&& ((analogTable[analogId].time_delay_high_off_cur == 0)
												|| (time(NULL)-analogTable[analogId].time_delay_high_off_cur < analogTable[analogId].time_delay_high_off)))
										));
										
							valueislow=(analogEng<analogTable[analogId].al_low);
							iamlow=((analogTable[analogId].al_low_active)
									&& (((valueislow)
											&& ((analogTable[analogId].time_delay_low==0)
												|| ((analogTable[analogId].time_delay_low_cur > 0)
												&& (time(NULL)-analogTable[analogId].time_delay_low_cur >= analogTable[analogId].time_delay_low))))
										||(										
										(!valueislow) && (analogTable[analogId].currently_low)
										&& (analogTable[analogId].time_delay_low_off>0)
											&& ((analogTable[analogId].time_delay_low_off_cur == 0)
												|| (time(NULL)-analogTable[analogId].time_delay_low_off_cur < analogTable[analogId].time_delay_low_off)))
										));

							if(iamhigh)
							{	//valore considerato alto
								if(!analogTable[analogId].currently_high)
								{
									analogTable[analogId].currently_high=1;
									analogTable[analogId].currently_low=0;
									//registro sforamento alto
									handleMessage(0,analogTable[analogId].id_analog,
										analogTable[analogId].device_num,analogTable[analogId].ch_num,
										analogTable[analogId].msg_h,
										analogTable[analogId].sms,0);
								}
								if(valueishigh)
								{
									//valore attuale alto
									analogTable[analogId].time_delay_high_off_cur=0;
									analogTable[analogId].time_delay_low_off_cur=0;
									analogTable[analogId].time_delay_low_cur=0;
								}
								else
								{
									if(analogTable[analogId].time_delay_high_off_cur==0)
										analogTable[analogId].time_delay_high_off_cur=time(NULL);

									analogTable[analogId].time_delay_low_cur=0;
									analogTable[analogId].time_delay_low_off_cur=0;

									if(valueislow)
									{
										//valore attuale basso
										if(analogTable[analogId].time_delay_low_cur==0)
											analogTable[analogId].time_delay_low_cur=time(NULL);
									}
								}
							}
							else
							{
								if(iamlow)
								{	//valore considerato basso
									if(!analogTable[analogId].currently_low)
									{
										analogTable[analogId].currently_low=1;
										analogTable[analogId].currently_high=0;
										//registro sforamento basso
										handleMessage(0,analogTable[analogId].id_analog,
											analogTable[analogId].device_num,
											analogTable[analogId].ch_num,
											analogTable[analogId].msg_l
											,analogTable[analogId].sms,0);
									}
									if(valueislow)
									{
										//valore attuale basso
										analogTable[analogId].time_delay_high_off_cur=0;
										analogTable[analogId].time_delay_low_off_cur=0;
										analogTable[analogId].time_delay_high_cur=0;
									}
									else
									{
										if(analogTable[analogId].time_delay_low_off_cur==0)
											analogTable[analogId].time_delay_low_off_cur=time(NULL);

										analogTable[analogId].time_delay_high_cur=0;
										analogTable[analogId].time_delay_high_off_cur=0;

										if(valueishigh)
										{
											//valore attuale alto
											if(analogTable[analogId].time_delay_high_cur==0)
												analogTable[analogId].time_delay_high_cur=time(NULL);
										}
									}
								}
								else
								{
									//valore considerato nella norma
									if(analogTable[analogId].currently_low || analogTable[analogId].currently_high)
									{
										analogTable[analogId].currently_low=0;
										analogTable[analogId].currently_high=0;
										
										//registro ritorno alla norma
										if((!valueishigh)&&(!valueislow))
										{
											handleMessage(0,analogTable[analogId].id_analog,
												analogTable[analogId].device_num,
												analogTable[analogId].ch_num,"value in range",
												0,0);
										}
									}
									
									if(valueishigh)
									{
										//valore attuale alto
										analogTable[analogId].time_delay_low_cur=0;
										if(analogTable[analogId].time_delay_high_cur==0)
											analogTable[analogId].time_delay_high_cur=time(NULL);
									}
									else
									{
										if(valueislow)
										{
											//valore attuale basso
											analogTable[analogId].time_delay_high_cur=0;
											if(analogTable[analogId].time_delay_low_cur==0)
												analogTable[analogId].time_delay_low_cur=time(NULL);
										}
										else
										{
											//valore attuale nella norma
											analogTable[analogId].time_delay_high_cur=0;
											analogTable[analogId].time_delay_low_cur=0;
										}
									}
								}
							}
//fine registro sforamenti
						}
						else
						{
							my_printf("I: %d - analogRead: %d - scale_full: %d - scale_zero: %d\n",i,analogRead,analogTable[analogId].scale_full,analogTable[analogId].scale_zero);

							analogRead=-1;
							analogEng=-1;
						}

						analogTable[analogId].value4=analogTable[analogId].value3;
						analogTable[analogId].value3=analogTable[analogId].value2;
						analogTable[analogId].value2=analogTable[analogId].value1;
						analogTable[analogId].value1=analogTable[analogId].value;
						analogTable[analogId].value=analogRead;
						analogTable[analogId].value_eng4=analogTable[analogId].value_eng3;
						analogTable[analogId].value_eng3=analogTable[analogId].value_eng2;
						analogTable[analogId].value_eng2=analogTable[analogId].value_eng1;
						analogTable[analogId].value_eng1=analogTable[analogId].value_eng;
						analogTable[analogId].value_eng=analogEng;
					}
					else
					{
//						my_printf("2*i+1=%d - recvLen=%d - id_analog=%d - enabled=%d\n",(2*i+1),recvLen,analogTable[i].id_analog,analogTable[i].enabled);
					}
				}

				j=in_ch_an*2;
				k=0;
				for(i=0;i< in_ch_d;i++)
				{
					digitalId=deviceToid[1][deviceId][i];
					
					if((j<recvLen)
						&&(digitalTable[digitalId].id_digital!=-1)
							&&(digitalTable[digitalId].enabled==1))
					{
						digitalTable[digitalId].value4=digitalTable[digitalId].value3;
						digitalTable[digitalId].value3=digitalTable[digitalId].value2;
						digitalTable[digitalId].value2=digitalTable[digitalId].value1;
						digitalTable[digitalId].value1=digitalTable[digitalId].value;
						digitalTable[digitalId].value=((recvBuf[j] & (1<<k))>>k);
						
						//gestione sforamenti
						
						if(digitalTable[digitalId].value==digitalTable[digitalId].alarm_value)
						{
							if((digitalTable[digitalId].time_delay_on>0)
									&&(digitalTable[digitalId].time_delay_on_cur==0))
								digitalTable[digitalId].time_delay_on_cur=time(NULL);

							if((digitalTable[digitalId].time_delay_on==0)||
								((digitalTable[digitalId].time_delay_on_cur>0)
									&&(time(NULL)-digitalTable[digitalId].time_delay_on_cur>=digitalTable[digitalId].time_delay_on)))
							{
								digitalTable[digitalId].time_delay_off_cur=0;
								//sforo
								if(!digitalTable[digitalId].currently_on)
								{
									digitalTable[digitalId].currently_on=1;
									//notifico sforamento
									handleMessage(1,digitalTable[digitalId].id_digital,
										digitalTable[digitalId].device_num,
										digitalTable[digitalId].ch_num,
										digitalTable[digitalId].msg,
										digitalTable[digitalId].sms,0);
								}
							}
						}
						else
						{
							if((digitalTable[digitalId].time_delay_off>0)
									&&(digitalTable[digitalId].time_delay_off_cur==0))
								digitalTable[digitalId].time_delay_off_cur=time(NULL);

							if((digitalTable[digitalId].time_delay_off==0)||
								((digitalTable[digitalId].time_delay_off_cur>0)
									&&(time(NULL)-digitalTable[digitalId].time_delay_off_cur>=digitalTable[digitalId].time_delay_off)))
							{
								digitalTable[digitalId].time_delay_on_cur=0;
								//sono in norma
								if(digitalTable[digitalId].currently_on)
								{
									digitalTable[digitalId].currently_on=0;
									//notifico ritorno norma
									handleMessage(1,digitalTable[digitalId].id_digital,
										digitalTable[digitalId].device_num,
										digitalTable[digitalId].ch_num,"value in range",
										0,0);
								}
							}
						}
						//fine gestione sforamenti
					}
					k++;
					if(k==in_ch_per_byte)
					{
						k=0;
						j++;
					}
				}
			}
		}
		else
		{
			out=-1;
		}
		free(recvBuf);
	}
	free(sendBuf);
	return(out);
}

int doConnect(int systemId)
/*--------------------------------------------
 * gestisce la connessione/disconnessione dei device
 * riprova ogni 2 secondi
 * int systemId - identificatore device [0-NUMSYSTEMS-1]
 * -----------------------------------------*/
{
	int out;
	int attempt=0;
	struct msgform msg;
	
	do
	{
		out=0;
		if(systems[systemId].enabled==0)
			out=-1;
		else
			out=socketConnect(systems[systemId].address,systems[systemId].port);

		if(out!=-1)
			attempt=0;
		else
		{
			if(systems[systemId].enabled)
			{
				attempt++;
				my_printf("%s %d: attempt %d failed\n",
					systems[systemId].address,systems[systemId].port,attempt);
			}
			if(attempt>=MAXATTEMPTS)
			{
				systems[systemId].status='3';	//failed after maxattempts
				sendResetSignal(systemId);
				sleep(3);
				attempt=0;
			}
			systemDisconnected(systemId);
			sleep(2);
		}
	}
	while(out<0);
	
	setOutputDefault(systemId);
	handleMessage(0,0,systems[systemId].device_num,0,"DEVICE CONNECTED",0,-1);

	return out;
}

int systemNumToId(int systemNum, int totale)
/*--------------------------------------------
 * trova l'indice in tabella in memoria systems 
 * relativo al valore device_num in db
 * int systemNum - valore in db
 * totale		 - conteggio totale devices
 * -----------------------------------------*/
{
	int i=0;
	while(i<totale && systems[i].device_num!=systemNum)
		i++;
	
	return(i<totale?i:-1);
}

void systemDisconnected(int systemId)
/*--------------------------------------------
 * scrive su analog e digital con valori -1
 * chiamata quando non "sento" la scheda
 * int systemId - identificativo device [0-NUMSYSTEMS-1]
 * -----------------------------------------*/
{
	int i,j,id;

	if((systems[systemId].enabled)
			&&(systems[systemId].status!='3'))
		systems[systemId].status='2';
	for(j=0;j<systems[systemId].in_ch_an;j++)
	{
		id=deviceToid[0][systemId][j];
		resetAnalogValues(id);
	}
	for(j=0;j<systems[systemId].in_ch_d;j++)
	{
		id=deviceToid[1][systemId][j];
		resetDigitalValues(id);
	}
}

void sendResetSignal(int systemId)
{
	int sock;
	struct sockaddr_in sa;
	int bytes_sent, buffer_length;
	char buffer[2];
	struct hostent *he;
	int totbytes=0;

	if ((he=gethostbyname(systems[systemId].address)) == NULL)
		return;
  
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock==-1)//if socket failed to initialize, exit
	{
		my_perror("Error creating socket for reset");
		return;
	}
 
	sa.sin_family = AF_INET;
	sa.sin_port = htons(systems[systemId].port);
	sa.sin_addr = *((struct in_addr *)he->h_addr);
 
	sprintf(buffer, "L");
	buffer_length = strlen(buffer);

	bytes_sent = sendto(sock, buffer, buffer_length, 0,(struct sockaddr*) &sa, sizeof(struct sockaddr_in) );
	if(bytes_sent < 0)
	{
		my_perror("Error sending first reset packet");
		close(sock);//close the socket
		return;
	}
	totbytes+=bytes_sent;
	usleep(500000); 	//attendo mezzo secondo tra i bytes

	sprintf(buffer, "E");
	buffer_length = strlen(buffer);

	bytes_sent = sendto(sock, buffer, buffer_length, 0,(struct sockaddr*) &sa, sizeof(struct sockaddr_in) );
	if(bytes_sent < 0)
	{
		my_perror("Error sending  second reset packet");
		close(sock);//close the socket
		return;
	}
	totbytes+=bytes_sent;
	
	my_printf("Sent %d UDP bytes to %s\n", totbytes, systems[systemId].address);
	close(sock);//close the socket
	return;
}


void updateSystem(int device_num)
{
	int state;
	int systemId;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i;

	systemId=systemNumToId(device_num,NUMSYSTEMS);

	sprintf(query,"SELECT system.tcp_ip,system.port,system.enabled"
								" FROM system "
								" WHERE system.device_num=%d",device_num);

	state = mysql_query(connection,query);

	if( state != 0 )
		my_printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
	
	if (( row = mysql_fetch_row(result)) != NULL )
	{
		if(row[0])
			strcpy(systems[systemId].address,row[0]);
		else
			strcpy(systems[systemId].address,"");
		systems[systemId].port=atoi(row[1]);
		systems[systemId].enabled=atoi(row[2]);
		if(systems[systemId].enabled)
			systems[systemId].enabled=2;	//force restart
	}
	mysql_free_result(result);
}

int loadSystemTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	
	state = mysql_query(connection, "SELECT system.device_type,system.device_num,system.tcp_ip,"
									"system.port,device_type.in_num_bytes,device_type.in_ch_an,"
									"device_type.in_ch_d,device_type.in_eos,device_type.out_num_bytes,"
									"device_type.out_ch_d,device_type.out_ch_per_byte,device_type.out_sos,"
									"device_type.out_eos,system.enabled"
								" FROM system LEFT JOIN device_type ON system.device_type=device_type.type"
								" WHERE system.removed=0"
								" ORDER BY system.device_num");

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

	if(!onlyupdate)
	{
		NUMSYSTEMS=mysql_num_rows(result);
		if(NUMSYSTEMS>0)
		{
			my_printf("get_shared_memory_segment: systems\n");
			systems=(struct system *)get_shared_memory_segment(NUMSYSTEMS * sizeof(struct system), &SHMSYSTEMSID, "/dev/zero");
			if(!systems)
				die("systems - get_shared_memory_segment\n");
		}
		else
			systems=0;
	}
	
	i=0;

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		systems[i].type=atoi(row[0]);
		systems[i].device_num=atoi(row[1]);

		if(row[2])
			strcpy(systems[i].address,row[2]);
		else
			strcpy(systems[i].address,"");

		systems[i].port=atoi(row[3]);
		systems[i].in_num_bytes=atoi(row[4]);
		systems[i].in_ch_an=atoi(row[5]);
		systems[i].in_ch_d=atoi(row[6]);
		systems[i].in_eos=atoi(row[7]);
		systems[i].out_num_bytes=atoi(row[8]);
		systems[i].out_ch_d=atoi(row[9]);
		systems[i].out_ch_per_byte=atoi(row[10]);
		systems[i].out_sos=atoi(row[11]);
		systems[i].out_eos=atoi(row[12]);
		systems[i].enabled=atoi(row[13]);

		if(!onlyupdate)
		{
			if(systems[i].enabled)
				systems[i].status='1';
			else
				systems[i].status='d';

			ANALOGCHANNELS+=systems[i].in_ch_an;
			DIGITALCHANNELS+=systems[i].in_ch_d;
			DIGITALOUTCHANNELS+=systems[i].out_ch_d;
		}
		i++;
	}
	mysql_free_result(result);
	return(0);
}
