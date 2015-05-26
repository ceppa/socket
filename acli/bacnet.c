#include "bacnet.h"

void doBacnet(int l)
{
	int out;
	struct msgform msg;

	do
	{
		if(bacnetDevices[l].enabled)
		{
			checkBacnetReload(l);

			if((bacnetDevices[l].status!='0')||(bacnetDevices[l].sockfd==-1))
			{
				bacnetDevices[l].status='1'; //connecting
				my_printf("trying to connect to bacnet %s %d\n",
					bacnetDevices[l].address,bacnetDevices[l].port);

				bacnetDevices[l].sockfd=doConnectBacnet(l);
			}
			if(bacnetDevices[l].sockfd!=-1)
			{
				out=operateBacnet(l);
		
				if(out==-1)
				{
					bacnetDeviceDisconnected(l);
					close(bacnetDevices[l].sockfd);
					bacnetDevices[l].sockfd=-1;

					my_printf("error in bacnet %s %d\nsleeping 5 seconds\n",
						bacnetDevices[l].address,bacnetDevices[l].port);
					sleep(5);
				}
				else
					bacnetDevices[l].status='0';	//connected
			}
			else
			{
				sleep(5);
			}
		}
		else
			bacnetDevices[l].status='d';
		usleep(50000);	//intervallo interrogazione scheda
	}
	while(1);
}

int doConnectBacnet(int id)
/*--------------------------------------------
 * apre il socket
 * riprova ogni 10 secondi
 * int id - identificatore device [0-NUMBACNETDEVICES-1]
 * -----------------------------------------*/
{
	int out;

	out=0;
	if(bacnetDevices[id].enabled==0)
		out=-1;
	else
	{
		out=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(out==-1)
			sleep(10);
	}
	bacnetDevices[id].sockfd=out;
	return out;
}

int operateBacnet(int id_device)
/*--------------------------------------------
 * invia / riceve pacchetti
 * aggiorna tabella dati
 * int id_device - identificatore device [0-NUMBACNETDEVICES-1]
 * assumo che il device sia enabled e abbia sockfd>0
 * -----------------------------------------*/
{
	int id_device_line;
	int id_bacnet_line;
	int i;
	int sock;
	int id=0;
	int out=-1;
	unsigned long object_instance,object_type,identifier,value;
	int received=0;
	int sent=0;

	for(i=0;i<NUMBACNETLINES;i++)
	{
		id_device_line=bacnetDeviceToId(bacnetTable[i].id_bacnet_device);
		if(id_device_line==id_device)
		{
			sock=bacnetDevices[id_device].sockfd;
			object_type=bacnetTable[i].object_type;
			object_instance=bacnetTable[i].object_instance;
			identifier=(object_type<<22)|object_instance;
			if((sock!=-1)&&(sendBacnetRequest(sock,
					identifier,bacnetDevices[id_device].address,id)))
			{
				sent++;
				if(waitForResponse(sock,BACNET_TIMEOUT))
				{
					if(receiveBacnetResponse(sock,&identifier,&value)!=-1)
					{
						received++;
						id_bacnet_line=bacnetIdentifierToId(
							bacnetDevices[id_device].id,
							identifier);
// gestione sforamenti e allarmi 
						if(bacnetTable[id_bacnet_line].object_type==3)
						{
							if((bacnetTable[id_bacnet_line].value==1)
								&&(value==0)&&(bacnetTable[id_bacnet_line].hi_low_msg))
								handleMessage(
									BACNET,
									bacnetTable[id_bacnet_line].id,
									bacnetTable[id_bacnet_line].id_bacnet_device,
									0,
									bacnetTable[id_bacnet_line].hi_low_msg,
									0,
									-1);
							if((bacnetTable[id_bacnet_line].value==0)
								&&(value==1)&&(bacnetTable[id_bacnet_line].low_hi_msg))
								handleMessage(
									BACNET,
									bacnetTable[id_bacnet_line].id,
									bacnetTable[id_bacnet_line].id_bacnet_device,
									0,
									bacnetTable[id_bacnet_line].low_hi_msg,
									0,
									-1);
							if(bacnetTable[id_bacnet_line].is_alarm)
							{
								if((bacnetTable[id_bacnet_line].value==0)&&(value==1))
									handleMessage(
										BACNET,
										bacnetTable[id_bacnet_line].id,
										bacnetTable[id_bacnet_line].id_bacnet_device,
										0,
										bacnetTable[id_bacnet_line].alarm_msg,
										0,
										0);
								if((bacnetTable[id_bacnet_line].value==1)&&(value==0))
									handleMessage(
										BACNET,
										bacnetTable[id_bacnet_line].id,
										bacnetTable[id_bacnet_line].id_bacnet_device,
										0,
										"value in range",
										0,
										-2);
							}
						}
						bacnetTable[id_bacnet_line].value_valid=TRUE;
						bacnetTable[id_bacnet_line].value=value;
// fine gestione sforamenti e allarmi 
					}
					else
					{
						id_bacnet_line=bacnetIdentifierToId(
							bacnetDevices[id_device].id,
							identifier);

						bacnetTable[id_bacnet_line].value_valid=FALSE;
						bacnetTable[id_bacnet_line].value=0xffffffff;
					}
				}
			}
		}
	}
	if(received==0)
	{

		bacnetDevices[id_device].status='2';
		bacnetDevices[id_device].failures++;

		if(bacnetDevices[id_device].failures>BACNET_MAXFAILURES)
		{
			if(bacnetDevices[id_device].ok==TRUE)
				handleMessage(BACNET,0,
					bacnetDevices[id_device].id,0,
					"DEVICE DISCONNECTED",0,0);
			bacnetDevices[id_device].ok=FALSE;
			bacnetDevices[id_device].failures=0;
			bacnetDevices[id_device].status='3';
			bacnetDeviceDisconnected(id_device);
			my_printf("BACNET %s error operating, sleeping 5 seconds\n",
				bacnetDevices[id_device].description);
			sleep(5);
		}
	}
	else
	{
		bacnetDevices[id_device].ok=TRUE;
		bacnetDevices[id_device].failures=0;
		//TODO: if received<sent increase timeout
/*		if(received<sent)
			printf("bacnet: sent %d received %d\n",sent,received);*/
		out=0;
		if(bacnetDevices[id_device].status!='0')
		{
			bacnetDevices[id_device].status='0';
			my_printf("address %s, port %d, bacnet connected\n",
				bacnetDevices[id_device].address, bacnetDevices[id_device].port);

			handleMessage(BACNET,0,
				bacnetDevices[id_device].id,0,"OPERATING OK",0,-2);
		}
	}
	return out;
}

