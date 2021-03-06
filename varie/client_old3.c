#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <mysql/mysql.h>

#define BACKLOG 10 
#define MYPORT 3480
#define MAXDATASIZE 5120
#define MAXCONNECTIONS 20
#define TERMINATOR "FiNe"
#define MSGKEY 75
#define MSGLENGTH 100

struct shmid_ds shmid_struct;	

typedef unsigned char bool;

struct analogLine
{
	int id_analog;
	char form_label[20];
	char description[50];
	char label[15];
	char sinottico[20];
	int device_num;
	int ch_num;
	int scale_zero;
	int scale_full;
	double range_zero;
	double range_full;
	double range_pend;
	bool bipolar;
	bool al_high_active;
	double al_high;
	bool al_low_active;
	double al_low;
	double offset;
	char unit[5];
	int time_delay_high;
	int time_delay_low;
	int time_delay_high_off;
	int time_delay_low_off;

	char curve[255];
	bool no_linear;
	bool printer;
	char msg_l[80];
	char msg_h[80];
	bool enabled;

	bool currently_high;
	bool currently_low;

	int value;
	int value1;
	int value2;
	int value3;
	int value4;

	double value_eng;
	double value_eng1;
	double value_eng2;
	double value_eng3;
	double value_eng4;
	
	time_t time_delay_high_cur;
	time_t time_delay_low_cur;
	time_t time_delay_high_off_cur;
	time_t time_delay_low_off_cur;
};
struct analogLine *analogTable;

struct digitalLine
{
	int id_digital;
	char form_label[20];
	char description[50];
	char label[15];
	char sinottico[20];
	int device_num;
	int ch_num;
	bool printer;
	int alarm_value;
	int time_delay_on;
	int time_delay_off;
	int currently_on;
	char msg[80];
	bool enabled;

	int value;
	int value1;
	int value2;
	int value3;
	int value4;
	time_t time_delay_on_cur;
	time_t time_delay_off_cur;
};
struct digitalLine *digitalTable;


struct digitalOutLine
{
	int id_digital_out;
	char description[50];
	int device_num;
	int ch_num;
	int def;
	int potime;
	int value;
	time_t start_time;
	time_t act_time;
};
struct digitalOutLine *digitalOutTable;

struct system
{
	int device_num;
	char address[14];
	int port;
	unsigned char type;
	unsigned char in_num_bytes;
	unsigned char in_ch_an;
	unsigned char in_ch_d;
	unsigned char in_eos;
	unsigned char out_num_bytes;
	unsigned char out_ch_d;
	unsigned char out_ch_per_byte;
	unsigned char out_sos;
	unsigned char out_eos;
	bool enabled;
	int sockfd;
};
struct system *systems;

struct msgform 
{
	long mtype;
	char mtext[MSGLENGTH];   
} msg;

struct effemeridi
{
	int dawn;
	int sunset;
} effemeridiTable[12][31];

int msgid;

char logPath[255];
char logFileName[80];
int logFileFd=-1;

int NUMSYSTEMS;
int ***deviceToid; //type,num,ch
int ANALOGCHANNELS=0;
int DIGITALCHANNELS=0;
int DIGITALOUTCHANNELS=0;
int MAXATTEMPTS=5;
int OFFSET_EFFE=0;
char LOCALITA[30];
char NOME[30];
int RECORDDATATIME=300;

int SHMSYSTEMSID;
int SHMANALOGID;
int SHMDIGITALID;
int SHMDIGITALOUTID;

int serverPids[MAXCONNECTIONS];		//connessioni esterne (web,telnet)
int *pid;	//connessioni alle schede
int serverPid;	//pid del server (padre di serverPids)
int controllerPid; //pid del controller

MYSQL *connection=NULL, mysql;

int mypid;
int doConnect(int systemId);
int checkSons(int *pidArray);
int operate(int sockfd,int deviceId, int timeout);
void killSons(int *pidArray);
void *get_shared_memory_segment(const int , int *shmid, const char *); 
void doServer();
void doController();
void performAction(int nwfd,char *buf);
int systemNumToId(int systemNum,int totale);
void initializeAnalogTable();
void initializeDigitalTable();
void initializeDigitalOutTable();
void systemDisconnected(int systemId);
double getCurveValue(char* values,double value);
void safecpy(char *dest,char *src);
void sendResetSignal(int systemId);
int getLogFileName(char *dest,char *path);
int getTimeString(char *format,char *dest);
void formatMessage(char *buffer,int adId,int id,int systemId,int channelId,char *msg);
void storeMessage(int adId,int id,int systemId,int channelId,char *msg);
void storeTables();
void updateSystem(int device_num);
void updateAnalogChannel(int id_analog);
void updateDigitalChannel(int id_digital);
void updateDigitalOutChannel(int id_digital);
int id_analogToId(int id_analog);
int id_digitalToId(int id_digital);
int id_digitalOutToId(int id);
void loadSystemTable(bool onlyupdate);
void loadAnalogTable(bool onlyupdate);
void loadDigitalTable(bool onlyupdate);
void loadDigitalOutTable(bool onlyupdate);
int setOutput(int id,int value);
void setOutputDefault(int systemId);
void resetAnalogValues(int i);
void resetDigitalOutValues(int i);
void resetDigitalValues(int i);
int h2i(char *h);
void i2h(char *h,int i);
void scriviEffemeridiSuFile(int fd);

void sigchld_handler(int s)
/*--------------------------------------------
 * aggiorna array dei pid alla morte dei figli
 * -----------------------------------------*/
{
	int i;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	if(getppid()==mypid)  //I'm a child, not a grandchild
	{
//		printf("child dying\n");
		for(i=0;i<MAXCONNECTIONS;i++)
			if((serverPids[i]!=-1)&&(kill(serverPids[i],0)==-1))
				serverPids[i]=-1;
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
	
	if(getppid()==mypid)
	{
		for(i=0;i<MAXCONNECTIONS;i++)
			if(serverPids[i]!=-1)
				kill(serverPids[i],SIGTERM);
	}
	else
	if(getpid()==mypid)		//sono il padre
	{
		if(signum==SIGSEGV)
			printf("non bene, ho ricevuto sigsegv\n");
		killSons(pid);
		kill(serverPid,SIGKILL);
		waitpid(serverPid,&status,0);	
		kill(controllerPid,SIGKILL);
		waitpid(controllerPid,&status,0);	

		formatMessage(writeBuffer,0,0,0,0,"SYSTEM STOPPING");
		write(logFileFd,writeBuffer,strlen(writeBuffer));
		close(logFileFd);

		msgctl(msgid, IPC_RMID, 0);
		
		mysql_close(connection);
	}


	if(shmdt(systems))
		perror("shmdt");
	shmctl(SHMSYSTEMSID, IPC_RMID, &shmid_struct);
	if(shmdt(analogTable))
		perror("shmdt");
	shmctl(SHMANALOGID, IPC_RMID, &shmid_struct);
	if(shmdt(digitalTable))
		perror("shmdt");
	shmctl(SHMDIGITALID, IPC_RMID, &shmid_struct);
	if(shmdt(digitalOutTable))
		perror("shmdt");
	shmctl(SHMDIGITALOUTID, IPC_RMID, &shmid_struct);

	exit(1);
}


int main(int argc, char *argv[])
{
	int sockfd;
	unsigned char buf[MAXDATASIZE];
	int pause=0;
	int i=0,j=0,k;
	int port;
	struct timeval tim;
	double t1;
	int received;
	int status;
	unsigned char c;
	int out;
	unsigned char *sendBuf;
	int idRow;
	char tempLogFileName[80];
	int vai;
	struct sigaction sa;
	char writeBuffer[MSGLENGTH];
	struct tm now;
	time_t timer;
	struct stat statbuf;
	
	time_t db_counter;
	time_t db_counter_now;
	
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;

	if (signal (SIGINT, termination_handler) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	if (signal (SIGSEGV, termination_handler) == SIG_IGN)
		signal (SIGSEGV, SIG_IGN);
	if (signal (SIGHUP, termination_handler) == SIG_IGN)
		signal (SIGHUP, SIG_IGN);
	if (signal (SIGTERM, termination_handler) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);

	mypid=getpid();

	if(getenv("HOME")!=NULL)
		sprintf(logPath,"%s/log/",getenv("HOME"));
	else
		strcpy(logPath,"./log/");
	
	gettimeofday(&tim, NULL);
	t1=tim.tv_sec+(tim.tv_usec/1000000.0);
//MYSQL STUFF
	mysql_init(&mysql);
	connection = mysql_real_connect(&mysql,"localhost","root", "minair","socket", 0, NULL,0);
	if( connection == NULL ) 
	{
		printf(mysql_error(&mysql));
		exit(1);
	}

//READ SYSTEM DEFAULTS
	state = mysql_query(connection, "SELECT record_data_time,localita,nome,reboot_time,offset_effe FROM system_ini");

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		exit(1);
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
	loadSystemTable(0);
//END READ SYSTEM DEFAULTS

//READ EFFEMERIDI

	for(i=0;i<12;i++)
		for(j=0;j<31;j++)
		{
			effemeridiTable[i][j].dawn=-1;
			effemeridiTable[i][j].sunset=-1;
		}

	state = mysql_query(connection, "SELECT month,day,dawn,sunset FROM effemeridi ORDER BY month,day");
	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		exit(1);
	}
		
	result = mysql_store_result(connection);
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		effemeridiTable[atoi(row[0])-1][atoi(row[1])-1].dawn=h2i(row[2])+OFFSET_EFFE;
		effemeridiTable[atoi(row[0])-1][atoi(row[1])-1].sunset=h2i(row[3])+OFFSET_EFFE;
	}
	mysql_free_result(result);
// END READ EFFEMERIDI

	
	pid=(int *)malloc(NUMSYSTEMS * sizeof(int));
	deviceToid=(int ***)malloc(3*sizeof(int **));
	deviceToid[0]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //analog
	deviceToid[1]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //digital
	deviceToid[2]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //digital_out
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
		
	loadDigitalOutTable(0);
	loadDigitalTable(0);
	loadAnalogTable(0);

	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].in_ch_an;j++)
			if(analogTable[deviceToid[0][i][j]].id_analog!=-1)
				printf("%d %d %s %d\n",analogTable[deviceToid[0][i][j]].device_num,
					analogTable[deviceToid[0][i][j]].ch_num,
					analogTable[deviceToid[0][i][j]].description,
					analogTable[deviceToid[0][i][j]].enabled);

	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].in_ch_d;j++)
			if(digitalTable[deviceToid[1][i][j]].id_digital!=-1)
				printf("%d %d %s %d\n",digitalTable[deviceToid[1][i][j]].device_num,
					digitalTable[deviceToid[1][i][j]].ch_num,
					digitalTable[deviceToid[1][i][j]].description,
					digitalTable[deviceToid[1][i][j]].enabled);

	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].out_ch_d;j++)
			if(digitalOutTable[deviceToid[2][i][j]].id_digital_out!=-1)
				printf("%d %d %s %d\n",digitalOutTable[deviceToid[2][i][j]].device_num,
					digitalOutTable[deviceToid[2][i][j]].ch_num,
					digitalOutTable[deviceToid[2][i][j]].description,
					digitalOutTable[deviceToid[2][i][j]].def);
					
	gettimeofday(&tim, NULL);
	printf("%.6f seconds to initialise\n",(tim.tv_sec+(tim.tv_usec/1000000.0))-t1);

