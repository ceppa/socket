#include "sms.h"

void doMessageHandler()
/*--------------------------------------------
 * gestisce la connessione/disconnessione del device SMS
 * riprova ogni 2 secondi
 * legge la coda degli SMS e invia
 * -----------------------------------------*/
{
	int out=0;
	int attempt=0;
	int vai;
	int i;
	static struct msgform msg;
	char *c;
	char tempLogFileName[80];
	struct stat statbuf;


	logFileFd=-1;
	strcpy(logPath,"/var/www/socket/log/");
	getLogFileName(logFileName,logPath);
	logFileFd=open(logFileName,O_WRONLY | O_CREAT | O_APPEND, 0644);
	fstat(logFileFd,&statbuf);
	if(statbuf.st_size==0)
		scriviEffemeridiSuFile(logFileFd);

	msg.mtype=0;

	while(1)
	{
		//leggo coda dei messaggi e scrivo su log
		getLogFileName(tempLogFileName,logPath);
		if(strcmp(tempLogFileName,logFileName)!=0)
		{
			close(logFileFd);
			strcpy(logFileName,tempLogFileName);
			logFileFd=open(logFileName,O_WRONLY | O_CREAT | O_APPEND, 0644);
			scriviEffemeridiSuFile(logFileFd);
		}

		vai=1;
		do
		{
			if(msgrcv(msg_alarm_id,&msg,MSGLENGTH,0,IPC_NOWAIT)!=-1)
			{
				write(logFileFd,msg.mtext,strlen(msg.mtext));
				if(msg.mtype>1)	//is sms
				{
					if(NUMSMSDEVICES>0)
					{
						if((sms_devices[0].status!='0')||(sms_devices[0].sockfd==-1))
							sms_devices[0].sockfd=doConnectSMS(0);
						if(sms_devices[0].sockfd>0)
						{
							for(i=0;i<NUMSMSNUMBERS;i++)
							{
								if((c=strrchr(msg.mtext,'\t'))==0)
									c=msg.mtext;
								else 
									c++;
								if(sendsms(sms_devices[0].sockfd,
										sms_numbers[i].number,c)<0)
									sms_disconnect(0);
								else
								{
									sms_devices[0].status='0';
									sms_devices[0].failures=0;
								}
							}
						}
						else
						{
							sms_devices[0].failures++;
							if(sms_devices[0].failures>MAXATTEMPTS)
								sms_devices[0].status='3';
							my_printf("sms - could not connect to device\n");
						}
					}
				}
			}
			else
				vai=0;
		}
		while(vai);
		usleep(100000);
	}
}

int loadSMS(bool reload)
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


	if(reload)
	{
		if(shmdt(sms_devices))
			my_perror("shmdt");
		shmctl(SHMSMSDEVICESID, IPC_RMID, &shmid_struct);

		if(shmdt(sms_numbers))
			my_perror("shmdt");
		shmctl(SHMSMSNUMBERSID, IPC_RMID, &shmid_struct);		
	}


	state = mysql_query(conn, "SELECT id,address,port,description "
								" FROM sms_device"
								" WHERE sms_device.enabled=1"
								" LIMIT 0,1");

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);

	NUMSMSDEVICES=mysql_num_rows(result);
	if(NUMSMSDEVICES)
	{
		my_printf("get_shared_memory_segment: sms_devices\n");
		sms_devices=(struct sms_device *)get_shared_memory_segment
			(NUMSMSDEVICES * sizeof(struct sms_device), 
				&SHMSMSDEVICESID, "/dev/zero");
			if(!sms_devices)
				die("sms_devices - get_shared_memory_segment\n");
	}
	else
		sms_devices=0;
	
	i=0;

	if( ( row = mysql_fetch_row(result)) != NULL )
	{
		sms_devices[i].id_sms_device=atoi(row[0]);
		if(row[1])
			strcpy(sms_devices[i].address,row[1]);
		else
			strcpy(sms_devices[i].address,"");
		sms_devices[i].port=atoi(row[2]);
		if(row[3])
			strcpy(sms_devices[i].description,row[3]);
		else
			strcpy(sms_devices[i].description,"");
		sms_devices[i].sockfd=-1;

		sms_devices[i].enabled=1; // per il momento un device sempre enabled

		if(sms_devices[i].enabled)
			sms_devices[i].status='1';
		else
			sms_devices[i].status='d';

		sms_devices[i].failures=0;
	}
	mysql_free_result(result);



	state = mysql_query(conn, "SELECT id,name,number"
								" FROM sms_numbers"
								" WHERE sms_numbers.enabled=1");

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(conn));
		return(1);
	}
	result = mysql_store_result(conn);

	NUMSMSNUMBERS=mysql_num_rows(result);
	if(NUMSMSNUMBERS)
	{
		my_printf("get_shared_memory_segment: sms_numbers\n");
		sms_numbers=(struct sms_number *)get_shared_memory_segment
			(NUMSMSNUMBERS * sizeof(struct sms_number), 
				&SHMSMSNUMBERSID, "/dev/zero");
			if(!sms_numbers)
				die("sms_numbers - get_shared_memory_segment\n");
	}
	else
		sms_numbers=0;
	
	i=0;

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		sms_numbers[i].id_sms_number=atoi(row[0]);
		if(row[1])
			strcpy(sms_numbers[i].name,row[1]);
		else
			strcpy(sms_numbers[i].name,"");
		if(row[2])
			strcpy(sms_numbers[i].number,row[2]);
		else
			strcpy(sms_numbers[i].number,"");
		i++;
	}
	mysql_free_result(result);
	mysql_close(conn);
	return(0);
}