void bacnetDeviceDisconnected(int id)
/*--------------------------------------------
 * scrive su bacnetTable 0xffff
 * chiamata quando non "sento" la scheda
 * int id - identificativo device [0-NUMBACNETDEVICES-1]
 * -----------------------------------------*/
{
	int j;

	if((bacnetDevices[id].enabled)
			&&(bacnetDevices[id].status!='3'))
		bacnetDevices[id].status='2';
	for(j=0;j<NUMBACNETLINES;j++)
	{
		if(bacnetTable[j].id_bacnet_device==bacnetDevices[id].id)
			resetBacnetValue(j);
	}
}


int id_bacnet_deviceToId(int id_bacnet_device)
{
	int i;
	
	for(i=0;i<NUMBACNETDEVICES;i++)
		if(bacnetDevices[i].id==id_bacnet_device)
			break;
	return((i<NUMBACNETDEVICES)?i:-1);
}

int loadBacnet(bool reload)
{
	MYSQL_RES *result;
	MYSQL_RES *resultLines;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	char query[512];
	char querylines[512];
	
	strcpy(query,"SELECT bacnet_devices.id,"
					"bacnet_devices.address,"
					"bacnet_devices.port,"
					"bacnet_devices.description,"
					"bacnet_devices.enabled"
					" FROM bacnet_devices"
					" ORDER BY bacnet_devices.id");
	state = mysql_query(connection, query);

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);



	strcpy(querylines,"SELECT bacnet_inputs.id,"
					"bacnet_inputs.id_bacnet_device,"
					"bacnet_inputs.object_type,"
					"bacnet_inputs.object_instance,"
					"bacnet_inputs.description,"
					"bacnet_inputs.hi_low_msg,"
					"bacnet_inputs.low_hi_msg,"
					"bacnet_inputs.is_alarm,"
					"bacnet_inputs.alarm_msg "
					" FROM bacnet_inputs"
					" ORDER BY bacnet_inputs.id");

	state = mysql_query(connection, querylines);

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(connection));
		return(1);
	}
	resultLines = mysql_store_result(connection);


	if(reload)
	{
		if(shmdt(bacnetDevices))
			my_perror("shmdt");
		shmctl(SHMBACNETDEVICESID, IPC_RMID, &shmid_struct);

		if(shmdt(bacnetTable))
			my_perror("shmdt");
		shmctl(SHMBACNETTABLEID, IPC_RMID, &shmid_struct);		
	}
	NUMBACNETDEVICES=mysql_num_rows(result);
	NUMBACNETLINES=mysql_num_rows(resultLines);


	if(NUMBACNETDEVICES&&NUMBACNETLINES)
	{
		my_printf("get_shared_memory_segment: bacnetDevices\n");
		bacnetDevices=(struct bacnetDevice *)get_shared_memory_segment(NUMBACNETDEVICES * sizeof(struct bacnetDevice), &SHMBACNETDEVICESID, "/dev/zero");
		if(!bacnetDevices)
			die("bacnetDevices - get_shared_memory_segment\n");
		my_printf("get_shared_memory_segment: bacnetTable\n");
		bacnetTable=(struct bacnetLine *)get_shared_memory_segment(NUMBACNETLINES * sizeof(struct bacnetLine), &SHMBACNETTABLEID, "/dev/zero");
		if(!bacnetTable)
			die("bacnetTable - get_shared_memory_segment\n");
	}
	else
	{
		bacnetTable=0;
		bacnetDevices=0;
	}

	i=0;

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		bacnetDevices[i].id=atoi(row[0]);
		if(row[1])
			strcpy(bacnetDevices[i].address,row[1]);
		else
			strcpy(bacnetDevices[i].address,"");
		bacnetDevices[i].port=atoi(row[2]);
		if(row[3])
			strcpy(bacnetDevices[i].description,row[3]);
		else
			strcpy(bacnetDevices[i].description,"");
		
		bacnetDevices[i].enabled=atoi(row[4]);

		bacnetDevices[i].status='0';
		if(!reload)
		{
			if(bacnetDevices[i].enabled)
				bacnetDevices[i].status='1';
			else
				bacnetDevices[i].status='d';
		}
		bacnetDevices[i].failures=0;
		bacnetDevices[i].sockfd=0;
		bacnetDevices[i].ok=TRUE;
		i++;
	}
	mysql_free_result(result);

	i=0;
	while( ( row = mysql_fetch_row(resultLines)) != NULL )
	{
		bacnetTable[i].id=atoi(row[0]);
		bacnetTable[i].id_bacnet_device=atoi(row[1]);
		bacnetTable[i].object_type=atoi(row[2]);
		bacnetTable[i].object_instance=atoi(row[3]);
		if(row[4])
			strcpy(bacnetTable[i].description,row[4]);
		else
			strcpy(bacnetTable[i].description,"");

		if(bacnetTable[i].object_type==3)
		{
			if(row[5])
				strcpy(bacnetTable[i].hi_low_msg,row[5]);
			else
				strcpy(bacnetTable[i].hi_low_msg,"");
			if(row[6])
				strcpy(bacnetTable[i].low_hi_msg,row[6]);
			else
				strcpy(bacnetTable[i].low_hi_msg,"");
			bacnetTable[i].is_alarm=atoi(row[7]);
			if(row[8])
				strcpy(bacnetTable[i].alarm_msg,row[8]);
			else
				strcpy(bacnetTable[i].alarm_msg,"");
		}
		else
		{
			strcpy(bacnetTable[i].hi_low_msg,"");
			strcpy(bacnetTable[i].low_hi_msg,"");
			bacnetTable[i].is_alarm=FALSE;
			strcpy(bacnetTable[i].alarm_msg,"");
		}
		bacnetTable[i].value=0xffffffff;
		bacnetTable[i].value_valid=FALSE;
		i++;
	}
	mysql_free_result(resultLines);
	return(0);
}

