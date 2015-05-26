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
		for(i=0;i<TOTALSYSTEMS;i++)
			if((pid[i]!=-1)&&(kill(pid[i],0)==-1))
			{
				my_printf("system %d died\n",pid[i]);
				pid[i]=-1;
			}

		for(i=0;i<MAXCONNECTIONS;i++)
			if((serverPids[i]!=-1)&&(kill(serverPids[i],0)==-1))
			{
				my_printf("connection %d closed\n",serverPids[i]);
				serverPids[i]=-1;
			}
		if((serverPid!=-1)&&(kill(serverPid,0)==-1))
		{
			my_printf("server %d died\n",serverPid);
			serverPid=-1;
		}

		if((messagesPid!=-1)&&(kill(messagesPid,0)==-1))
		{
			my_printf("sms server %d died\n",messagesPid);
			messagesPid=-1;
		}
	
		if((controllerPid!=-1)&&(kill(controllerPid,0)==-1))
		{
			my_printf("controller %d died\n",controllerPid);
			controllerPid=-1;
		}
	}

}

void kill_handler(int s)
{
	my_printf("sono %d ricevo %d ed esco\n",getpid(),s);
	exit(0);
}


void termination_handler (int signum)
/*--------------------------------------------
 * pulizia prima di uscire
 * libera la memoria condivisa
 * -----------------------------------------*/
{
	int i;
	int status;
	char writeBuffer[MSGLENGTH];
	int temp;
	

	if(getpid()==messagesPid)
	{
		formatMessage(writeBuffer,0,0,0,0,"SYSTEM STOPPING");
		write(logFileFd,writeBuffer,strlen(writeBuffer));
		close(logFileFd);
	}
	if(getpid()==mypid)		//sono il padre
	{
		my_printf("signal %d to %d\n",signum,getpid());
//		my_printf("ho ricevuto %d\n",signum);
		if(signum==SIGSEGV)
			my_printf("non bene, ho ricevuto sigsegv\n");

		killAllSystems();

		my_printf("freeing ai system nodes\n");
		free_system_nodes();

		msgctl(msg_alarm_id, IPC_RMID, 0);
		
		temp=NUMPANELS;
		NUMPANELS=0;
		for(i=0;i<temp;i++)
			free(panels[i]);
		if(panels)
			free(panels);
		panels=NULL;

		freeshm();

		my_printf("Fine\n");
		exit(0);
	}
	else
	{
		if(getpid()==serverPid)
		{
			for(i=0;i<MAXCONNECTIONS;i++)
				if(serverPids[i]>0)
					killPid(serverPids[i]);
//			exit(1);
		}
	}
}

void readSystemDefaults()
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	char query[255];
	MYSQL *conn;
	
	conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("%s\n",mysql_error(conn));
		return;
	}

	//READ SYSTEM DEFAULTS
	strcpy(query,"SELECT record_data_time,localita,nome,reboot_time,offset_effe FROM system_ini");
	state = mysql_query(conn, query);

	if( state != 0 )
	{
		printf("%s\n",mysql_error(conn));
		my_printf("%s - %s\n",query,mysql_error(conn));
		termination_handler(2);
	}

	result = mysql_store_result(conn);

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
	mysql_close(conn);
//END READ SYSTEM DEFAULTS
}

MYSQL *mysqlConnect()
{
//MYSQL STUFF
	MYSQL *conn,mysql;

	my_bool reconnect = 1;
	mysql_options(&mysql, MYSQL_OPT_RECONNECT, &reconnect);
//	conn=mysql_init(&mysql);
	conn=mysql_init(NULL);
	mysql_real_connect(conn,"localhost","root", "minair","socket", 0, NULL,0);
	return conn;
}


int main(int argc, char *argv[])
{
	struct sigaction sa;
	
	time_t db_counter;
	time_t db_counter_now;
	
	struct msgform msg;
	char message[160];
	bool notAllProcessesOn=0;
	int k;
	argv0size = strlen(argv[0]);


	if (signal (SIGUSR1, kill_handler) == SIG_IGN)
		signal (SIGUSR1, SIG_IGN);

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
		my_perror("sigaction");
		termination_handler(2);
	}

	msg_alarm_id=msgget(MSGKEY,0777|IPC_CREAT); /* ottiene un descrittore per la chiave KEY; coda nuova */
	msg.mtype=1;   //not sms

	mypid=getpid();