void handleMessage(int adId,
	int id,
	int systemId,
	int channelId,
	char *message, 
	bool is_sms,
	int event_type)
{
/*--------------------------------------------
 * gestisce formattazione, invio e store di messaggi ed eventi
 * int adId  0=analogico, 1=digitale, 2=digital_out, 3=mm ecc
 * int id	id_analog o id_digital o quant'altro da tabella db
 * int systemId		device number
 * int channelId	channel number
 * char *message	messaggio
 * bool is_sms 	send sms?
 * int event_type	-2: alarm back in range, -1: no store, 0:store as alarm, >0:store as event
 * -----------------------------------------*/
	static struct msgform msg;

	switch(event_type)
	{
		case -2:
			valueInRange(adId,systemId,id);
			break;
		case -1:
			break;
		case 0:
			if(storeAlarm(adId,systemId,id,message)<=0)
				is_sms=0;
			break;
		default:
//			storeEvent(event_type,systemId,channelId,id,message);
			break;
	}
	formatMessage(msg.mtext,adId,id,systemId,channelId,message);
	msg.mtype=(is_sms?2:1);
	msgsnd(msg_alarm_id,&msg,MSGLENGTH,IPC_NOWAIT);
}

void checkSMSReload(int id)
{				
	if(sms_devices[id].enabled>1)
	{
		my_printf("requested sms reload, address %s port %d\n",
						sms_devices[id].address,sms_devices[id].port);
		close(sms_devices[id].sockfd);
		sms_devices[id].sockfd=-1;
		sms_devices[id].enabled=1;
		sms_devices[id].status='1';
	}
}

void sms_disconnect(int sms_id)
{
	if(sms_devices[sms_id].sockfd!=-1)
		close(sms_devices[sms_id].sockfd);
	sms_devices[sms_id].sockfd=-1;
	sms_devices[sms_id].status='2';
}


int sendsms(int sockfd,char *number,char *message)
{
	int numbytes,received;
	static char buf[255];
	int len=255;


	my_printf("sms - trying to send %s\n",message);

	bzero(buf,255);
	numbytes=sprintf(buf,"AT+CMGF=1%c",13);
	if(sendAndReceived(sockfd,buf,numbytes,"OK")<0)
		return -1;

	bzero(buf,255);
	numbytes=sprintf(buf,"AT+CMGS=\"%s\"%c",number,13);
	if(sendAndReceived(sockfd,buf,numbytes,">")<0)
		return -1;

	bzero(buf,255);
	numbytes=sprintf(buf,"%s %c",message,26);
	if(sendAndReceived(sockfd,buf,numbytes,message)<0)
		return -1;

	sleep(1);
	bzero(buf,255);
	numbytes=sprintf(buf,"\n%c",13);
	if(sendAndReceived(sockfd,buf,numbytes,"OK")<0)
		return -1;

	return 1;
}



int sendAndReceived(int sockfd,char *out,int numbytes,char *expected)
{
	static char buf[255];
	int received;

	bzero(buf,255);
	if (!send(sockfd,out,numbytes,0)) 
	{
		my_printf("sms - error in sending %s\n",out);
		return -1;
	}
	received=receive(sockfd,buf,255,expected,2);
	if(received<0)
	{
		my_printf("sms - error sending %s\n",out);
		return -1;
	}
	else
		my_printf("sms - ok sending %s\n",out);
	return 0;
}


int initializeSMS(int sockfd)
{
	int numbytes,received;
	char buf[255];
	int len=255;

	bzero(buf,255);
	numbytes=sprintf(buf,"AT+CMEE=1%c",13);
	if(sendAndReceived(sockfd,buf,numbytes,"OK")<0)
		return -1;

	return 0;
}

int doConnectSMS(int id)
{
	int sockfd;

	my_printf("sms - status: %c, sockfd: %d\n",
		sms_devices[id].status,sms_devices[id].sockfd);
	my_printf("trying to connect to sms %s %d\n",
		sms_devices[id].address,sms_devices[id].port);

	sms_devices[id].status='1';
	sockfd=socketConnect(sms_devices[id].address,
									sms_devices[id].port);
	if(sockfd>0)
	{
		if(initializeSMS(sockfd)<0)
		{
			my_printf("sms - could not initialize\n");
			sms_disconnect(0);
			sockfd=-1;
		}
		else
			my_printf("sms - address %s, port %d connected\n",
				sms_devices[0].address, 
				sms_devices[0].port);
	}
	return sockfd;
}