//FINE MYSQL STUFF

	getLogFileName(logFileName,logPath);
	logFileFd=open(logFileName,O_WRONLY | O_CREAT | O_APPEND, 0644);
	
	fstat(logFileFd,&statbuf);
	if(statbuf.st_size==0)
		scriviEffemeridiSuFile(logFileFd);
	
	formatMessage(writeBuffer,0,0,0,0,"SYSTEM STARTING");
	write(logFileFd,writeBuffer,strlen(writeBuffer));


	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	msgid=msgget(MSGKEY,0777|IPC_CREAT); /* ottiene un descrittore per la chiave KEY; coda nuova */
	msg.mtype=1;   

	for(k=0;k<NUMSYSTEMS;k++)
	{
		pid[k]=-1;
		
		switch(pid[k]=fork())
		{
			case -1:
				perror("xe casini mulon!");
				exit(1);
				break;
			case 0:
				sockfd=doConnect(k);
				systems[k].sockfd=sockfd;
				printf("address %s, port %d, type %d connected\n",systems[k].address, systems[k].port, systems[k].type);

				i=0;
				do
				{
					out=operate(sockfd,systemNumToId(systems[k].device_num,NUMSYSTEMS),2);
					
					if(out==-1)
					{
						formatMessage(msg.mtext,0,0,systems[k].device_num,0,"DEVICE DISCONNECTED");
						msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);

						systemDisconnected(k);
						close(sockfd);
						systems[k].sockfd=-1;
						if(systems[k].enabled)
						{
							printf("error in %s %d type %d\nsleeping 5 seconds\n",systems[k].address,systems[k].port,systems[k].type);
							sleep(5);
							printf("trying to connect\n");
						}
						sockfd=doConnect(k);
						systems[k].sockfd=sockfd;
						printf("address %s port %d type %d connected\n",systems[k].address, systems[k].port, systems[k].type);
					}
					i++;
					usleep(1000000);	//intervallo interrogazione scheda
				}
				while(1);
				exit(0);
				break;
			default:
				break;
		}
	}
	//fork to act as controller
	if((controllerPid=fork())==0)
	{
		doController();
		wait(&status);
		exit(0);
	}	
	
	//fork to act as a server
	
	if((serverPid=fork())==0)
	{
		doServer();
		wait(&status);
		exit(0);
	}	
	
	db_counter=time(NULL);
	do
	{
		// fa qualcosa qui
		// ora attendo solo la morte dei figli
		usleep(10000);
		
		getLogFileName(tempLogFileName,logPath);
		if(strcmp(tempLogFileName,logFileName)!=0)
		{
			close(logFileFd);
			strcpy(logFileName,tempLogFileName);
			logFileFd=open(logFileName,O_WRONLY | O_CREAT | O_APPEND, 0644);
			scriviEffemeridiSuFile(logFileFd);
		}
		//leggo coda dei messaggi e scrivo su log
		vai=1;
		do
		{
			if(msgrcv(msgid,&msg,MSGLENGTH,1,IPC_NOWAIT)!=-1)
				write(logFileFd,msg.mtext,strlen(msg.mtext));
			else
				vai=0;
		}
		while(vai);
	
		db_counter_now=time(NULL);
		if((db_counter_now - db_counter >= RECORDDATATIME)
				&& ((db_counter_now % RECORDDATATIME) <50))
		{
			storeTables();
			db_counter=time(NULL);
		}
		
		
		if(checkSons(pid)!=NUMSYSTEMS)
			printf("Not all children are alive\nYou had better restart\n");
	}
	while(1);
}

int checkSons(int *pidArray)
/*--------------------------------------------
 * controlla lo stato dei figli ed eventualmente 
 * aggiorna l'array dei pid
 * int *pidArray - array con i pid dei figli
 * -----------------------------------------*/
{
	int status;
	int out=0;
	int k;
	for(k=0;k<NUMSYSTEMS;k++)
	{
		if(pidArray[k]!=-1)
			if(waitpid(pidArray[k],&status,WNOHANG)==pidArray[k])
				pidArray[k]=-1;
		out+=(pidArray[k]!=-1);
	}
	return out;
}
	

void killSons(int *pidArray)
/*--------------------------------------------
 * uccide i figli all'uscita per non lasciare zombie
 * int *pidArray - array con i pid dei figli
 * -----------------------------------------*/
{
	int k;
	int status;
	
	for(k=0;k<NUMSYSTEMS;k++)
	{
		if(pidArray[k]!=-1)
		{
			kill(pidArray[k],SIGKILL);
			if(waitpid(pidArray[k],&status,0)==pidArray[k])
				pidArray[k]=-1;
		}
	}
}

int doConnect(int systemId)
/*--------------------------------------------
 * gestisce la connessione/disconnessione dei device
 * riprova ogni 2 secondi
 * int systemId - identificatore device [0-NUMSYSTEMS-1]
 * -----------------------------------------*/
{
	struct hostent *he;
	struct sockaddr_in their_addr;
	int sockfd;
	int out;
	struct timeval tv;
	int attempt=0;
	
	do
	{
		out=0;
		if(systems[systemId].enabled==0)
			out=-1;
		else
		{
			if ((he=gethostbyname(systems[systemId].address)) == NULL)
				out=-1;
			else
			{
				if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
					out=-1;
				else
				{
					tv.tv_sec = 2;
					tv.tv_usec = 0;
					if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv))
						perror("setsockopt");

					if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,  sizeof tv))
						perror("setsockopt");

					their_addr.sin_family = AF_INET;
					their_addr.sin_port = htons(systems[systemId].port);
					their_addr.sin_addr = *((struct in_addr *)he->h_addr);
					bzero(&(their_addr.sin_zero), 8);
					if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
					{
						close(sockfd);
						out=-1;
					}
					else 
						attempt=0;
				}
			}
		}
		if(out==-1)
		{
			if(systems[systemId].enabled)
				attempt++;
			if(attempt>MAXATTEMPTS)
			{
				sendResetSignal(systemId);
				attempt=0;
			}
			systemDisconnected(systemId);
			sleep(2);
		}
	}
	while(out!=0);
	
	setOutputDefault(systemId);
	formatMessage(msg.mtext,0,0,systems[systemId].device_num,0,"DEVICE CONNECTED");
	msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);

	return sockfd;
}



