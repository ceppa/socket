#include "client.h"


void sigchld_handler(int s)
/*--------------------------------------------
 * aggiorna array dei pid alla morte dei figli
 * controlla tutti i figli, se morto mette pid a -1
 * -----------------------------------------*/
{
	int i;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	if(getpid()==mypid)  //I'm parent
	{
		for(i=0;i<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS;i++)
			if((pid[i]!=-1)&&(kill(pid[i],0)==-1))
			{
				printf("system %d died\n",pid[i]);
				pid[i]=-1;
			}

		for(i=0;i<MAXCONNECTIONS;i++)
			if((serverPids[i]!=-1)&&(kill(serverPids[i],0)==-1))
			{
				printf("connection %d closed\n",serverPids[i]);
				serverPids[i]=-1;
			}
		if((serverPid!=-1)&&(kill(serverPid,0)==-1))
		{
			printf("server %d died\n",serverPid);
			serverPid=-1;
		}
	
		if((messagesPid!=-1)&&(kill(messagesPid,0)==-1))
		{
			printf("sms server %d died\n",messagesPid);
			messagesPid=-1;
		}
	
		if((controllerPid!=-1)&&(kill(controllerPid,0)==-1))
		{
			printf("controller %d died\n",controllerPid);
			controllerPid=-1;
		}
	}

}


void termination_handler (int signum)
/*--------------------------------------------
 * pulizia prima di uscire
 * libera la memoria condivisa
 * chiude mysql
 * -----------------------------------------*/
{
	int i;
	int status;
	char writeBuffer[MSGLENGTH];
	int temp;
	
	printf("signal %d to %d\n",signum,getpid());

	if(getpid()==messagesPid)
	{
		formatMessage(writeBuffer,0,0,0,0,"SYSTEM STOPPING");
		write(logFileFd,writeBuffer,strlen(writeBuffer));
		close(logFileFd);
	}
	if(getpid()==mypid)		//sono il padre
	{
		printf("ho ricevuto %d\n",signum);
		if(signum==SIGSEGV)
			printf("error %s, non bene, ho ricevuto sigsegv\n",strerror(errno));

		killAllSystems();

		if(signum==2)
		{
			//libera la struttura di antiintrusione
			printf("freeing ai system nodes\n");
			free_system_nodes();
		}

		msgctl(msgid, IPC_RMID, 0);
		mysql_close(connection);
		
		temp=NUMPANELS;
		NUMPANELS=0;
		for(i=0;i<temp;i++)
			free(panels[i]);
		if(panels)
			free(panels);
		panels=NULL;

		freeshm();

		printf("Fine\n");
	}
	else
	{
		if(getpid()==serverPid)
		{
			for(i=0;i<MAXCONNECTIONS;i++)
				if(serverPids[i]>0)
					killPid(serverPids[i]);
		}
	}
	exit(1);
}

void readSystemDefaults(MYSQL *connection)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;

	//READ SYSTEM DEFAULTS
	state = mysql_query(connection, "SELECT record_data_time,localita,nome,reboot_time,offset_effe FROM system_ini");

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		termination_handler(0);
	}

	result = mysql_store_result(connection);

	if( ( row = mysql_fetch_row(result)) != NULL )
	{
		RECORDDATATIME=60*atoi(row[0]);

		if(row[1])
			strcpy(LOCALITA,row[1]);
		else
			strcpy(LOCALITA,"");

		if(row[2])
			strcpy(NOME,row[2]);
		else
			strcpy(NOME,"");

		MAXATTEMPTS=atoi(row[3])/2;
		OFFSET_EFFE=atoi(row[4]);
	}
	mysql_free_result(result);
//END READ SYSTEM DEFAULTS
}

MYSQL *mysqlConnect()
{
//MYSQL STUFF
	MYSQL *connection;

	mysql_init(&mysql);
	my_bool reconnect = 1;
	mysql_options(&mysql, MYSQL_OPT_RECONNECT, &reconnect);
	connection = mysql_real_connect(&mysql,"localhost","root", "minair","socket", 0, NULL,0);
	if( connection == NULL ) 
	{
		printf("%s\n",mysql_error(&mysql));
		termination_handler(0);
	}
	return connection;
}