void resetBacnetValue(int i)
{
	bacnetTable[i].value=0xffffffff;
	bacnetTable[i].value_valid=FALSE;
}



int id_bacnet_lineToId(int id_bacnet_line)
{
	int i;
	
	for(i=0;i<NUMBACNETLINES;i++)
		if(bacnetTable[i].id==id_bacnet_line)
			break;
	return((i<NUMBACNETLINES)?i:-1);
}


/*************************************/
int receiveBacnetResponse( int socket,unsigned long *identifier,unsigned long *value )
/*************************************/
{
	int id=-1,i;
	unsigned char frame[64] ;
	struct sockaddr_in  addr ;
	int len  = sizeof( addr ) ;
	int size = recvfrom( socket, (char *)frame, sizeof(frame), 0, (struct sockaddr *)&addr, &len ) ;
	int resultLen=0;
	unsigned long tempvalue=0xffffffff;
	unsigned long tempidentifier=0;

	// Check the header
	if((size>8)
		&&(frame[0]==BACNET_IP)
		&&(frame[1]==UNICAST)
		&&(frame[4]==BACNET_VERSION))
	{
		if((((frame[2] << 8)|frame[3])==size) 
			&&(frame[8]==READ_PROPERTY)
			&&(frame[6] != BACNET_ERROR ))
		{
			 if((size > 18)
					&&(frame[6]==COMPLEX_ACK)
					&&(frame[14]==PROPERTY_TAG)
					&&(frame[15]==PRESENT_VALUE)
					&&((frame[17]==BINARY_INPUT_ANS)||(frame[17]==ANALOG_INPUT_ANS))
					)
			{
				id=frame[7];
				for(i=0;i<4;i++)
					tempidentifier=((tempidentifier)<<8)+frame[10+i];
				resultLen=(frame[17]==BINARY_INPUT_ANS?1:4);
				tempvalue=0;
				for(i=0;i<resultLen;i++)
					tempvalue=(tempvalue<<8)+frame[18+i];
				*identifier=tempidentifier;
				*value=tempvalue;
			}
		}
	}
	return id;
}