int operate(int sockfd,int deviceId, int timeout)
/*--------------------------------------------
 * scrive su socket, attende risposta, aggiorna tabelle
 * int sockfd 	- descrittore del socket
 * int deviceId - identificatore device [0-NUMSYSTEMS-1]
 * int timeout	- timeout di attesa (tipicamente 2 secondi)
 * -----------------------------------------*/
{
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

	bool valueislow;
	bool valueishigh;
	bool iamlow;
	bool iamhigh;

	
	int analogRead;
	double analogEng;

	time_t inizio;
	
	
	if(systems[deviceId].enabled>2)	//try to reload
	{
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
		for(i=0;i<out_ch_d;i++)
		{
			if((digitalOutTable[deviceToid[2][deviceId][i]].id_digital_out!=-1)
					&&(digitalOutTable[deviceToid[2][deviceId][i]].value!=-1))
				out_data[i]=digitalOutTable[deviceToid[2][deviceId][i]].value;
			else
				out_data[i]=0;
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
						&&(analogTable[i].id_analog!=-1)
							&&(analogTable[i].enabled==1))
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
									formatMessage(msg.mtext,0,analogTable[analogId].id_analog,analogTable[analogId].device_num,analogTable[analogId].ch_num,analogTable[analogId].msg_h);
									msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
									storeMessage(0,analogTable[analogId].id_analog,analogTable[analogId].device_num,analogTable[analogId].ch_num,analogTable[analogId].msg_h);
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
										formatMessage(msg.mtext,0,analogTable[analogId].id_analog,analogTable[analogId].device_num,analogTable[analogId].ch_num,analogTable[analogId].msg_l);
										msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
										storeMessage(0,analogTable[analogId].id_analog,analogTable[analogId].device_num,analogTable[analogId].ch_num,analogTable[analogId].msg_l);
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
											formatMessage(msg.mtext,0,analogTable[analogId].id_analog,analogTable[analogId].device_num,analogTable[analogId].ch_num,"value in range");
											msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
											storeMessage(0,analogTable[analogId].id_analog,analogTable[analogId].device_num,analogTable[analogId].ch_num,"value in range");
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
									formatMessage(msg.mtext,1,digitalTable[digitalId].id_digital,digitalTable[digitalId].device_num,digitalTable[digitalId].ch_num,digitalTable[digitalId].msg);
									msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
									storeMessage(1,digitalTable[digitalId].id_digital,digitalTable[digitalId].device_num,digitalTable[digitalId].ch_num,digitalTable[digitalId].msg);
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
									formatMessage(msg.mtext,1,digitalTable[digitalId].id_digital,digitalTable[digitalId].device_num,digitalTable[digitalId].ch_num,"value in range");
									msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
									storeMessage(1,digitalTable[digitalId].id_digital,digitalTable[digitalId].device_num,digitalTable[digitalId].ch_num,"value in range");
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

void *get_shared_memory_segment(const int size, int *shmid, const char *filename)
/*--------------------------------------------
 * alloca memoria confivisa (systems,analogTable,digitalTable)
 * -----------------------------------------*/
{
	key_t key;
	int err = 0;

	void *temp_shm_pointer;

	if ((key = ftok(filename, 1 + (rand() % 255) )) == -1)
		err = 1;

	if (((*shmid) = shmget(key, size, IPC_CREAT | 0600)) == -1)
		err = 1;

	temp_shm_pointer = (void *)shmat((*shmid), 0, 0);
	if (temp_shm_pointer == (void *)(-1))
		err = 1;
	if (!err)
		return temp_shm_pointer;
	else
		return 0;
}



void doServer()
/*--------------------------------------------
 * gestisce le connessioni dei clients
 * -----------------------------------------*/
{
	int sockfd, new_fd,nbytes;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr; /* address information del chiamante */
	int sin_size;	
	unsigned char buf[MAXDATASIZE];
	int out;
	int newpid;
	const int SHM_INT_SIZE = sizeof(int);
	int i;
	int true=1;

	for(i=0;i<MAXCONNECTIONS;i++)
		serverPids[i]=-1;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &true, sizeof(true)) == -1)
		perror("setsockopt");
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYPORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);
	while(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		sleep(1);
	}

	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}
	printf("Listen on Port : %d\n",MYPORT);

	while(1)
	{
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
		{
			perror("accept");
			continue;
		}
		
		for(i=0;i<MAXCONNECTIONS;i++)
			if((serverPids[i]!=-1)&&(kill(serverPids[i],0)==-1))
				serverPids[i]=-1;
						
		i=0;
		while((i<MAXCONNECTIONS)&&(serverPids[i]!=-1))
			i++;
		if(i<MAXCONNECTIONS)
		{
//			printf("Connect!! %s is on the Server!\n",inet_ntoa(their_addr.sin_addr));
			if ((newpid=fork())==0)
			{
				do
				{
					out=0;
					// Processo figlio
					if ((nbytes=recv(new_fd,buf,MAXDATASIZE,0)) == -1)
						perror("recv");
					if(nbytes==0)
					{
						close(new_fd);
						exit(0);
					}
					buf[nbytes]='\0';

					performAction(new_fd,buf);
					out=(strstr(buf,"quit")!=NULL);
				}
				while(out==0);
				close(new_fd);
				exit(0);
			}
			serverPids[i]=newpid;
			close(new_fd);
		}
		else
		{
			printf("too many connections\n");
			close(new_fd);
		}
	}
}


void doController()
/*--------------------------------------------
 * gestisce i comandi alle schede
 * -----------------------------------------*/
{
	char writeBuffer[MSGLENGTH];

	while(1)
	{
//		printf("controllo\n");
/* giorgio
 * 		analogTable[id_analogToId(23)].value;
 *		digitalTable[id_digitalToId(1)].value;
 * 		setOutput(22,0);
 * 	
 * 		formatMessage(writeBuffer,0,0,0,0,"SYSTEM STOPPING");
 *		write(logFileFd,writeBuffer,strlen(writeBuffer));
 * 		
 * 
 * */

		
		usleep(10000);
	}
}


