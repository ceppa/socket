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
	time_t start_time;
	int i;
	struct msgform msg;
	char message[160];
	char *c;
	char tempLogFileName[80];
	struct stat statbuf;


	logFileFd=-1;
	strcpy(logPath,"/var/www/domotica/log/");
	getLogFileName(logFileName,logPath);
	logFileFd=open(logFileName,O_WRONLY | O_CREAT | O_APPEND, 0644);
	fstat(logFileFd,&statbuf);
	if(statbuf.st_size==0)
		scriviEffemeridiSuFile(logFileFd);


	strcpy(message,"connesso");
	msg.mtype=0;
	start_time=0;	//last connection fail

	if(NUMSMSDEVICES>0)
	{
//		my_printf("trying to connect to sms\n");
		start_time=time(NULL);
		sms_devices[0].sockfd=socketConnect(sms_devices[0].address,
				sms_devices[0].port);
		if(sms_devices[0].sockfd>0)
			my_printf("address %s, port %d, sms connected\n",sms_devices[0].address, sms_devices[0].port);
	}


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
			if(msgrcv(msgid,&msg,MSGLENGTH,0,IPC_NOWAIT)!=-1)
			{
				write(logFileFd,msg.mtext,strlen(msg.mtext));
				if(msg.mtype>1)	//is sms
				{
					if(NUMSMSDEVICES>0)
					{
						if((sms_devices[0].sockfd<0)&&(time(NULL)-start_time>2))
						{
//							my_printf("trying to connect to sms\n");
							start_time=time(NULL);
							sms_devices[0].sockfd=socketConnect(sms_devices[0].address,
								sms_devices[0].port);
						}
						if(sms_devices[0].sockfd>0)
						{
							my_printf("address %s, port %d, sms connected\n",sms_devices[0].address, sms_devices[0].port);
							for(i=0;i<NUMSMSNUMBERS;i++)
							{
								if((c=strrchr(msg.mtext,'\t'))==0)
									c=msg.mtext;
								else 
									c++;
								if(sendsms(sms_devices[0].sockfd,
									sms_numbers[i].number,c)<0)
								{
									close(sms_devices[0].sockfd);
									sms_devices[0].sockfd=-1;
									start_time=time(NULL);
								}
							}
						}
					}
				}
			}
			else
				vai=0;
		}
		while(vai);
		usleep(10000);
	}
}

int loadSMS(bool reload)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	

	if(reload)
	{
		if(shmdt(sms_devices))
			my_perror("shmdt");
		shmctl(SHMSMSDEVICESID, IPC_RMID, &shmid_struct);

		if(shmdt(sms_numbers))
			my_perror("shmdt");
		shmctl(SHMSMSNUMBERSID, IPC_RMID, &shmid_struct);		
	}


	state = mysql_query(connection, "SELECT id,address,port,description"
								" FROM sms_device"
								" WHERE sms_device.enabled=1"
								" LIMIT 0,1");

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

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
	}
	mysql_free_result(result);



	state = mysql_query(connection, "SELECT id,name,number"
								" FROM sms_numbers"
								" WHERE sms_numbers.enabled=1");

	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

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
 * int adId  0=analogico, 1=digitale, 2=digital_out
 * int id	id da tabella db
 * int systemId		device number
 * int channelId	channel number
 * char *message	messaggio
 * bool is_sms 	send sms?
 * int event_type	-1: no store, 0:store as alarm, >0:store as event
 * -----------------------------------------*/
	struct msgform msg;

	formatMessage(msg.mtext,adId,id,systemId,channelId,message);
	msg.mtype=(is_sms?2:1);
	msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
	switch(event_type)
	{
		case -1:
			break;
		case 0:
			storeAlarm(adId,id,systemId,channelId,message);
			break;
		default: 
			storeEvent(event_type,systemId,channelId,id,message);
			break;
	}
}