int main(int argc, char *argv[])
{
	struct sigaction sa;
	
	time_t db_counter;
	time_t db_counter_now;
	
	struct msgform msg;
	char message[160];

	if (signal (SIGINT, termination_handler) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	if (signal (SIGSEGV, termination_handler) == SIG_IGN)
		signal (SIGSEGV, SIG_IGN);
	if (signal (SIGHUP, termination_handler) == SIG_IGN)
		signal (SIGHUP, SIG_IGN);
	if (signal (SIGTERM, termination_handler) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);


	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		termination_handler(0);
	}

	msgid=msgget(MSGKEY,0777|IPC_CREAT); /* ottiene un descrittore per la chiave KEY; coda nuova */
	msg.mtype=1;   //not sms

	mypid=getpid();

/*	if(getenv("HOME")!=NULL)
		sprintf(logPath,"%s/log/",getenv("HOME"));
	else
		strcpy(logPath,"./log/");*/
	
	printf("get_shared_memory_segment: reloadCommand\n");
	reloadCommand=(bool *)get_shared_memory_segment(1, &SHMRELOADCOMMAND, "/dev/zero");
	if(!reloadCommand)
			die("reloadCommand - get_shared_memory_segment\n");

//
	*reloadCommand=0;		//così all'inizio parte server
	while(1)
	{
		loadAllSystems();
		startAllSystems();


		*reloadCommand=0;
		db_counter=time(NULL);
		do
		{
			// fa qualcosa qui
			*buonGiornoAttivo=checkBuonGiornoAttivo();
			*buonaNotteAttivo=checkBuonaNotteAttivo();
	
			// ora attendo solo la morte dei figli
			usleep(10000);
	

			//salvo valori storici ad intervalli regolari
			db_counter_now=time(NULL);
			if((db_counter_now - db_counter >= RECORDDATATIME)
					&& ((db_counter_now % RECORDDATATIME) <50))
			{
				storeTables();
				db_counter=time(NULL);
			}
		
			if(checkSystems(pid)!=NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS)
				printf("Not all children are alive\nYou had better restart\n");
		}
		while(*reloadCommand==0);
		killAllSystems();
		printf("fine sistemi\n");
		freeshm();
	}

//
}

int checkSystems(int *pidArray)
/*--------------------------------------------
 * controlla lo stato dei figli ed eventualmente 
 * aggiorna l'array dei pid
 * int *pidArray - array con i pid dei figli
 * -----------------------------------------*/
{
	int status;
	int out=0;
	int k;
	for(k=0;k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS;k++)
	{
		if(pidArray[k]!=-1)
			if(waitpid(pidArray[k],&status,WNOHANG)==pidArray[k])
				pidArray[k]=-1;
		out+=(pidArray[k]!=-1);
	}
	return out;
}


void killSystems(int *pidArray)
/*--------------------------------------------
 * uccide i figli all'uscita per non lasciare zombie
 * int *pidArray - array con i pid dei figli
 * -----------------------------------------*/
{
	int k,l;
	int status;
	
	for(k=0;k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS;k++)
	{
		if(k<NUMSYSTEMS)	// tabella system
		{
			if(systems[k].sockfd!=-1)
				close(systems[k].sockfd);
		}
		else 
		{
			if(k<NUMSYSTEMS+NUMMULTIMETERS)//multimetro
			{
				l=k-NUMSYSTEMS;
				if(multimeters[l].sockfd!=-1)
					close(multimeters[l].sockfd);
			}
			else 	//knx
			{
				l=k-NUMSYSTEMS-NUMMULTIMETERS;
				if(knxGateways[l].sockfd!=-1)
					close(knxGateways[l].sockfd);
			}
		}
		killPid(pidArray[k]);
		pidArray[k]=-1;
/*		if(pidArray[k]!=-1)
		{
			
			kill(pidArray[k],SIGKILL);
			if(waitpid(pidArray[k],&status,0)==pidArray[k])
				pidArray[k]=-1;
		}*/
	}
}