/*	if(getenv("HOME")!=NULL)
		sprintf(logPath,"%s/log/",getenv("HOME"));
	else
		strcpy(logPath,"./log/");*/
	
	my_printf("get_shared_memory_segment: reloadCommand\n");
	reloadCommand=(bool *)get_shared_memory_segment(1, &SHMRELOADCOMMAND, "/dev/zero");
	if(!reloadCommand)
			die("reloadCommand - get_shared_memory_segment\n");

//
	*reloadCommand=0;		//così all'inizio parte server
	while(1)
	{
		loadAllSystems();
		startAllSystems(argv);
		notAllProcessesOn=0;

		*reloadCommand=0;
		db_counter=time(NULL);
		do
		{
			// fa qualcosa qui
			*buonGiornoAttivo=checkBuonGiornoAttivo();
			*buonaNotteAttivo=checkBuonaNotteAttivo();
	
			// ora attendo solo la morte dei figli
			usleep(1000);
	

			//salvo valori storici ad intervalli regolari
/*			db_counter_now=time(NULL);
			if((db_counter_now - db_counter >= RECORDDATATIME)
					&& ((db_counter_now % RECORDDATATIME) <50))
			{
				storeTables();
				db_counter=time(NULL);
			}
*/

			if(checkSystems(pid)!=TOTALSYSTEMS)
			{
				my_printf("Respawning dead process\n");
				for(k=0;k<TOTALSYSTEMS;k++)
					if(pid[k]==-1)
						forkSystem(k,argv);
/*				if(notAllProcessesOn==0)
				{
					my_printf("Not all children are alive\nYou had better restart\n");
					notAllProcessesOn=1;
				}*/
			}
		}
		while(*reloadCommand==0);
		my_printf("reload required\n");
		killAllSystems();
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
	for(k=0;k<TOTALSYSTEMS;k++)
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
	
	for(k=0;k<TOTALSYSTEMS;k++)
	{
		killPid(pidArray[k]);

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
			else 	
			{
				if(k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS)//knx
				{
					l=k-NUMSYSTEMS-NUMMULTIMETERS;
					if(knxGateways[l].sockfd!=-1)
						close(knxGateways[l].sockfd);
				}
				else
				{
					if(k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS+NUMCEIABIGATEWAYS)//bacnet
					{
						l=k-NUMSYSTEMS-NUMMULTIMETERS-NUMKNXGATEWAYS;
						if(bacnetDevices[l].sockfd!=-1)
							close(bacnetDevices[l].sockfd);
					}
					else //ceiabi
					{
						l=k-NUMSYSTEMS-NUMMULTIMETERS-NUMKNXGATEWAYS+NUMBACNETDEVICES;
						if(CEIABIGateways[l].sockfd!=-1)
							close(CEIABIGateways[l].sockfd);
					}
				}
			}
		}



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
	MYSQL *conn=mysqlConnect();
	if(!conn)
		return;

	query=(char *)malloc(strlen(msg)+255);
	sprintf(query,"INSERT INTO events(data,event_type,device_num,ch_num,ch_id,msg) "
					"VALUES (NOW(),'%d','%d','%d','%d','%s')",event_type,device_num,ch_num,ch_id,msg);
	if(mysql_query(conn,query)!=0)
		my_printf("%s\n%s\n",query,mysql_error(conn));
	free(query);
	mysql_close(conn);
}

int storeAlarm(int adId,int systemId,int channelId,char *msg)
{
	char query[255];
	int out=0;
	MYSQL *conn=mysqlConnect();

	sprintf(query,"call insertAlarm(%d,%d,%d,'%s')",adId,systemId,channelId,msg);
	if(mysql_query(conn,query)!=0)
		my_printf("%s\n%s\n",query,mysql_error(conn));

	out=mysql_affected_rows(conn);
	mysql_close(conn);
	return out;
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
		my_perror("send");
	return;
}