void performAction(int fd,char *buf)
/*--------------------------------------------
 * esegue operazione richiesta dal client (web o telnet)
 * int fd 	 - descrittore del socket
 * char *buf - stringa con il comando
 * attualmente implementate:
 * "get channels" restituisce i descrittori dei canali
 * "get values" restituisce i valori dei canali
 * "system disable %id" disabilita scheda id
 * "system enable %id" abilita scheda id
 * "chn disable %sysId %adId %chId"	disabilita canale chId tipo adId su scheda sysId
 * "chn enable %sysId %adId %chId"	abilita canale chId tipo adId su scheda sysId
 * -----------------------------------------*/
{
	char buffer[255];
	int value;
	int i,j,k;
	int state;
	MYSQL *connection=NULL, mysql;
	int systemId;
	int chnId;
	int adId;
	char query[255];
	char *c,*d;
	struct timeval tim;
	double t1;
	int rem_time;
	
	if(strstr(buf,"set output ")==buf)
	{
		if(c=strchr(&buf[11],' '))
		{
			*c='\0';
			c++;
			if(d=strchr(c,' '))
			{
				*d='\0';
				d++;
				systemId=systemNumToId(atoi(&buf[11]),NUMSYSTEMS);
				chnId=atoi(c)-1;
				printf("%d %d\n",digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out, atoi(d));
				if(((systemId<NUMSYSTEMS)&&(chnId<systems[systemId].out_ch_d))
					&&(digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out!=-1))
					setOutput(digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out,atoi(d));
			}
			else
				setOutput(atoi(&buf[11]),atoi(c));
		}
		return;
	}
	if(strstr(buf,"toggle ")==buf)
	{
		if(c=strchr(&buf[7],' '))
		{
			*c='\0';
			c++;
			systemId=systemNumToId(atoi(&buf[7]),NUMSYSTEMS);
			chnId=atoi(c)-1;
			if(((systemId<NUMSYSTEMS)&&(chnId<systems[systemId].out_ch_d))
					&&(digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out!=-1)
					&&(digitalOutTable[deviceToid[2][systemId][chnId]].value!=-1))
				setOutput(digitalOutTable[deviceToid[2][systemId][chnId]].id_digital_out,
					(1-digitalOutTable[deviceToid[2][systemId][chnId]].value));
		}
		return;
	}
	if(strstr(buf,"reset ")==buf)
	{
		sendResetSignal(systemNumToId(atoi(&buf[6]),NUMSYSTEMS));
		return;
	}
	if(strstr(buf,"reload")==buf)
	{
		gettimeofday(&tim, NULL);
		t1=tim.tv_sec+(tim.tv_usec/1000000.0);
		
		for(i=0;i<NUMSYSTEMS;i++)
			systems[i].enabled+=3;	//tell to update
		loadSystemTable(1);
		loadDigitalTable(1);
		loadDigitalOutTable(1);
		loadAnalogTable(1);
		
		gettimeofday(&tim, NULL);
		printf("reload in %f seconds\n",tim.tv_sec+(tim.tv_usec/1000000.0)-t1);
		return;
	}

	if(strstr(buf,"dig ")==buf)
	{
		if(c=strchr(&buf[4],' '))
		{
			*c='\0';
			c++;
			i=deviceToid[1][systemNumToId(atoi(&buf[4]),NUMSYSTEMS)][atoi(c)-1];
			printf("id_digital\tdescription\tdevice_num\tch_num\tenabled\n");
			printf("%d\t%s\t%d\t%d\t%d\n",digitalTable[i].id_digital,
						digitalTable[i].description,digitalTable[i].device_num,
						digitalTable[i].ch_num,digitalTable[i].enabled);
		}
		return;
	}
	if(strstr(buf,"ana ")==buf)
	{
		if(c=strchr(&buf[4],' '))
		{
			*c='\0';
			c++;
			i=deviceToid[0][systemNumToId(atoi(&buf[4]),NUMSYSTEMS)][atoi(c)-1];
			printf("id_analog\tdescription\tdevice_num\tch_num\tenabled\n");
			printf("%d\t%s\t%d\t%d\t%d\n",analogTable[i].id_analog,
						analogTable[i].description,analogTable[i].device_num,
						analogTable[i].ch_num,analogTable[i].enabled);
		}
		return;
	}
	if(strstr(buf,"out ")==buf)
	{
		if(c=strchr(&buf[4],' '))
		{
			printf("out\n");
			*c='\0';
			c++;
			i=deviceToid[2][systemNumToId(atoi(&buf[4]),NUMSYSTEMS)][atoi(c)-1];
			printf("id_digital_out\tdescription\tdevice_num\tch_num\tvalue\n");
			printf("%d\t%s\t%d\t%d\t%d\n",digitalOutTable[i].id_digital_out,
						digitalOutTable[i].description,digitalOutTable[i].device_num,
						digitalOutTable[i].ch_num,digitalOutTable[i].value);
		}
		return;
	}
	
	
	if(strstr(buf,"update")==buf)
	{
		if(strstr(&buf[6]," analog ")==&buf[6])
		{
			updateAnalogChannel(atoi(&buf[14]));
			return;
		}
		if(strstr(&buf[6]," digital ")==&buf[6])
		{
			updateDigitalChannel(atoi(&buf[15]));
			return;
		}
		if(strstr(&buf[6]," out ")==&buf[6])
		{
			updateDigitalOutChannel(atoi(&buf[11]));
			return;
		}
		return;
	}
	if(strstr(buf,"get")==buf)
	{
		if(strstr(&buf[3]," channels")==&buf[3])
		{
			for(i=0;i<NUMSYSTEMS;i++)
			{
				if(systems[i].enabled)
				{
					for(j=0;j<systems[i].in_ch_an;j++)
					{
						if(analogTable[deviceToid[0][i][j]].enabled)
						{
							k=deviceToid[0][i][j];
							sprintf(buffer,"0 %d %d %s`",analogTable[k].device_num,
									analogTable[k].ch_num,analogTable[k].description);
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								perror("send");
						}
					}
					for(j=0;j<systems[i].in_ch_d;j++)
					{
						if(digitalTable[deviceToid[1][i][j]].enabled)
						{
							k=deviceToid[1][i][j];
							sprintf(buffer,"1 %d %d %s`",digitalTable[k].device_num,
									digitalTable[k].ch_num,digitalTable[k].description);
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				perror("send");
			return;
		}
		if(strstr(&buf[3]," values")==&buf[3])
		{
			for(i=0;i<NUMSYSTEMS;i++)
			{
				if(systems[i].enabled)
				{
					for(j=0;j<systems[i].in_ch_an;j++)
					{
						if(analogTable[deviceToid[0][i][j]].enabled)
						{
							k=deviceToid[0][i][j];
							sprintf(buffer,"0 %d %d %f %f %f %f %f %d %d`",
								analogTable[k].device_num,
								analogTable[k].ch_num,
								analogTable[k].value_eng,
								analogTable[k].value_eng1,
								analogTable[k].value_eng2,
								analogTable[k].value_eng3,
								analogTable[k].value_eng4,
								analogTable[k].value,
								(analogTable[k].enabled && systems[i].enabled));

							if (send(fd, buffer, strlen(buffer), 0) == -1)
								perror("send");
						}
					}
					for(j=0;j<systems[i].in_ch_d;j++)
					{
						if(digitalTable[deviceToid[1][i][j]].enabled)
						{
							k=deviceToid[1][i][j];
							sprintf(buffer,"1 %d %d %d %d %d %d %d %d %d`",
								digitalTable[k].device_num,
								digitalTable[k].ch_num,
								digitalTable[k].value,
								digitalTable[k].value1,
								digitalTable[k].value2,
								digitalTable[k].value3,
								digitalTable[k].value4,
								digitalTable[k].value,
								(digitalTable[k].enabled && systems[i].enabled));
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				perror("send");
			return;
		}
		if(strstr(&buf[3]," outchannels")==&buf[3])
		{
			for(i=0;i<NUMSYSTEMS;i++)
			{
				if(systems[i].enabled)
				{
					for(j=0;j<systems[i].out_ch_d;j++)
					{
						if(digitalOutTable[deviceToid[2][i][j]].id_digital_out!=-1)
						{
							k=deviceToid[2][i][j];
							sprintf(buffer,"%d %d %s`",digitalOutTable[k].device_num,
								digitalOutTable[k].ch_num,digitalOutTable[k].description);
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				perror("send");
			return;
		}
		if(strstr(&buf[3]," outputs")==&buf[3])
		{
			for(i=0;i<NUMSYSTEMS;i++)
			{
				if(systems[i].enabled)
				{
					for(j=0;j<systems[i].out_ch_d;j++)
					{
						if(digitalOutTable[deviceToid[2][i][j]].id_digital_out!=-1)
						{
							rem_time=digitalOutTable[deviceToid[2][i][j]].potime - 
									(time(NULL)-digitalOutTable[deviceToid[2][i][j]].start_time);
							if(rem_time<0)
								rem_time=0;
							
							k=deviceToid[2][i][j];
							sprintf(buffer,"%d %d %d %d`",
								digitalOutTable[k].device_num,
								digitalOutTable[k].ch_num,
								digitalOutTable[k].value,
								rem_time);

							if (send(fd, buffer, strlen(buffer), 0) == -1)
								perror("send");
						}
					}
				}
			}
			if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
				perror("send");
			return;
		}
		if(strstr(&buf[3]," value ")==&buf[3])
		{
			if(strlen(&buf[10])>2)
			{
				buf[11]='\0';
				
				adId=atoi(&buf[10]);
				if(adId==0)
				{
					i=id_analogToId(atoi(&buf[12]));
					sprintf(buffer,"%f",analogTable[i].value_eng);
				}
				else
				{
					i=id_digitalToId(atoi(&buf[12]));
					sprintf(buffer,"%d",digitalTable[i].value);
				}
				if (send(fd, buffer, strlen(buffer), 0) == -1)
					perror("send");
				if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
					perror("send");
			}
			return;
		}
		return;
	}
	if(strstr(buf,"system")==buf)
	{
		if(strstr(&buf[6]," update ")==&buf[6])
		{
			strcpy(buf,&buf[14]);
			while(c=strchr(buf,'|'))
			{
				*c='\0';
				i=atoi(buf);
				updateSystem(i);
				strcpy(buf,c+1);
			}
		}
		if(strstr(&buf[6]," disable")==&buf[6])
		{
			if(strlen(&buf[14]))
			{
				systemId=atoi(&buf[14]);
				mysql_init(&mysql);
				connection = mysql_real_connect(&mysql,"localhost","root", "minair","socket", 0, NULL,0);
				if( connection == NULL ) 
					return;
				sprintf(query,"UPDATE system SET enabled=0 WHERE device_num=%d",systems[systemId].device_num);
				state = mysql_query(connection, query);
				if(state==0)
					systems[systemId].enabled=0;
				mysql_close(connection);		
				return;
			}
		}
		if(strstr(&buf[6]," enable")==&buf[6])
		{
			if(strlen(&buf[13]))
			{
				systemId=atoi(&buf[13]);
				mysql_init(&mysql);
				connection = mysql_real_connect(&mysql,"localhost","root", "minair","socket", 0, NULL,0);
				if( connection == NULL ) 
					return;
				sprintf(query,"UPDATE system SET enabled=1 WHERE device_num=%d",systems[systemId].device_num);
				state = mysql_query(connection, query);
				if(state==0)
					systems[systemId].enabled=1;
				mysql_close(connection);		
				return;
			}
		}
	}
	if(strstr(buf,"chn")==buf)
	{
		if(strstr(&buf[3]," disable ")==&buf[3])
		{
			if(strlen(&buf[12])&&(c=strchr(&buf[12],' '))&&(d=strchr(c+1,' ')))
			{
				*c='\0';
				*d='\0';
				systemId=atoi(&buf[12]);
				adId=atoi(c+1);
				chnId=atoi(d+1);
				mysql_init(&mysql);
				connection = mysql_real_connect(&mysql,"localhost","root", "minair","socket", 0, NULL,0);
				if( connection == NULL ) 
					return;
				if(adId==0)
					sprintf(query,"UPDATE analog SET enabled=0 WHERE device_num=%d AND ch_num=%d",analogTable[deviceToid[0][systemId][chnId]].device_num,analogTable[deviceToid[0][systemId][chnId]].ch_num);
				else
					sprintf(query,"UPDATE digital SET enabled=0 WHERE device_num=%d AND ch_num=%d",digitalTable[deviceToid[1][systemId][chnId]].device_num,digitalTable[deviceToid[1][systemId][chnId]].ch_num);
				state = mysql_query(connection, query);
				printf("%s\n",query);
				if(state==0)
				{
					if(adId==0)
						analogTable[deviceToid[adId][systemId][chnId]].enabled=0;
					else
						digitalTable[deviceToid[adId][systemId][chnId]].enabled=0;
				}
				return;
			}
		}
		if(strstr(&buf[3]," enable ")==&buf[3])
		{
			if(strlen(&buf[11])&&(c=strchr(&buf[11],' '))&&(d=strchr(c+1,' ')))
			{
				*c='\0';
				*d='\0';
				systemId=atoi(&buf[11]);
				adId=atoi(c+1);
				chnId=atoi(d+1);
				mysql_init(&mysql);
				connection = mysql_real_connect(&mysql,"localhost","root", "minair","socket", 0, NULL,0);
				if( connection == NULL ) 
					return;
				if(adId==0)
					sprintf(query,"UPDATE analog SET enabled=1 WHERE device_num=%d AND ch_num=%d",analogTable[deviceToid[0][systemId][chnId]].device_num,analogTable[deviceToid[0][systemId][chnId]].ch_num);
				else
					sprintf(query,"UPDATE digital SET enabled=1 WHERE device_num=%d AND ch_num=%d",digitalTable[deviceToid[1][systemId][chnId]].device_num,digitalTable[deviceToid[1][systemId][chnId]].ch_num);
				state = mysql_query(connection, query);
				printf("%s\n",query);
				if(state==0)
				{
					if(adId==0)
						analogTable[deviceToid[adId][systemId][chnId]].enabled=1;
					else
						digitalTable[deviceToid[adId][systemId][chnId]].enabled=1;
				}
				return;
			}
		}
	}
	return;
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


void initializeAnalogTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella analog
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<ANALOGCHANNELS;i++)
	{
		analogTable[i].id_analog=-1;
		strcpy(analogTable[i].form_label,"");
		strcpy(analogTable[i].description,"");
		strcpy(analogTable[i].label,"");
		strcpy(analogTable[i].sinottico,"");
		analogTable[i].device_num=-1;
		analogTable[i].ch_num=-1;
		analogTable[i].scale_zero=-1;
		analogTable[i].scale_full=-1;
		analogTable[i].range_zero=-1;
		analogTable[i].range_full=-1;
		analogTable[i].bipolar=0;
		analogTable[i].al_high_active=0;
		analogTable[i].al_high=-1;
		analogTable[i].al_low_active=0;
		analogTable[i].al_low=-1;
		analogTable[i].offset=-1;
		strcpy(analogTable[i].unit,"");
		strcpy(analogTable[i].curve,"");
		analogTable[i].no_linear=0;
		analogTable[i].printer=0;
		strcpy(analogTable[i].msg_l,"");
		strcpy(analogTable[i].msg_h,"");
		analogTable[i].time_delay_high=0;
		analogTable[i].time_delay_low=0;
		analogTable[i].enabled=0;

		resetAnalogValues(i);
	}
}

void resetAnalogValues(int i)
{
	analogTable[i].time_delay_high_cur=0;
	analogTable[i].time_delay_low_cur=0;
	analogTable[i].time_delay_high_off_cur=0;
	analogTable[i].time_delay_low_off_cur=0;
	analogTable[i].currently_high=0;
	analogTable[i].currently_low=0;
	analogTable[i].value=-1;
	analogTable[i].value1=-1;
	analogTable[i].value2=-1;
	analogTable[i].value3=-1;
	analogTable[i].value4=-1;
	analogTable[i].value_eng=-1;
	analogTable[i].value_eng1=-1;
	analogTable[i].value_eng2=-1;
	analogTable[i].value_eng3=-1;
	analogTable[i].value_eng4=-1;
}

void initializeDigitalTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella digital
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<DIGITALCHANNELS;i++)
	{
		digitalTable[i].id_digital=-1;
		strcpy(digitalTable[i].form_label,"");
		strcpy(digitalTable[i].description,"");
		strcpy(digitalTable[i].label,"");
		strcpy(digitalTable[i].sinottico,"");
		digitalTable[i].device_num=-1;
		digitalTable[i].ch_num=-1;
		digitalTable[i].printer=0;
		digitalTable[i].time_delay_on=0;
		digitalTable[i].time_delay_off=0;
		digitalTable[i].alarm_value=-1;
		strcpy(digitalTable[i].msg,"");
		digitalTable[i].enabled=0;
		
		resetDigitalValues(i);
	}
}

void resetDigitalValues(int i)
{
	digitalTable[i].currently_on=0;
	digitalTable[i].time_delay_on_cur=0;
	digitalTable[i].time_delay_off_cur=0;
	digitalTable[i].value=-1;
	digitalTable[i].value1=-1;
	digitalTable[i].value2=-1;
	digitalTable[i].value3=-1;
	digitalTable[i].value4=-1;
}

void initializeDigitalOutTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella digitalOut
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<DIGITALOUTCHANNELS;i++)
	{
		
		digitalOutTable[i].id_digital_out=-1;
		strcpy(digitalOutTable[i].description,"");
		digitalOutTable[i].device_num=-1;
		digitalOutTable[i].ch_num=-1;
		digitalOutTable[i].def=-1;
		digitalOutTable[i].potime-1;
		
		resetDigitalOutValues(i);
	}
}

void resetDigitalOutValues(int i)
{
	digitalOutTable[i].value=-1;
	digitalOutTable[i].start_time=0;
	digitalOutTable[i].act_time=0;
}

void systemDisconnected(int systemId)
/*--------------------------------------------
 * scrive su analog e digital con valori -1
 * chiamata quando non "sento" la scheda
 * int systemId - identificativo device [0-NUMSYSTEMS-1]
 * -----------------------------------------*/
{
	int i,j,id;

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
	for(j=0;j<systems[systemId].out_ch_d;j++)
	{
		id=deviceToid[2][systemId][j];
		resetDigitalOutValues(id);
	}
}


double getCurveValue(char* values,double value)
/*--------------------------------------------
 * trasforma valore raw in eng se non lineare
 * char* values - stringa dei campioni
 * double value - valore raw in ingresso
 * -----------------------------------------*/
{
	char *temp;
	bool esci=0;
	char *p;
	char *q;
	double precScale=-1;
	double precPrecScale=-1;
	double precRange=-1;
	double precPrecRange=-1;
	double pend=-1;
	double out=-1;
	int i=0;

	temp=(char *)malloc(strlen(values));
	strcpy(temp,values);
	p = strtok(temp, ";");

	while (p != NULL && esci==0)
	{
		switch(i%3)
		{
			case 1:
				if(atof(p)>value)
				{
					if((precScale!=-1)&&(precRange!=-1))
					{
						esci=1;
						q = strtok(NULL, ";");
						if(q!=NULL)
						{
							pend=(atof(q)-precRange)/(atof(p)-precScale);
							out=precRange+pend*(value-precScale);
						}
					}
					else
						precScale=atof(p);
				}
				else
				{
					precPrecScale=precScale;
					precScale=atof(p);
				}
				break;
			case 2:
				precPrecRange=precRange;
				precRange=atof(p);
				break;
			default:
				break;
		}	
		i++;
		if(!esci)
			p = strtok(NULL, ";");
	}
	free(temp);
	
	if((p==NULL)&&(precScale!=-1)&&(precRange!=-1)&&(precPrecScale!=-1)&&(precPrecRange!=-1))
	{
		pend=(precRange-precPrecRange)/(precScale-precPrecScale);
		out=precRange+pend*(value-precScale);
	}
	return out;
}


void safecpy(char *dest,char *src)
{
	char *c;
	
	strcpy(dest,src);
	if(c=strstr(dest,TERMINATOR))
		*c='f';
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
		printf("Error Creating Socket\n");
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
		printf("Error sending packet: %s\n", strerror(errno) );
		return;
	}
	totbytes+=bytes_sent;
	usleep(500000); 	//attendo mezzo secondo tra i bytes

	sprintf(buffer, "E");
	buffer_length = strlen(buffer);

	bytes_sent = sendto(sock, buffer, buffer_length, 0,(struct sockaddr*) &sa, sizeof(struct sockaddr_in) );
	if(bytes_sent < 0)
	{
		printf("Error sending packet: %s\n", strerror(errno) );
		return;
	}
	totbytes+=bytes_sent;
	
	printf("Sent %d UDP bytes to %s\n", totbytes, systems[systemId].address);
	close(sock);//close the socket
	return;
}


int getLogFileName(char *dest,char *path)
{
	time_t t;
	struct tm *tmp;
	char outstr[10];
	struct stat buf;

	if(stat(path,&buf)!=-1)
	{
		if((buf.st_mode & S_IFMT)!=S_IFDIR)
		{
			if(unlink(path)==-1)
				return -1;
		}
	}
	else
	{
		if(mkdir(path,0755)==-1)
			return -1;
	}
	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) 
		return -1;

	if (strftime(outstr, sizeof(outstr), "%Y%m%d", tmp) == 0)
		return -1;
	
	return(sprintf(dest,"%s%s.txt", path, outstr));
}


int getTimeString(char *format,char *dest)
{
	time_t t;
	struct tm *tmp;
	char outstr[50];
	struct stat buf;

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) 
		return -1;

	if (strftime(outstr, sizeof(outstr), format, tmp) == 0)
		return -1;
	else 
		strcpy(dest,outstr);
	return 0;
}

void storeMessage(int adId,int id,int systemId,int channelId,char *msg)
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
 * int adId  0=analogico, 1=digitale
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
		printf("%s\n",mysql_error(connection));
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

void updateAnalogChannel(int id_analog)
{
	int state;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[1023];
	int i,j;
	int systemId;


	sprintf(query,"SELECT form_label,description,label,"
							"sinottico,scale_zero,scale_full,range_zero,range_full,"
							"al_high_active,al_high,al_low_active,al_low,offset,"
							"unit,time_delay_high,time_delay_low,time_delay_high_off,"
							"time_delay_low_off,printer,msg_l,msg_h,enabled,"
							"device_num,ch_num,no_linear "
						"FROM analog "
						"WHERE id_analog=%d",id_analog);

	state = mysql_query(connection,query);

	if( state != 0 )
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
	
	if (( row = mysql_fetch_row(result)) != NULL )
	{
		j=id_analogToId(id_analog);//source
		systemId=systemNumToId(atoi(row[22]),NUMSYSTEMS);
		if(atoi(row[23])>0)
			i=deviceToid[0][systemId][atoi(row[23])-1];//dest
		else 
			i=-1;
			
		if(i!=-1)
		{
			analogTable[i].id_analog=id_analog;
			if(j!=-1)
			{
				if(systems[systemId].sockfd!=-1)
				{
					analogTable[i].value=analogTable[j].value;
					analogTable[i].value1=analogTable[j].value1;
					analogTable[i].value2=analogTable[j].value2;
					analogTable[i].value3=analogTable[j].value3;
					analogTable[i].value4=analogTable[j].value4;

					analogTable[i].value_eng=analogTable[j].value_eng;
					analogTable[i].value_eng1=analogTable[j].value_eng1;
					analogTable[i].value_eng2=analogTable[j].value_eng2;
					analogTable[i].value_eng3=analogTable[j].value_eng3;
					analogTable[i].value_eng4=analogTable[j].value_eng4;
	
					analogTable[i].time_delay_high_cur=analogTable[j].time_delay_high_cur;
					analogTable[i].time_delay_low_cur=analogTable[j].time_delay_low_cur;
					analogTable[i].time_delay_high_off_cur=analogTable[j].time_delay_high_off_cur;
					analogTable[i].time_delay_low_off_cur=analogTable[j].time_delay_low_off_cur;
				}
				else
					resetAnalogValues(i);

				analogTable[j].id_analog=-1;
				analogTable[j].enabled=0;
				resetAnalogValues(j);
			}
			else
				resetAnalogValues(i);
			
			if(row[0])
				safecpy(analogTable[i].form_label,row[0]);
			else
				strcpy(analogTable[i].form_label,"");

			if(row[1])
				safecpy(analogTable[i].description,row[1]);
			else
				strcpy(analogTable[i].description,"");
			if(row[2])
				safecpy(analogTable[i].label,row[2]);
			else
				strcpy(analogTable[i].label,"");

			if(row[3])
				safecpy(analogTable[i].sinottico,row[3]);
			else
				strcpy(analogTable[i].sinottico,"");

			analogTable[i].scale_zero=atoi(row[4]);
			analogTable[i].scale_full=atoi(row[5]);
			analogTable[i].range_zero=atof(row[6]);
			analogTable[i].range_full=atof(row[7]);
			analogTable[i].al_high_active=atoi(row[8]);
			analogTable[i].al_high=atof(row[9]);
			analogTable[i].al_low_active=atoi(row[10]);
			analogTable[i].al_low=atof(row[11]);
			analogTable[i].offset=atof(row[12]);

			if(row[13])
				safecpy(analogTable[i].unit,row[13]);
			else
				strcpy(analogTable[i].unit,"");
			analogTable[i].time_delay_high=atoi(row[14]);
			analogTable[i].time_delay_low=atoi(row[15]);
			analogTable[i].time_delay_high_off=atoi(row[16]);
			analogTable[i].time_delay_low_off=atoi(row[17]);
			analogTable[i].printer=atoi(row[18]);
			if(row[19])
				safecpy(analogTable[i].msg_l,row[19]);
			else
				strcpy(analogTable[i].msg_l,"");
			if(row[20])
				safecpy(analogTable[i].msg_h,row[20]);
			else
				strcpy(analogTable[i].msg_h,"");		
			analogTable[i].enabled=atoi(row[21]);
			analogTable[i].device_num=atoi(row[22]);
			analogTable[i].ch_num=atoi(row[23]);
			analogTable[i].no_linear=atoi(row[24]);
			analogTable[i].range_pend=((double)(analogTable[i].range_full-analogTable[i].range_zero))/(analogTable[i].scale_full-analogTable[i].scale_zero);
		}
		else
		{
			if(j!=-1)
			{
				analogTable[j].id_analog=-1;
				analogTable[j].enabled=0;
				resetAnalogValues(j);
			}
		}
	}
	mysql_free_result(result);	
	return;
}


void updateDigitalChannel(int id_digital)
{
	int state;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i,j;
	int deviceId;

	sprintf(query,"SELECT form_label,description,label,"
							"sinottico,printer,"
							"time_delay_on,time_delay_off,"
							"alarm_value,msg,enabled,"
							"device_num,ch_num "
						"FROM digital "
						"WHERE id_digital=%d",id_digital);

	state = mysql_query(connection,query);

	if( state != 0 )
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
	

	if (( row = mysql_fetch_row(result)) != NULL )
	{
		j=id_digitalToId(id_digital);//source

		deviceId=systemNumToId(atoi(row[10]),NUMSYSTEMS);
		if(atoi(row[11])>0)
			i=deviceToid[1][deviceId][atoi(row[11])-1];//dest
		else 
			i=-1;
			
			
		if(i!=-1) //canale di destinazione >0
		{
			digitalTable[i].id_digital=id_digital;
			if(j!=-1) //sorgente era associato 
			{
				if(systems[deviceId].sockfd!=-1)
				{
					digitalTable[i].value=digitalTable[j].value;
					digitalTable[i].value1=digitalTable[j].value1;
					digitalTable[i].value2=digitalTable[j].value2;
					digitalTable[i].value3=digitalTable[j].value3;
					digitalTable[i].value4=digitalTable[j].value4;
					digitalTable[i].time_delay_on_cur=digitalTable[j].time_delay_on_cur;
					digitalTable[i].time_delay_off_cur=digitalTable[j].time_delay_off_cur;
				}
				else
					resetDigitalValues(i);

				digitalTable[j].id_digital=-1;
				digitalTable[j].enabled=0;
				resetDigitalValues(j);
			}
			else //sorgente non era associato
				resetDigitalValues(i);
				
			
			if(row[0])
				safecpy(digitalTable[i].form_label,row[0]);
			else
				strcpy(digitalTable[i].form_label,"");
			if(row[1])
				safecpy(digitalTable[i].description,row[1]);
			else
				strcpy(digitalTable[i].description,"");
			if(row[2])
				safecpy(digitalTable[i].label,row[2]);
			else
				strcpy(digitalTable[i].label,"");
			if(row[3])
				safecpy(digitalTable[i].sinottico,row[3]);
			else
				strcpy(digitalTable[i].sinottico,"");
	
			digitalTable[i].printer=atoi(row[4]);
			digitalTable[i].time_delay_on=atoi(row[5]);
			digitalTable[i].time_delay_off=atoi(row[6]);
			digitalTable[i].alarm_value=atoi(row[7]);
			if(row[8])
				safecpy(digitalTable[i].msg,row[8]);
			else
				strcpy(digitalTable[i].msg,"");
			digitalTable[i].enabled=atoi(row[9]);
			digitalTable[i].device_num=atoi(row[10]);
			digitalTable[i].ch_num=atoi(row[11]);
		}
		else //canale di destinazione=0
		{
			if(j!=-1) //era associato
			{
				digitalTable[j].id_digital=-1;
				digitalTable[j].enabled=0;
				resetDigitalValues(j);
			}
		}
	}	
	mysql_free_result(result);	
}


void updateDigitalOutChannel(int id_digital_out)
{
	int state;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i,j;
	int systemId;


	sprintf(query,"SELECT description,device_num,ch_num,`default`,poweron_time "
						"FROM digital_out WHERE id_digital_out=%d",id_digital_out);

	state = mysql_query(connection,query);

			printf("%s %d\n",query,state);

	if( state != 0 )
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
	

	if (( row = mysql_fetch_row(result)) != NULL )
	{
		j=id_digitalOutToId(id_digital_out);//source index
		systemId=systemNumToId(atoi(row[1]),NUMSYSTEMS);

		if(atoi(row[2])>0)
			i=deviceToid[2][systemId][atoi(row[2])-1];//dest index
		else 
			i=-1;
			
			
		if(i!=-1)	//canale di destinazione >0
		{
			digitalOutTable[i].id_digital_out=id_digital_out;
			if(j!=-1) //sorgente era associato 
			{
				if(systems[systemId].sockfd!=-1)
				{
					digitalOutTable[i].start_time=digitalOutTable[j].start_time;
					digitalOutTable[i].value=digitalOutTable[j].value;
					if(digitalOutTable[i].start_time==0)
					{
						digitalOutTable[i].value=digitalOutTable[j].def;
						digitalOutTable[i].start_time=time(NULL);
					}
				}
				else
					resetDigitalOutValues(i);
				
				digitalOutTable[j].id_digital_out=-1;
				resetDigitalOutValues(j);
				printf("%d %s %d %d\n",digitalOutTable[j].id_digital_out,digitalOutTable[j].description,
						digitalOutTable[j].device_num,digitalOutTable[j].ch_num);

			}
			else //sorgente non era associato
			{	
				resetDigitalOutValues(i);
				if(systems[systemId].sockfd!=-1)
				{
					digitalOutTable[i].start_time=time(NULL);
					digitalOutTable[i].value=atoi(row[3]); //valore di default
				}
			}

			if(row[0])
				safecpy(digitalOutTable[i].description,row[0]);
			else
				safecpy(digitalOutTable[i].description,"");
			digitalOutTable[i].device_num=atoi(row[1]);
			digitalOutTable[i].ch_num=atoi(row[2]);
			digitalOutTable[i].def=atoi(row[3]);
			digitalOutTable[i].potime=atoi(row[4]);
			
			printf("%d %s %d %d\n",digitalOutTable[i].id_digital_out,digitalOutTable[i].description,
						digitalOutTable[i].device_num,digitalOutTable[i].ch_num);
		}
		else //canale di destinazione=0
		{
			if(j!=-1) //era associato
			{
				digitalOutTable[j].id_digital_out=-1;
				resetDigitalOutValues(j);
				printf("%d %s %d %d\n",digitalOutTable[j].id_digital_out,digitalOutTable[j].description,
						digitalOutTable[j].device_num,digitalOutTable[j].ch_num);

			}
		}
	}
	mysql_free_result(result);	
}


int id_analogToId(int id_analog)
{
	int i;
	
	for(i=0;i<ANALOGCHANNELS;i++)
		if(analogTable[i].id_analog==id_analog)
			break;
	return((i<ANALOGCHANNELS)?i:-1);
}

int id_digitalToId(int id_digital)
{
	int i;
	
	for(i=0;i<DIGITALCHANNELS;i++)
		if(digitalTable[i].id_digital==id_digital)
			break;
	return((i<DIGITALCHANNELS)?i:-1);
}

int id_digitalOutToId(int id_digital_out)
{
	int i;
	
	for(i=0;i<DIGITALOUTCHANNELS;i++)
		if(digitalOutTable[i].id_digital_out==id_digital_out)
			break;
	return((i<DIGITALOUTCHANNELS)?i:-1);
}

void loadSystemTable(bool onlyupdate)
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
								" WHERE system.removed=0");

	if( state != 0 )
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);

	if(!onlyupdate)
	{
		NUMSYSTEMS=mysql_num_rows(result);
		systems=(struct system *)get_shared_memory_segment(NUMSYSTEMS * sizeof(struct system), &SHMSYSTEMSID, "/dev/zero");
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
			ANALOGCHANNELS+=systems[i].in_ch_an;
			DIGITALCHANNELS+=systems[i].in_ch_d;
			DIGITALOUTCHANNELS+=systems[i].out_ch_d;
		}
		i++;
	}
	mysql_free_result(result);
}

void loadAnalogTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;

	if(!onlyupdate)
		analogTable=(struct analogLine *)get_shared_memory_segment(ANALOGCHANNELS * sizeof(struct analogLine), &SHMANALOGID, "/dev/zero");
		
	initializeAnalogTable();

	state = mysql_query(connection, "SELECT analog.id_analog,analog.form_label,"
							"analog.description,analog.label,analog.sinottico,"
							"analog.device_num,analog.ch_num,analog.scale_zero,"
							"analog.scale_full,analog.range_zero,analog.range_full,"
							"analog.bipolar,analog.al_high_active,analog.al_high,"
							"analog.al_low_active,analog.al_low,analog.offset,"
							"analog.unit,analog.time_delay_high,analog.time_delay_low,"
							"analog.time_delay_high_off,analog.time_delay_low_off,analog.curve,"
							"analog.no_linear,analog.printer,analog.msg_l,"
							"analog.msg_h,analog.enabled "
						"FROM analog JOIN system ON analog.device_num=system.device_num "
						"WHERE system.removed=0 AND analog.ch_num!=0");
	if( state != 0 ) 
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		i=systemNumToId(atoi(row[5]),NUMSYSTEMS);
		if((i!=-1) && (atoi(row[6])<=systems[i].in_ch_an))
		{
			if(!onlyupdate)
				deviceToid[0][i][atoi(row[6]) - 1]=idRow;
			else
				idRow=deviceToid[0][i][atoi(row[6]) - 1];

			analogTable[idRow].id_analog=atoi(row[0]);
			if(row[1])
				safecpy(analogTable[idRow].form_label,row[1]);
			else
				strcpy(analogTable[idRow].form_label,"");
			if(row[2])
				safecpy(analogTable[idRow].description,row[2]);
			else
				strcpy(analogTable[idRow].description,"");
			if(row[3])
				safecpy(analogTable[idRow].label,row[3]);
			else
				strcpy(analogTable[idRow].label,"");
			if(row[4])
				safecpy(analogTable[idRow].sinottico,row[4]);
			else
				strcpy(analogTable[idRow].sinottico,"");
			analogTable[idRow].device_num=atoi(row[5]);
			analogTable[idRow].ch_num=atoi(row[6]);
			analogTable[idRow].scale_zero=atoi(row[7]);
			analogTable[idRow].scale_full=atoi(row[8]);
			analogTable[idRow].range_zero=atof(row[9]);
			analogTable[idRow].range_full=atof(row[10]);
			analogTable[idRow].range_pend=((double)(analogTable[idRow].range_full-analogTable[idRow].range_zero))/(analogTable[idRow].scale_full-analogTable[idRow].scale_zero);
			analogTable[idRow].bipolar=atoi(row[11]);
			analogTable[idRow].al_high_active=atoi(row[12]);
			analogTable[idRow].al_high=atoi(row[13]);
			analogTable[idRow].al_low_active=atoi(row[14]);
			analogTable[idRow].al_low=atoi(row[15]);
			analogTable[idRow].offset=atof(row[16]);
			if(row[17])
				safecpy(analogTable[idRow].unit,row[17]);
			else
				safecpy(analogTable[idRow].unit,"");
			analogTable[idRow].time_delay_high=atoi(row[18]);
			analogTable[idRow].time_delay_low=atoi(row[19]);
			analogTable[idRow].time_delay_high_off=atoi(row[20]);
			analogTable[idRow].time_delay_low_off=atoi(row[21]);
			if(row[22])
				safecpy(analogTable[idRow].curve,row[22]);
			else
				safecpy(analogTable[idRow].curve,"");
			analogTable[idRow].no_linear=atoi(row[23]);
			analogTable[idRow].printer=atoi(row[24]);
			if(row[25])
				safecpy(analogTable[idRow].msg_l,row[25]);
			else
				safecpy(analogTable[idRow].msg_l,"");
			if(row[26])
				safecpy(analogTable[idRow].msg_h,row[26]);
			else
				safecpy(analogTable[idRow].msg_h,"");
			analogTable[idRow].enabled=atoi(row[27]);
		
			idRow++;
		}
	}
	mysql_free_result(result);
	
	if(!onlyupdate)
	{
		for(i=0;i<NUMSYSTEMS;i++)
			for(k=0;k<systems[i].in_ch_an;k++)
				if(deviceToid[0][i][k]==-1)
				{
					deviceToid[0][i][k]=idRow;
					idRow++;
				}
		if(idRow!=ANALOGCHANNELS)
			printf("there's a problem with analog channels\n");
	}
}

void loadDigitalOutTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;

	if(!onlyupdate)
		digitalOutTable=(struct digitalOutLine *)get_shared_memory_segment(DIGITALOUTCHANNELS * sizeof(struct digitalOutLine), &SHMDIGITALOUTID, "/dev/zero");
	initializeDigitalOutTable();
	
	state = mysql_query(connection, "SELECT digital_out.id_digital_out,digital_out.description,"
							"digital_out.device_num,digital_out.ch_num,"
							"digital_out.default,digital_out.poweron_time "
						"FROM digital_out JOIN system ON digital_out.device_num=system.device_num "
						"WHERE system.removed=0 AND digital_out.ch_num!=0");
	if( state != 0 )
	{
		sprintf("%s\n",mysql_error(connection));
		exit(1);
	}
	result = mysql_store_result(connection);

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		i=systemNumToId(atoi(row[2]),NUMSYSTEMS);
		if((i!=-1) && (atoi(row[3])<=systems[i].out_ch_d))
		{
			if(!onlyupdate)
				deviceToid[2][i][atoi(row[3]) - 1]=idRow;
			else
				idRow=deviceToid[2][i][atoi(row[3]) - 1];

			digitalOutTable[idRow].id_digital_out=atoi(row[0]);
			if(row[1])
				safecpy(digitalOutTable[idRow].description,row[1]);
			else
				safecpy(digitalOutTable[idRow].description,"");
			digitalOutTable[idRow].device_num=atoi(row[2]);
			digitalOutTable[idRow].ch_num=atoi(row[3]);
			digitalOutTable[idRow].def=atoi(row[4]);
			digitalOutTable[idRow].potime=atoi(row[5]);
			idRow++;
		}
	}
	mysql_free_result(result);

	if(!onlyupdate)
	{
		for(i=0;i<NUMSYSTEMS;i++)
			for(k=0;k<systems[i].out_ch_d;k++)
				if(deviceToid[2][i][k]==-1)
				{
					deviceToid[2][i][k]=idRow;
					idRow++;
				}
	
		if(idRow!=DIGITALOUTCHANNELS)
			printf("there's a problem with digital out channels\n");
	}
}


void loadDigitalTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;

	if(!onlyupdate)
		digitalTable=(struct digitalLine *)get_shared_memory_segment(DIGITALCHANNELS * sizeof(struct digitalLine), &SHMDIGITALID, "/dev/zero");

	initializeDigitalTable();
	
	state = mysql_query(connection, "SELECT digital.id_digital,digital.form_label,"
							"digital.description,digital.label,digital.sinottico,"
							"digital.device_num,digital.ch_num,digital.printer,"
							"digital.time_delay_on,digital.time_delay_off,"
							"digital.alarm_value,digital.msg,digital.enabled "
						"FROM digital JOIN system ON digital.device_num=system.device_num "
						"WHERE system.removed=0 AND digital.ch_num!=0");
	if( state != 0 )
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
		
	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		i=systemNumToId(atoi(row[5]),NUMSYSTEMS);
		if((i!=-1) && (atoi(row[6])<=systems[i].in_ch_d))
		{
			if(!onlyupdate)
				deviceToid[1][i][atoi(row[6]) - 1]=idRow;
			else
				idRow=deviceToid[1][i][atoi(row[6]) - 1];

			digitalTable[idRow].id_digital=atoi(row[0]);
			if(row[1])
				safecpy(digitalTable[idRow].form_label,row[1]);
			else
				safecpy(digitalTable[idRow].form_label,"");
			if(row[2])
				safecpy(digitalTable[idRow].description,row[2]);
			else
				safecpy(digitalTable[idRow].description,"");
			if(row[3])
				safecpy(digitalTable[idRow].label,row[3]);
			else
				safecpy(digitalTable[idRow].label,"");
			if(row[4])
				safecpy(digitalTable[idRow].sinottico,row[4]);
			else
				safecpy(digitalTable[idRow].sinottico,"");
			digitalTable[idRow].device_num=atoi(row[5]);
			digitalTable[idRow].ch_num=atoi(row[6]);
			digitalTable[idRow].printer=atoi(row[7]);
			digitalTable[idRow].time_delay_on=atoi(row[8]);
			digitalTable[idRow].time_delay_off=atoi(row[9]);
			digitalTable[idRow].alarm_value=atoi(row[10]);
			if(row[11])
				safecpy(digitalTable[idRow].msg,row[11]);
			else
				safecpy(digitalTable[idRow].msg,"");
			digitalTable[idRow].enabled=atoi(row[12]);
			idRow++;
		}
	}
	mysql_free_result(result);

	if(!onlyupdate)
	{
		for(i=0;i<NUMSYSTEMS;i++)
			for(k=0;k<systems[i].in_ch_d;k++)
				if(deviceToid[1][i][k]==-1)
				{
					deviceToid[1][i][k]=idRow;
					idRow++;
				}
	
		if(idRow!=DIGITALCHANNELS)
			printf("there's a problem with digital channels\n");
	}
}