void storeEvent(int event_type,int device_num,int ch_num,int ch_id,char *msg)
/*--------------------------------------------
 * salva evento su tabella events
 * int event_type	1=irrigazione, 2=circuito_irrigazione
 * int id	id da tabella db
 * int device_num	
 * int ch_num
 * int ch_id
 * char *msg	messaggio
 * -----------------------------------------*/
{
	char *query;

	query=(char *)malloc(strlen(msg)+255);
	sprintf(query,"INSERT INTO events(data,event_type,device_num,ch_num,ch_id,msg) "
					"VALUES (NOW(),'%d','%d','%d','%d','%s')",event_type,device_num,ch_num,ch_id,msg);
	if(mysql_query(connection,query)!=0)
		printf("%s\n%s\n",query,mysql_error(connection));
	free(query);
}

void storeAlarm(int adId,int id,int systemId,int channelId,char *msg)
{
	int id_analog=0;
	int id_digital=0;
	char query[255];
	
	if(adId)
		id_digital=id;
	else
		id_analog=id;

	sprintf(query,"call insertMsg(%d,%d,%d,%d,'%s')",systemId,channelId,id_analog,id_digital,msg);
	if(mysql_query(connection,query)!=0)
		printf("%s\n",mysql_error(connection));
}

void formatMessage(char *writeBuffer,int adId,int id,int systemId,int channelId,char *msg)
/*--------------------------------------------
 * formatta il messaggio per la scrittura su file
 * char *writeBuffer	buffer
 * int adId  0=analogico, 1=digitale, 2=digital_out
 * int id	id da tabella db
 * int systemId	
 * int channelId
 * char *msg	messaggio
 * -----------------------------------------*/
{

	char temp[10];
	char tempString[MSGLENGTH];
	
	getTimeString("%H:%M:%S",temp);
	sprintf(writeBuffer,"%s\t%d\t%d\t%d\t%d\t%s\n",temp,id,adId,systemId,channelId,msg);
}


void storeTables()
{
	char datetime[20];
	char query[255];
	int i;
	
	getTimeString("%Y-%m-%d %H:%M:%S",datetime);
	for(i=0;i<ANALOGCHANNELS;i++)
	{
		if((systems[systemNumToId(analogTable[i].device_num,NUMSYSTEMS)].enabled)
			&&(analogTable[i].enabled)
			&&(analogTable[i].id_analog!=-1))
		{
		
			sprintf(query,"INSERT INTO history(`datetime`,`device_num`,`ch_num`,`id_analog`,`value`,`raw`,`um`)" 
							" VALUES ('%s',%d,%d,%d,%f,%d,'%s')",datetime,analogTable[i].device_num,
								analogTable[i].ch_num,analogTable[i].id_analog,analogTable[i].value_eng,
								analogTable[i].value,analogTable[i].unit);
								
			if(mysql_query(connection,query)!=0)
				printf("%s\n",mysql_error(connection));
		}
	}
	for(i=0;i<DIGITALCHANNELS;i++)
	{
		if((systems[systemNumToId(digitalTable[i].device_num,NUMSYSTEMS)].enabled)
			&&(digitalTable[i].enabled)
			&&(digitalTable[i].id_digital!=-1))
		{
			sprintf(query,"INSERT INTO history(`datetime`,`device_num`,`ch_num`,`id_digital`,`value`,`raw`)" 
							" VALUES ('%s',%d,%d,%d,%d,%d)",datetime,digitalTable[i].device_num,
								digitalTable[i].ch_num,digitalTable[i].id_digital,digitalTable[i].value,
								digitalTable[i].value);
			if(mysql_query(connection,query)!=0)
				printf("%s\n",mysql_error(connection));
		}
	}
}