void die(char *text)
{
	my_perror(text);
	termination_handler(2);
}

void killPid(int pid)
{
	int status;
	if(pid!=-1)
	{
		if(kill(pid,0)==0)
		{
			kill(pid,SIGUSR1);
			waitpid(pid,&status,0);	
		}
	}
}

void killAllSystems()
{
	my_printf("killing sms\n");
	killPid(messagesPid);
	my_printf("killing controller\n");
	killPid(controllerPid);
	my_printf("killing systems\n");
	killSystems(pid);
	my_printf("killing server\n");
	killPid(serverPid);
}

void freeshm()
{
	if(*reloadCommand==0)
	{
		if(reloadCommand&&(shmdt(reloadCommand)==-1))
			my_perror("reloadCommand");
		reloadCommand=0;
		shmctl(SHMRELOADCOMMAND, IPC_RMID, &shmid_struct);
	}

	if(sms_devices&&(shmdt(sms_devices)==-1))
		my_perror("sms_devices");
	sms_devices=0;
	shmctl(SHMSMSDEVICESID, IPC_RMID, &shmid_struct);

	if(sms_numbers&&(shmdt(sms_numbers)==-1))
		my_perror("sms_numbers");
	sms_numbers=0;
	shmctl(SHMSMSNUMBERSID, IPC_RMID, &shmid_struct);

	if(systems&&(shmdt(systems)==-1))
		my_perror("shmdt systems");
	systems=0;
	shmctl(SHMSYSTEMSID, IPC_RMID, &shmid_struct);

	if(multimeters&&(shmdt(multimeters)==-1))
		my_perror("shmdt multimeters");
	multimeters=0;
	shmctl(SHMMULTIMETERSID, IPC_RMID, &shmid_struct);

	if(analogTable&&(shmdt(analogTable)==-1))
		my_perror("shmdt analogtable");
	analogTable=0;
	shmctl(SHMANALOGID, IPC_RMID, &shmid_struct);

	if(knxGateways&&(shmdt(knxGateways)==-1))
		my_perror("shmdt knxgateways");
	knxGateways=0;
	shmctl(SHMKNXGATEWAYSID, IPC_RMID, &shmid_struct);

	if(knxTable&&(shmdt(knxTable)==-1))
		my_perror("shmdt knxtable");
	knxTable=0;
	shmctl(SHMKNXTABLEID, IPC_RMID, &shmid_struct);

	if(readingTable&&(shmdt(readingTable)==-1))
		my_perror("shmdt readingtable");
	readingTable=0;
	shmctl(SHMREADINGID, IPC_RMID, &shmid_struct);

	if(digitalTable&&(shmdt(digitalTable)==-1))
		my_perror("shmdt digitaltable");
	digitalTable=0;
	shmctl(SHMDIGITALID, IPC_RMID, &shmid_struct);

	if(digitalOutTable&&(shmdt(digitalOutTable)==-1))
		my_perror("shmdt digitalouttable");
	digitalOutTable=0;
	shmctl(SHMDIGITALOUTID, IPC_RMID, &shmid_struct);

	if(scenariPresenzeTable&&(shmdt(scenariPresenzeTable)==-1))
		my_perror("shmdt scenaipresenzetable");
	scenariPresenzeTable=0;
	shmctl(SHMSCENARIPRESENZEID, IPC_RMID, &shmid_struct);

	if(scenariPresenzeAttivo&&(shmdt(scenariPresenzeAttivo)==-1))
		my_perror("shmdt scenariPresenzeAttivo");
	scenariPresenzeAttivo=0;
	shmctl(SHMSCENARIPRESENZEATTIVOID, IPC_RMID, &shmid_struct);

	if(scenariBgBnTable&&(shmdt(scenariBgBnTable)==-1))
		my_perror("shmdt scenarigbgntable");
	scenariBgBnTable=0;
	shmctl(SHMSCENARIBGBNID, IPC_RMID, &shmid_struct);

	if(irrigazioneCircuitiTable&&(shmdt(irrigazioneCircuitiTable)==-1))
		my_perror("shmdt irrigazionecircuititable");
	irrigazioneCircuitiTable=0;
	shmctl(SHMIRRIGAZIONECIRCUITIID, IPC_RMID, &shmid_struct);

	if(irrigazioneTable&&(shmdt(irrigazioneTable)==-1))
		my_perror("shmdt irrigazionetable");
	irrigazioneTable=0;
	shmctl(SHMIRRIGAZIONEID, IPC_RMID, &shmid_struct);

	if(id_digital_pioggia&&(shmdt(id_digital_pioggia)==-1))
		my_perror("shmdt id_digital_pioggia");
	id_digital_pioggia=0;
	shmctl(SHMPIOGGIA, IPC_RMID, &shmid_struct);

	if(buonGiornoAttivo&&(shmdt(buonGiornoAttivo)==-1))
		my_perror("shmdt buonGiornoAttivo");
	buonGiornoAttivo=0;
	shmctl(SHMSCENARIBGATTIVOID, IPC_RMID, &shmid_struct);

	if(buonaNotteAttivo&&(shmdt(buonaNotteAttivo)==-1))
		my_perror("shmdt buonaNotteAttivo");
	buonaNotteAttivo=0;
	shmctl(SHMSCENARIBNATTIVOID, IPC_RMID, &shmid_struct);

	if(ai_sistemi&&(shmdt(ai_sistemi)==-1))
		my_perror("shmdt ai_sistemi");
	ai_sistemi=0;
	shmctl(SHMAISYSTEMS, IPC_RMID, &shmid_struct);

	if(bacnetTable&&(shmdt(bacnetTable)==-1))
		my_perror("shmdt bacnetTable");
	bacnetTable=0;
	shmctl(SHMBACNETTABLEID, IPC_RMID, &shmid_struct);

	if(bacnetDevices&&(shmdt(bacnetDevices)==-1))
		my_perror("shmdt bacnetDevices");
	bacnetDevices=0;
	shmctl(SHMBACNETDEVICESID, IPC_RMID, &shmid_struct);
}