int setOutput(int id,int value)
{
	int i;
	i=id_digitalOutToId(id);
	
	if(i==-1)
		return -1;

	if((digitalOutTable[i].potime<=0)
			||((time(NULL)-digitalOutTable[i].start_time)>=digitalOutTable[i].potime))
		digitalOutTable[i].value=value;
	return 1;
}

void setOutputDefault(int systemId)
{
	int i;
	int idRow;
	
	for(i=0;i<systems[systemId].out_ch_d;i++)
	{
		idRow=deviceToid[2][systemId][i];
		if(digitalOutTable[idRow].id_digital_out!=-1)
		{
			if(digitalOutTable[idRow].potime>0)
				digitalOutTable[idRow].start_time=time(NULL);
		
			digitalOutTable[idRow].value=digitalOutTable[idRow].def;
		}
	}
}

int h2i(char *h)
{
	char *c;
	char *d;
	int out=-1;
	
	if(c=strchr(h,':'))
	{
		*c='\0';
		c++;
		d=strchr(c,':');
		if(d)
			*d='\0';
		out=atoi(h)*60+atoi(c);
	}
	return out;
}

void i2h(char *h,int i)
{
	sprintf(h,"%d:%02d",i/60,i % 60);
}

void scriviEffemeridiSuFile(int fd)
{
	char buffer[255];
	time_t timer;
	struct tm now;
	char dawn[6];
	char sunset[6];
	
	timer=time(NULL);
	now=*localtime(&timer);
	
	i2h(dawn,effemeridiTable[now.tm_mon][now.tm_mday - 1].dawn);
	i2h(sunset,effemeridiTable[now.tm_mon][now.tm_mday - 1].sunset);
	sprintf(buffer,"caricate effemeridi: alba %s - tramonto %s\n",dawn,sunset);
	write(fd,buffer,strlen(buffer));
}