void get_sottostato(int fd,char *buf)
/*--------------------------------------------
 * 		restituisce la stringa relativa al comano
 * 		get sottostato inizio_estate inizio_inverno
 * -----------------------------------------*/
{
	bool antiintrusione=1;
	time_t timer;
	struct tm now;
	int dawn;
	int sunset;
	int giorno;
	int minuti;
	int inizio_estate;
	int inizio_inverno;
	int estate=1;
	char *c;
	char buffer[255];
	int pioggia=-1;

	if(*id_digital_pioggia!=-1)
	{
		if(digitalTable[*id_digital_pioggia].value!=-1)
			pioggia=(digitalTable[*id_digital_pioggia].value==1);
	}

	if(c=strchr(buf,' '))
	{
		*c='\0';
		c++;
		inizio_estate=atoi(buf);
		inizio_inverno=atoi(c);
	}
	else
	{
		inizio_estate=2103;
		inizio_inverno=2109;
	}

	timer=time(NULL);
	now=*localtime(&timer);

	estate=((((now.tm_mon+1)>(inizio_estate % 100))&&((now.tm_mon+1)<(inizio_inverno % 100)))
		||(((now.tm_mon+1)==(inizio_estate % 100))&&(now.tm_mday>=div(inizio_estate,100).quot))
		||(((now.tm_mon+1)==(inizio_inverno % 100))&&(now.tm_mday<div(inizio_inverno,100).quot)));

	dawn=effemeridiTable[now.tm_mon][now.tm_mday - 1].dawn;
	sunset=effemeridiTable[now.tm_mon][now.tm_mday - 1].sunset;
	minuti=(now.tm_hour-daylight)*60+now.tm_min;

	giorno=((minuti>=dawn)&&(minuti<=sunset));
	
	sprintf(buffer,"%d %d %d %d %s",antiintrusione,pioggia,giorno,estate,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		perror("send");
	return;
}

void die(char *text)
{
	perror(text);
	termination_handler(0);
}

void killPid(int pid)
{
	int status;
	if(pid!=-1)
	{
		if(kill(pid,0)!=-1)
		{
			kill(pid,SIGTERM);
			waitpid(pid,&status,0);	
		}
	}
}

void killAllSystems()
{
	printf("killing controller\n");
	killPid(controllerPid);
	printf("killing systems\n");
	killSystems(pid);
	printf("killing server\n");
	killPid(serverPid);
	printf("killing sms\n");
	killPid(messagesPid);
}

void freeshm()
{
	if(*reloadCommand==0)
	{
		if(reloadCommand&&(shmdt(reloadCommand)==-1))
			perror("reloadCommand");
		reloadCommand=0;
		shmctl(SHMRELOADCOMMAND, IPC_RMID, &shmid_struct);
	}

	if(sms_devices&&(shmdt(sms_devices)==-1))
		perror("sms_devices");
	sms_devices=0;
	shmctl(SHMSMSDEVICESID, IPC_RMID, &shmid_struct);

	if(sms_numbers&&(shmdt(sms_numbers)==-1))
		perror("sms_numbers");
	sms_numbers=0;
	shmctl(SHMSMSNUMBERSID, IPC_RMID, &shmid_struct);

	if(systems&&(shmdt(systems)==-1))
		perror("shmdt systems");
	systems=0;
	shmctl(SHMSYSTEMSID, IPC_RMID, &shmid_struct);

	if(multimeters&&(shmdt(multimeters)==-1))
		perror("shmdt multimeters");
	multimeters=0;
	shmctl(SHMMULTIMETERSID, IPC_RMID, &shmid_struct);

	if(analogTable&&(shmdt(analogTable)==-1))
		perror("shmdt analogtable");
	analogTable=0;
	shmctl(SHMANALOGID, IPC_RMID, &shmid_struct);

	if(knxGateways&&(shmdt(knxGateways)==-1))
		perror("shmdt knxgateways");
	knxGateways=0;
	shmctl(SHMKNXGATEWAYSID, IPC_RMID, &shmid_struct);

	if(knxTable&&(shmdt(knxTable)==-1))
		perror("shmdt knxtable");
	knxTable=0;
	shmctl(SHMKNXTABLEID, IPC_RMID, &shmid_struct);

	if(readingTable&&(shmdt(readingTable)==-1))
		perror("shmdt readingtable");
	readingTable=0;
	shmctl(SHMREADINGID, IPC_RMID, &shmid_struct);

	if(digitalTable&&(shmdt(digitalTable)==-1))
		perror("shmdt digitaltable");
	digitalTable=0;
	shmctl(SHMDIGITALID, IPC_RMID, &shmid_struct);

	if(digitalOutTable&&(shmdt(digitalOutTable)==-1))
		perror("shmdt digitalouttable");
	digitalOutTable=0;
	shmctl(SHMDIGITALOUTID, IPC_RMID, &shmid_struct);

	if(scenariPresenzeTable&&(shmdt(scenariPresenzeTable)==-1))
		perror("shmdt scenaipresenzetable");
	scenariPresenzeTable=0;
	shmctl(SHMSCENARIPRESENZEID, IPC_RMID, &shmid_struct);

	if(scenariPresenzeAttivo&&(shmdt(scenariPresenzeAttivo)==-1))
		perror("shmdt scenariPresenzeAttivo");
	scenariPresenzeAttivo=0;
	shmctl(SHMSCENARIPRESENZEATTIVOID, IPC_RMID, &shmid_struct);

	if(scenariBgBnTable&&(shmdt(scenariBgBnTable)==-1))
		perror("shmdt scenarigbgntable");
	scenariBgBnTable=0;
	shmctl(SHMSCENARIBGBNID, IPC_RMID, &shmid_struct);

	if(irrigazioneCircuitiTable&&(shmdt(irrigazioneCircuitiTable)==-1))
		perror("shmdt irrigazionecircuititable");
	irrigazioneCircuitiTable=0;
	shmctl(SHMIRRIGAZIONECIRCUITIID, IPC_RMID, &shmid_struct);

	if(irrigazioneTable&&(shmdt(irrigazioneTable)==-1))
		perror("shmdt irrigazionetable");
	irrigazioneTable=0;
	shmctl(SHMIRRIGAZIONEID, IPC_RMID, &shmid_struct);

	if(id_digital_pioggia&&(shmdt(id_digital_pioggia)==-1))
		perror("shmdt id_digital_pioggia");
	id_digital_pioggia=0;
	shmctl(SHMPIOGGIA, IPC_RMID, &shmid_struct);

	if(buonGiornoAttivo&&(shmdt(buonGiornoAttivo)==-1))
		perror("shmdt buonGiornoAttivo");
	buonGiornoAttivo=0;
	shmctl(SHMSCENARIBGATTIVOID, IPC_RMID, &shmid_struct);

	if(buonaNotteAttivo&&(shmdt(buonaNotteAttivo)==-1))
		perror("shmdt buonaNotteAttivo");
	buonaNotteAttivo=0;
	shmctl(SHMSCENARIBNATTIVOID, IPC_RMID, &shmid_struct);

	if(ai_sistemi&&(shmdt(ai_sistemi)==-1))
		perror("shmdt ai_sistemi");
	ai_sistemi=0;
	shmctl(SHMAISYSTEMS, IPC_RMID, &shmid_struct);
}

void loadAllSystems()
{
	int i=0,j=0,k,l;
	struct timeval tim;
	double t1;
	int knx_ch_in;
	int knx_ch_out;

	panels=NULL;
	NUMPANELS=0;
	NUMSMSDEVICES=0;
	NUMSMSNUMBERS=0;

	NUMKNXCHANNELS=0;
	ANALOGCHANNELS=0;
	DIGITALCHANNELS=0;
	DIGITALOUTCHANNELS=0;
	READINGCHANNELS=0;
	SCENARIPRESENZECOUNT=0;
	SCENARIBGBNCOUNT=0;
	IRRIGAZIONECIRCUITS=0;
	IRRIGAZIONESYSTEMS=0;
	MAXATTEMPTS=5;
	OFFSET_EFFE=0;
	LOCALITA[30];
	RECORDDATATIME=300;
	RNDSEED=0;
	connection=NULL;

	gettimeofday(&tim, NULL);
	t1=tim.tv_sec+(tim.tv_usec/1000000.0);
//MYSQL STUFF
	connection=mysqlConnect();
//READ SYSTEM DEFAULTS (needs mysql);
	readSystemDefaults(connection);
//READ EFFEMERIDI (needs mysql);
	readEffemeridi(connection);

	if(loadSMS(0))
		termination_handler(0);

	if(loadSystemTable(0))
		termination_handler(0);

	if(loadMultimeterTable(0))
		termination_handler(0);

	if(loadKnx(0))
		termination_handler(0);

	for(i=0;i<NUMMULTIMETERS;i++)
	{
		printf("%d %s\n",multimeters[i].multimeter_num,multimeters[i].address);
		for(j=0;j<multimeters[i].in_bytes_1_length;j++)
			printf("%x ",multimeters[i].in_bytes_1[j]);
		printf("\n");
		for(j=0;j<multimeters[i].in_bytes_2_length;j++)
			printf("%x ",multimeters[i].in_bytes_2[j]);
		printf("\n");
	}

	pid=(int *)malloc((NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS) * sizeof(int));
	deviceToid=(int ***)malloc(6*sizeof(int **));
	deviceToid[0]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //analog
	deviceToid[1]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //digital
	deviceToid[2]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //digital_out
	deviceToid[3]=(int **)malloc(NUMMULTIMETERS * sizeof(int *)); //mm
	deviceToid[4]=(int **)malloc(NUMKNXGATEWAYS * sizeof(int *)); //knx_in
	deviceToid[5]=(int **)malloc(NUMKNXGATEWAYS * sizeof(int *)); //knx_out
	for(i=0;i<NUMSYSTEMS;i++)
	{
		deviceToid[0][i]=(int *)malloc(systems[i].in_ch_an*sizeof(int));
		for(k=0;k<systems[i].in_ch_an;k++)
			deviceToid[0][i][k]=-1;
		deviceToid[1][i]=(int *)malloc(systems[i].in_ch_d*sizeof(int));
		for(k=0;k<systems[i].in_ch_d;k++)
			deviceToid[1][i][k]=-1;
		deviceToid[2][i]=(int *)malloc(systems[i].out_ch_d*sizeof(int));
		for(k=0;k<systems[i].out_ch_d;k++)
			deviceToid[2][i][k]=-1;
	}
	for(i=0;i<NUMMULTIMETERS;i++)
	{
		deviceToid[3][i]=(int *)malloc((multimeters[i].out_ch_1+multimeters[i].out_ch_2)*sizeof(int));
		for(k=0;k<multimeters[i].out_ch_1+multimeters[i].out_ch_2;k++)
			deviceToid[3][i][k]=-1;
	}
	for(i=0;i<NUMKNXGATEWAYS;i++)
	{
		deviceToid[4][i]=(int *)malloc((knxGateways[i].ch_in)*sizeof(int));
		for(k=0;k<knxGateways[i].ch_in;k++)
			deviceToid[4][i][k]=-1;
		deviceToid[5][i]=(int *)malloc((knxGateways[i].ch_out)*sizeof(int));
		for(k=0;k<knxGateways[i].ch_out;k++)
			deviceToid[5][i][k]=-1;
	}
	knx_ch_in=0;
	knx_ch_out=0;
	for(k=0;k<NUMKNXCHANNELS;k++)
	{
		l=id_knx_gatewayToId(knxTable[k].id_knx_gateway);
		if(l!=-1)
		{
			if(knxTable[k].input_output==1)
			{
				deviceToid[4][l][knx_ch_in]=k;
				knx_ch_in++;
			}
			else
			{
				deviceToid[5][l][knx_ch_out]=k;
				knx_ch_out++;
			}
		}
	}

/*
	printf("ANALOGCHANNELS - DIGITALCHANNELS - DIGITALOUTCHANNELS - READINGCHANNELS\n");
	printf("%d - %d - %d - %d\n",ANALOGCHANNELS,DIGITALCHANNELS,DIGITALOUTCHANNELS,READINGCHANNELS);
	*/
	if(loadDigitalOutTable(0))
		termination_handler(0);
	if(loadScenariPresenzeTable(0))
		termination_handler(0);
	if(loadScenariBgBnTable(0))
		termination_handler(0);
	if(loadDigitalTable(0))
		termination_handler(0);
	if(loadAnalogTable(0))
		termination_handler(0);
	if(loadReadingTable(0))
		termination_handler(0);
	if(loadIrrigazioneTables(0,0))
		termination_handler(0);
	if(loadIntrusione())
		termination_handler(0);

/*
	printf("--- PANELS ---\n");
	loadPanels();
	for(i=0;i<NUMPANELS;i++)
		for(j=0;j<16;j++)
			printf("%d %d %d\n",i,j,panels[i][j]);

	printf("--- ANALOG ---\n");
	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].in_ch_an;j++)
			if(analogTable[deviceToid[0][i][j]].id_analog!=-1)
				printf("%d %d %d %s %d\n",analogTable[deviceToid[0][i][j]].id_analog,
					analogTable[deviceToid[0][i][j]].device_num,
					analogTable[deviceToid[0][i][j]].ch_num,
					analogTable[deviceToid[0][i][j]].description,
					analogTable[deviceToid[0][i][j]].enabled);

	printf("--- DIGITAL ---\n");
	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].in_ch_d;j++)
			if(digitalTable[deviceToid[1][i][j]].id_digital!=-1)
				printf("%d %d %s %d\n",digitalTable[deviceToid[1][i][j]].device_num,
					digitalTable[deviceToid[1][i][j]].ch_num,
					digitalTable[deviceToid[1][i][j]].description,
					digitalTable[deviceToid[1][i][j]].enabled);

*/
	printf("--- DIGITALOUT ---\n");
	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].out_ch_d;j++)
			if(digitalOutTable[deviceToid[2][i][j]].id_digital_out!=-1)
				printf("%d %d %d %s %d %d %d - %d %d\n",
					digitalOutTable[deviceToid[2][i][j]].id_digital_out,
					digitalOutTable[deviceToid[2][i][j]].device_num,
					digitalOutTable[deviceToid[2][i][j]].ch_num,
					digitalOutTable[deviceToid[2][i][j]].description,
					digitalOutTable[deviceToid[2][i][j]].def,
					digitalOutTable[deviceToid[2][i][j]].value,
					digitalOutTable[deviceToid[2][i][j]].id_digital,
					digitalOutTable[deviceToid[2][i][j]].po_delay,
					digitalOutTable[deviceToid[2][i][j]].on_time);