float formatBacnetValue(int object_type,unsigned long value)
{
	float out;
	if(object_type==0)
		memcpy(&out,&value,4);
	else
		out=(float)value;
	return out;
}

/*********************************/
 bool sendBacnetRequest( int socket,unsigned long object,char *ip,int id )
/*********************************/

{
   unsigned char frame[REQUEST_LENGTH] ;

   frame[ 0] = BACNET_IP ;
   frame[ 1] = UNICAST  ;
   frame[ 2] = REQUEST_LENGTH >> 8 ;
   frame[ 3] = REQUEST_LENGTH ;
   frame[ 4] = BACNET_VERSION ;
   frame[ 5] = 0x04 ; // No DNET, no SNET, reply expected
   frame[ 6] = 0x00 ; // Confirmed request
   frame[ 7] = 0x00 ; // Response size 50 bytes
   frame[ 8] = id   ; // Invoke id
   frame[ 9] = READ_PROPERTY ;
   frame[10] = 0x0C ; // Object id tag

   frame[11] = object >> 24 ;
   frame[12] = object >> 16 ;
   frame[13] = object >>  8 ;
   frame[14] = object;

   frame[15] = PROPERTY_TAG ;
   frame[16] = PRESENT_VALUE ;

   struct sockaddr_in addr ;
   addr.sin_family = AF_INET ;
   addr.sin_port = htons( 0xBAC0 ) ;
   addr.sin_addr.s_addr = inet_addr(ip) ;

   return (sendto(socket,(const char *)frame,REQUEST_LENGTH,0,(struct sockaddr *)&addr,sizeof(addr)) == REQUEST_LENGTH) ;
}

/**************************************************/
 bool waitForResponse( int socket, int timeout )
/**************************************************/

{
   struct timeval tv ;
   tv.tv_sec  = 0 ;
   tv.tv_usec = timeout ;

   fd_set fdset ;
   FD_ZERO( &fdset ) ;
   FD_SET( socket, &fdset ) ;

   return (select(socket+1,&fdset,NULL,NULL,&tv) > 0 ) ;
}


int bacnetIdentifierToId(int id_bacnet_device,unsigned long identifier)
{
	unsigned long object_instance;
	unsigned long object_type;
	int i;

	object_type=identifier>>22;
	object_instance=identifier & 0x3FFFFF;
	for(i=0;i<NUMBACNETLINES;i++)
		if((bacnetTable[i].id_bacnet_device==id_bacnet_device)
				&&(bacnetTable[i].object_type==object_type)
				&&(bacnetTable[i].object_instance==object_instance))
			return i;
	return -1;
}

int bacnetDeviceToId(int id_bacnet_device)
{
	int i;
	for(i=0;i<NUMBACNETDEVICES;i++)
		if(bacnetDevices[i].id==id_bacnet_device)
			return i;
	return -1;
}

void updateBacnet(int id_bacnet_device)
{
	int state;
	int id;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i;

	id=id_bacnet_deviceToId(id_bacnet_device);
	bacnetDevices[id].enabled=0;	//preventing bacnet to work while in update

	sprintf(query,"SELECT bacnet_devices.address,bacnet_devices.port,bacnet_devices.enabled"
								" FROM bacnet_devices "
								" WHERE bacnet_devices.id=%d",id_bacnet_device);

	state = mysql_query(connection,query);

	if( state != 0 )
		my_printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
	
	if (( row = mysql_fetch_row(result)) != NULL )
	{
		if(row[0])
			strcpy(bacnetDevices[id].address,row[0]);
		else
			strcpy(bacnetDevices[id].address,"");
		bacnetDevices[id].port=atoi(row[1]);
		bacnetDevices[id].enabled=atoi(row[2]);
		if(bacnetDevices[id].enabled)
			bacnetDevices[id].enabled=2;	//force restart
	}
	mysql_free_result(result);
}

void checkBacnetReload(int id)
{				
	if(bacnetDevices[id].enabled>1)
	{
		my_printf("requested bacnet reload, address %s port %d\n",
						bacnetDevices[id].address,bacnetDevices[id].port);
		close(bacnetDevices[id].sockfd);
		bacnetDevices[id].sockfd=-1;
		bacnetDevices[id].enabled=1;
		bacnetDevices[id].status='1';
	}
}
