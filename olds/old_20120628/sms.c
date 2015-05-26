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

	strcpy(message,"connesso");
	msg.mtype=0;
	start_time=0;	//last connection fail

	if(NUMSMSDEVICES>0)
	{
//		printf("trying to connect to sms\n");
		start_time=time(NULL);
		sms_devices[0].sockfd=socketConnect(sms_devices[0].address,
				sms_devices[0].port);
		if(sms_devices[0].sockfd>0)
			printf("address %s, port %d, sms connected\n",sms_devices[0].address, sms_devices[0].port);
	}
/*
		for(i=0;i<NUMSMSNUMBERS;i++)
		{
			if(sendsms(sms_devices[0].sockfd,
					sms_numbers[i].number,message)<0)
			{
				close(sms_devices[0].sockfd);
				sms_devices[0].sockfd=-1;
				start_time=time(NULL);
			}
		}*/
	


	while(1)
	{
		//leggo coda dei messaggi e scrivo su log
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
//							printf("trying to connect to sms\n");
							start_time=time(NULL);
							sms_devices[0].sockfd=socketConnect(sms_devices[0].address,
								sms_devices[0].port);
						}
						if(sms_devices[0].sockfd>0)
						{
							printf("address %s, port %d, sms connected\n",sms_devices[0].address, sms_devices[0].port);
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
			perror("shmdt");
		shmctl(SHMSMSDEVICESID, IPC_RMID, &shmid_struct);

		if(shmdt(sms_numbers))
			perror("shmdt");
		shmctl(SHMSMSNUMBERSID, IPC_RMID, &shmid_struct);		
	}


	state = mysql_query(connection, "SELECT id,address,port,description"
								" FROM sms_device"
								" WHERE sms_device.enabled=1"
								" LIMIT 0,1");

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

	NUMSMSDEVICES=mysql_num_rows(result);
	if(NUMSMSDEVICES)
	{
		printf("get_shared_memory_segment: sms_devices\n");
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
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

	NUMSMSNUMBERS=mysql_num_rows(result);
	if(NUMSMSNUMBERS)
	{
		printf("get_shared_memory_segment: sms_numbers\n");
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