/*
	printf("--- MM ---\n");
	for(i=0;i<NUMMULTIMETERS;i++)
		for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
			if(readingTable[deviceToid[3][i][j]].id_reading!=-1)
				printf("%d %d %d %s %d\n",readingTable[deviceToid[3][i][j]].id_reading,
					readingTable[deviceToid[3][i][j]].multimeter_num,
					readingTable[deviceToid[3][i][j]].ch_num,
					readingTable[deviceToid[3][i][j]].description,
					readingTable[deviceToid[3][i][j]].enabled);

	printf("--- KNX IN ---\n");
	for(i=0;i<NUMKNXGATEWAYS;i++)
		for(j=0;j<knxGateways[i].ch_in;j++)
			if(knxTable[deviceToid[4][i][j]].id_knx_line!=-1)
				printf("%d %d %s %c %s\n",knxTable[deviceToid[4][i][j]].id_knx_line,
					knxTable[deviceToid[4][i][j]].id_knx_gateway,
					knxTable[deviceToid[4][i][j]].group_address,
					knxTable[deviceToid[4][i][j]].data_type,
					knxTable[deviceToid[4][i][j]].description);

	printf("--- KNX OUT ---\n");
	for(i=0;i<NUMKNXGATEWAYS;i++)
		for(j=0;j<knxGateways[i].ch_out;j++)
			if(knxTable[deviceToid[5][i][j]].id_knx_line!=-1)
				printf("%d %d %s %c %s\n",knxTable[deviceToid[5][i][j]].id_knx_line,
					knxTable[deviceToid[5][i][j]].id_knx_gateway,
					knxTable[deviceToid[5][i][j]].group_address,
					knxTable[deviceToid[5][i][j]].data_type,
					knxTable[deviceToid[5][i][j]].description);

	printf("--- SCENARIPRESENZE ---\n");
	for(i=0;i<SCENARIPRESENZECOUNT;i++)
		printf("%d %d %d %d %d %d %d\n",scenariPresenzeTable[i].id_digital_out,
					scenariPresenzeTable[i].attivo,
					scenariPresenzeTable[i].ciclico,
					scenariPresenzeTable[i].tempo_on,
					scenariPresenzeTable[i].tempo_off,
					scenariPresenzeTable[i].ora_ini,
					scenariPresenzeTable[i].ora_fine);

	printf("--- SCENARIBGBN ---\n");
	for(i=0;i<SCENARIBGBNCOUNT;i++)
		printf("%d %d %d %d\n",scenariBgBnTable[i].id_digital_out,
					scenariBgBnTable[i].attivo,
					scenariBgBnTable[i].ritardo,
					scenariBgBnTable[i].bg);

	printf("--- IRRIGAZIONE ---\n");
	for(i=0;i<IRRIGAZIONESYSTEMS;i++)
		printf("%d %d %d %d %d %d %d\n",
			irrigazioneTable[i].id_irrigazione,
			irrigazioneTable[i].ora_start,
			irrigazioneTable[i].ripetitivita,
			irrigazioneTable[i].tempo_off,
			irrigazioneTable[i].id_digital_out,
			irrigazioneTable[i].num_circuiti,
			IRRIGAZIONECIRCUITS);
	for(i=0;i<IRRIGAZIONECIRCUITS;i++)
	{
		if(irrigazioneCircuitiTable[i].id_irrigazione!=-1)
			printf("%d %d %d %d %d\n",
				irrigazioneCircuitiTable[i].id_irrigazione,
				irrigazioneCircuitiTable[i].circuito,
				irrigazioneCircuitiTable[i].id_digital_out,
				irrigazioneCircuitiTable[i].durata,
				irrigazioneCircuitiTable[i].validita);
	}

	printf("--- SMS ---\n");
	for(i=0;i<NUMSMSDEVICES;i++)
		printf("%d %s %d %s\n",
			sms_devices[i].id_sms_device,
			sms_devices[i].address,
			sms_devices[i].port,
			sms_devices[i].description);
	for(i=0;i<NUMSMSNUMBERS;i++)
		printf("%d %s %s\n",
			sms_numbers[i].id_sms_number,
			sms_numbers[i].name,
			sms_numbers[i].number);
*/
	printf("pid: %d\n",getpid());
	gettimeofday(&tim, NULL);
	printf("%.6f seconds to initialise\n",(tim.tv_sec+(tim.tv_usec/1000000.0))-t1);