void loadAllSystems()
{
	int i=0,j=0,k,l;
	struct timeval tim;
	double t1;
	int knx_ch_in;
	int knx_ch_out;
	MYSQL *conn;

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
	
	NUMBACNETDEVICES=0;
	NUMBACNETLINES=0;
	NUMCEIABIGATEWAYS=0;

	gettimeofday(&tim, NULL);
	t1=tim.tv_sec+(tim.tv_usec/1000000.0);
//MYSQL STUFF
	conn=mysqlConnect();
	if( conn == NULL ) 
	{
		my_printf("error connecting: %s\n",mysql_error(conn));
		termination_handler(2);
	}
//READ SYSTEM DEFAULTS (needs mysql);
	my_printf("reading system defaults\n");
	readSystemDefaults();

//READ EFFEMERIDI (needs mysql);
	my_printf("reading effemeridi\n");
	readEffemeridi();
	mysql_close(conn);

	if(loadSMS(0))
		termination_handler(2);

	if(loadSystemTable(0))
		termination_handler(2);

	if(loadMultimeterTable(0))
		termination_handler(2);

	if(loadKnx(0))
		termination_handler(2);

	if(loadBacnet(0))
		termination_handler(2);

	if(loadCEIABI(0))
		termination_handler(2);


/*	for(i=0;i<NUMMULTIMETERS;i++)
	{
		my_printf("%d %s\n",multimeters[i].multimeter_num,multimeters[i].address);
		for(j=0;j<multimeters[i].in_bytes_1_length;j++)
			my_printf("%x ",multimeters[i].in_bytes_1[j]);
		my_printf("\n");
		for(j=0;j<multimeters[i].in_bytes_2_length;j++)
			my_printf("%x ",multimeters[i].in_bytes_2[j]);
		my_printf("\n");
	}
*/
	TOTALSYSTEMS=NUMSYSTEMS+NUMMULTIMETERS+
				NUMKNXGATEWAYS+NUMBACNETDEVICES+NUMCEIABIGATEWAYS;

	pid=(int *)malloc((TOTALSYSTEMS) * sizeof(int));
	deviceToid=(int ***)malloc(6*sizeof(int **));
	deviceToid[ANALOG]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //analog
	deviceToid[DIGITAL]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //digital
	deviceToid[DIGITAL_OUT]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //digital_out
	deviceToid[MULTIMETER]=(int **)malloc(NUMMULTIMETERS * sizeof(int *)); //mm
	deviceToid[KNX_IN]=(int **)malloc(NUMKNXGATEWAYS * sizeof(int *)); //knx_in
	deviceToid[KNX_OUT]=(int **)malloc(NUMKNXGATEWAYS * sizeof(int *)); //knx_out
	deviceToid[BACNET]=(int **)malloc(NUMBACNETDEVICES * sizeof(int *)); //bacnet
	deviceToid[CEIABI]=(int **)malloc(NUMCEIABIGATEWAYS * sizeof(int *)); //ceiabi

	for(i=0;i<NUMSYSTEMS;i++)
	{
		deviceToid[ANALOG][i]=(int *)malloc(systems[i].in_ch_an*sizeof(int));
		for(k=0;k<systems[i].in_ch_an;k++)
			deviceToid[ANALOG][i][k]=-1;
		deviceToid[DIGITAL][i]=(int *)malloc(systems[i].in_ch_d*sizeof(int));
		for(k=0;k<systems[i].in_ch_d;k++)
			deviceToid[DIGITAL][i][k]=-1;
		deviceToid[DIGITAL_OUT][i]=(int *)malloc(systems[i].out_ch_d*sizeof(int));
		for(k=0;k<systems[i].out_ch_d;k++)
			deviceToid[DIGITAL_OUT][i][k]=-1;
	}
	for(i=0;i<NUMMULTIMETERS;i++)
	{
		deviceToid[MULTIMETER][i]=(int *)malloc((multimeters[i].out_ch_1+multimeters[i].out_ch_2)*sizeof(int));
		for(k=0;k<multimeters[i].out_ch_1+multimeters[i].out_ch_2;k++)
			deviceToid[MULTIMETER][i][k]=-1;
	}
	for(i=0;i<NUMKNXGATEWAYS;i++)
	{
		deviceToid[KNX_IN][i]=(int *)malloc((knxGateways[i].ch_in)*sizeof(int));
		for(k=0;k<knxGateways[i].ch_in;k++)
			deviceToid[KNX_IN][i][k]=-1;
		deviceToid[KNX_OUT][i]=(int *)malloc((knxGateways[i].ch_out)*sizeof(int));
		for(k=0;k<knxGateways[i].ch_out;k++)
			deviceToid[KNX_OUT][i][k]=-1;
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
				deviceToid[KNX_IN][l][knx_ch_in]=k;
				knx_ch_in++;
			}
			else
			{
				deviceToid[KNX_OUT][l][knx_ch_out]=k;
				knx_ch_out++;
			}
		}
	}

	for(i=0;i<NUMBACNETDEVICES;i++)
	{
		deviceToid[BACNET][i]=(int *)malloc(bacnetDevices[i].in_ch*sizeof(int));
		for(k=0;k<bacnetDevices[i].in_ch;k++)
			deviceToid[BACNET][i][k]=-1;
	}
	for(i=0;i<NUMCEIABIGATEWAYS;i++)
	{
		deviceToid[CEIABI][i]=(int *)malloc(CEIABIGateways[i].in_ch*sizeof(int));
		for(k=0;k<CEIABIGateways[i].in_ch;k++)
			deviceToid[CEIABI][i][k]=-1;
	}

/*
	my_printf("ANALOGCHANNELS - DIGITALCHANNELS - DIGITALOUTCHANNELS - READINGCHANNELS\n");
	my_printf("%d - %d - %d - %d\n",ANALOGCHANNELS,DIGITALCHANNELS,DIGITALOUTCHANNELS,READINGCHANNELS);
	*/
	if(loadDigitalOutTable(0))
		termination_handler(2);
	if(loadScenariPresenzeTable(0))
		termination_handler(2);
	if(loadScenariBgBnTable(0))
		termination_handler(2);
	if(loadDigitalTable(0))
		termination_handler(2);
	if(loadAnalogTable(0))
		termination_handler(2);
	if(loadReadingTable(0))
		termination_handler(2);
	if(loadIrrigazioneTables(0,0))
		termination_handler(2);
	if(loadIntrusione())
		termination_handler(2);

/*
	my_printf("--- PANELS ---\n");
	loadPanels();
	for(i=0;i<NUMPANELS;i++)
		for(j=0;j<16;j++)
			my_printf("%d %d %d\n",i,j,panels[i][j]);

	my_printf("--- ANALOG ---\n");
	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].in_ch_an;j++)
			if(analogTable[deviceToid[0][i][j]].id_analog!=-1)
				my_printf("%d %d %d %s %d\n",analogTable[deviceToid[0][i][j]].id_analog,
					analogTable[deviceToid[0][i][j]].device_num,
					analogTable[deviceToid[0][i][j]].ch_num,
					analogTable[deviceToid[0][i][j]].description,
					analogTable[deviceToid[0][i][j]].enabled);

	my_printf("--- DIGITAL ---\n");
	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].in_ch_d;j++)
			if(digitalTable[deviceToid[1][i][j]].id_digital!=-1)
				my_printf("%d %d %s %d\n",digitalTable[deviceToid[1][i][j]].device_num,
					digitalTable[deviceToid[1][i][j]].ch_num,
					digitalTable[deviceToid[1][i][j]].description,
					digitalTable[deviceToid[1][i][j]].enabled);


	my_printf("--- DIGITALOUT ---\n");
	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].out_ch_d;j++)
			if(digitalOutTable[deviceToid[2][i][j]].id_digital_out!=-1)
				my_printf("%d %d %d %s %d %d - %d %d\n",
					digitalOutTable[deviceToid[2][i][j]].id_digital_out,
					digitalOutTable[deviceToid[2][i][j]].device_num,
					digitalOutTable[deviceToid[2][i][j]].ch_num,
					digitalOutTable[deviceToid[2][i][j]].description,
					digitalOutTable[deviceToid[2][i][j]].def,
					digitalOutTable[deviceToid[2][i][j]].value,
					digitalOutTable[deviceToid[2][i][j]].po_delay,
					digitalOutTable[deviceToid[2][i][j]].on_time);

	my_printf("--- MM ---\n");
	for(i=0;i<NUMMULTIMETERS;i++)
		for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
			if(readingTable[deviceToid[3][i][j]].id_reading!=-1)
				my_printf("%d %d %d %s %d\n",readingTable[deviceToid[3][i][j]].id_reading,
					readingTable[deviceToid[3][i][j]].multimeter_num,
					readingTable[deviceToid[3][i][j]].ch_num,
					readingTable[deviceToid[3][i][j]].description,
					readingTable[deviceToid[3][i][j]].enabled);

	my_printf("--- KNX IN ---\n");
	for(i=0;i<NUMKNXGATEWAYS;i++)
		for(j=0;j<knxGateways[i].ch_in;j++)
			if(knxTable[deviceToid[4][i][j]].id_knx_line!=-1)
				my_printf("%d %d %s %c %s %d\n",knxTable[deviceToid[4][i][j]].id_knx_line,
					knxTable[deviceToid[4][i][j]].id_knx_gateway,
					knxTable[deviceToid[4][i][j]].group_address,
					knxTable[deviceToid[4][i][j]].data_type,
					knxTable[deviceToid[4][i][j]].description,
					knxTable[deviceToid[4][i][j]].enabled);

	my_printf("--- KNX ALL ---\n");
	for(i=0;i<NUMKNXGATEWAYS;i++)
		for(j=0;j<knxGateways[i].ch_in+knxGateways[i].ch_out;j++)
				my_printf("%d %d %s %c %s %d\n",knxTable[j].id_knx_line,
					knxTable[j].id_knx_gateway,
					knxTable[j].group_address,
					knxTable[j].data_type,
					knxTable[j].description,
					knxTable[j].enabled);

	my_printf("--- KNX OUT ---\n");
	for(i=0;i<NUMKNXGATEWAYS;i++)
		for(j=0;j<knxGateways[i].ch_out;j++)
			if(knxTable[deviceToid[5][i][j]].id_knx_line!=-1)
				my_printf("%d %d %s %c %s\n",knxTable[deviceToid[5][i][j]].id_knx_line,
					knxTable[deviceToid[5][i][j]].id_knx_gateway,
					knxTable[deviceToid[5][i][j]].group_address,
					knxTable[deviceToid[5][i][j]].data_type,
					knxTable[deviceToid[5][i][j]].description);

	my_printf("--- SCENARIPRESENZE ---\n");
	for(i=0;i<SCENARIPRESENZECOUNT;i++)
		my_printf("%d %d %d %d %d %d %d\n",scenariPresenzeTable[i].id_digital_out,
					scenariPresenzeTable[i].attivo,
					scenariPresenzeTable[i].ciclico,
					scenariPresenzeTable[i].tempo_on,
					scenariPresenzeTable[i].tempo_off,
					scenariPresenzeTable[i].ora_ini,
					scenariPresenzeTable[i].ora_fine);

	my_printf("--- SCENARIBGBN ---\n");
	for(i=0;i<SCENARIBGBNCOUNT;i++)
		my_printf("%d %d %d %d\n",scenariBgBnTable[i].id_digital_out,
					scenariBgBnTable[i].attivo,
					scenariBgBnTable[i].ritardo,
					scenariBgBnTable[i].bg);

	my_printf("--- IRRIGAZIONE ---\n");
	for(i=0;i<IRRIGAZIONESYSTEMS;i++)
		my_printf("%d %d %d %d %d %d %d\n",
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
			my_printf("%d %d %d %d %d\n",
				irrigazioneCircuitiTable[i].id_irrigazione,
				irrigazioneCircuitiTable[i].circuito,
				irrigazioneCircuitiTable[i].id_digital_out,
				irrigazioneCircuitiTable[i].durata,
				irrigazioneCircuitiTable[i].validita);
	}

	my_printf("--- SMS ---\n");
	for(i=0;i<NUMSMSDEVICES;i++)
		my_printf("%d %s %d %s\n",
			sms_devices[i].id_sms_device,
			sms_devices[i].address,
			sms_devices[i].port,
			sms_devices[i].description);
	for(i=0;i<NUMSMSNUMBERS;i++)
		my_printf("%d %s %s\n",
			sms_numbers[i].id_sms_number,
			sms_numbers[i].name,
			sms_numbers[i].number);

	my_printf("--- BACNET ---\n");
	for(i=0;i<NUMBACNETDEVICES;i++)
		my_printf("%d %s %d %s %d %d %d %d\n",
			bacnetDevices[i].id,
			bacnetDevices[i].address,
			bacnetDevices[i].port,
			bacnetDevices[i].description,
			bacnetDevices[i].enabled,
			bacnetDevices[i].status,
			bacnetDevices[i].sockfd,
			bacnetDevices[i].failures);

	my_printf("--- BACNET_INPUT ---\n");
	for(i=0;i<NUMBACNETLINES;i++)
		my_printf("%d %d %d %d %s %s %s %d %s %d\n",
			bacnetTable[i].id,
			bacnetTable[i].id_bacnet_device,
			bacnetTable[i].object_type,
			bacnetTable[i].object_instance,
			bacnetTable[i].description,
			bacnetTable[i].hi_low_msg,
			bacnetTable[i].low_hi_msg,
			bacnetTable[i].is_alarm,
			bacnetTable[i].alarm_msg,
			bacnetTable[i].value);
*/


	my_printf("pid: %d\n",getpid());
	gettimeofday(&tim, NULL);
	my_printf("%.6f seconds to initialise\n",(tim.tv_sec+(tim.tv_usec/1000000.0))-t1);