//FINE MYSQL STUFF
}


void startAllSystems()
{
	int k,l;
	int status;
	char writeBuffer[MSGLENGTH];

	//fork to act as sms sender
	if((messagesPid=fork())==0)
	{
		messagesPid=getpid();
		doMessageHandler();
		wait(&status);
		exit(0);
	}
	printf("sms sender pid %d\n",messagesPid);


	handleMessage(0,0,0,0,"SYSTEM STARTING",0,-1);

	for(k=0;k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS;k++)
	{
		pid[k]=-1;
		
		switch(pid[k]=fork()) // fork for every system or multimeter or knx
		{
			case -1:
				perror("fork!");
				termination_handler(0);
				break;
			case 0:
				pid[k]=getpid();
				if(k<NUMSYSTEMS)	// tabella system
					doSystem(k);
				else 
				{
					if(k<NUMSYSTEMS+NUMMULTIMETERS)//multimetro
					{
						l=k-NUMSYSTEMS;
						doMultimeter(l);
					}
					else 	//knx
					{
						l=k-NUMSYSTEMS-NUMMULTIMETERS;
						doKNX(l);
					}
				}
				exit(0);
				break;
			default:
//				printf("padre :%d\n",getpid());

				if(k<NUMSYSTEMS)	// tabella system
					printf("system pid %d\n",pid[k]);
				else 
				{
					if(k<NUMSYSTEMS+NUMMULTIMETERS)//multimetro
						printf("multimeter pid %d\n",pid[k]);
					else 	//knx
						printf("knx pid %d\n",pid[k]);
				}
				break;
		}
	}

	//fork to act as controller
	if((controllerPid=fork())==0)
	{
		controllerPid=getpid();
		doController();
		wait(&status);
		exit(0);
	}	
	printf("controller pid %d\n",controllerPid);
	

	//fork to act as a server
	if((serverPid=fork())==0)
	{
		serverPid=getpid();
		doServer();
		wait(&status);
		exit(0);
	}
	printf("server pid %d\n",serverPid);

}