//FINE MYSQL STUFF
}


void startAllSystems(char **argv)
{
	int k;
	int status;
	char writeBuffer[MSGLENGTH];

	//fork to act as sms sender
	if((messagesPid=fork())==0)
	{
		strncpy(argv[0],"sms",argv0size);
		messagesPid=getpid();
		doMessageHandler();
		wait(&status);
		exit(0);
	}
	my_printf("sms sender pid %d\n",messagesPid);


	handleMessage(0,0,0,0,"SYSTEM STARTING",0,ALARM_NO_STORE);

	for(k=0;k<TOTALSYSTEMS;k++)
		forkSystem(k,argv);

	//fork to act as controller
	if((controllerPid=fork())==0)
	{
		strncpy(argv[0],"controller",argv0size);
		controllerPid=getpid();
		doController();
		wait(&status);
		exit(0);
	}	
	my_printf("controller pid %d\n",controllerPid);
	

	//fork to act as a server
	if((serverPid=fork())==0)
	{
		strncpy(argv[0],"server",argv0size);
		serverPid=getpid();
		doServer();
		wait(&status);
		exit(0);
	}
	my_printf("server pid %d\n",serverPid);

}


void forkSystem(int k,char **argv)
{
	int l;
	pid[k]=-1;
	char processName[255];
	
	switch(pid[k]=fork()) // fork for every system or multimeter or knx or bacnet
	{
		case -1:
			my_perror("fork!");
			termination_handler(2);
			break;
		case 0:
			pid[k]=getpid();
			if(k<NUMSYSTEMS)	// tabella system
			{
				sprintf(processName,"sys_%d",k);
				strncpy(argv[0],processName,argv0size);
				doSystem(k);
			}
			else 
			{
				if(k<NUMSYSTEMS+NUMMULTIMETERS)//multimetro
				{
					l=k-NUMSYSTEMS;
					sprintf(processName,"mm_%d",l);
					strncpy(argv[0],processName,argv0size);
					doMultimeter(l);
				}
				else
				{
					if(k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS)//knx
					{
						l=k-NUMSYSTEMS-NUMMULTIMETERS;
						sprintf(processName,"knx_%d",l);
						strncpy(argv[0],processName,argv0size);
						doKNX(l);
					}
					else
					{
						if(k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS+NUMBACNETDEVICES)//bacnet
						{
							l=k-NUMSYSTEMS-NUMMULTIMETERS-NUMKNXGATEWAYS;
							sprintf(processName,"bac_%d",l);
							strncpy(argv[0],processName,argv0size);
							doBacnet(l);
						}
						else//ceiabi
						{
							l=k-NUMSYSTEMS-NUMMULTIMETERS-NUMKNXGATEWAYS-NUMBACNETDEVICES;
							sprintf(processName,"cei_%d",l);
							strncpy(argv[0],processName,argv0size);
							doCEIABI(l);
						}
					}
				}
			}
			exit(0);
			break;
		default:
//				my_printf("padre :%d\n",getpid());

			if(k<NUMSYSTEMS)	// tabella system
				my_printf("system pid %d\n",pid[k]);
			else 
			{
				if(k<NUMSYSTEMS+NUMMULTIMETERS)//multimetro
					my_printf("multimeter pid %d\n",pid[k]);
				else
				{
					if(k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS)//knx
						my_printf("knx pid %d\n",pid[k]);
					else
					{
						if(k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS+NUMBACNETDEVICES)//bacnet
							my_printf("bacnet pid %d\n",pid[k]);
						else//ceiabi
							my_printf("ceiabi pid %d\n",pid[k]);						
					}
				}
			}
			break;
	}
}

void valueInRange(int adId,int systemId,int channelId)
/*--------------------------------------------
 * ritorno da allarme
 * int adId  0=analogico, 1=digitale, 2=digital_out, 3=mm ecc
 * int systemId		device number
 * int channelId	channel number
  * -----------------------------------------*/
{
	char query[255];
	MYSQL *conn=mysqlConnect();
	if(!conn)
		return;

	sprintf(query,"UPDATE alarms SET inattivo=1 "
		"WHERE board_type='%d' AND device_id='%d' AND ch_id='%d'"
		,adId,systemId,channelId);

	if(mysql_query(conn,query)!=0)
		my_printf("%s\n%s\n",query,mysql_error(conn));
	mysql_close(conn);
}

