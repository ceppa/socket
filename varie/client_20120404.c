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
#include "util.h"

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

	char *curve;
	bool no_linear;
	bool printer;
	bool sms;
	char msg_l[80];
	char msg_h[80];
	bool msg_is_event;
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
	bool sms;
	int alarm_value;
	int time_delay_on;
	int time_delay_off;
	int currently_on;
	char msg[80];
	bool msg_is_event;
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

struct readingLine
{
	int id_reading;
	char form_label[20];
	char description[50];
	char label[15];
	char sinottico[20];
	int multimeter_num;
	int ch_num;
	bool printer;
	bool sms;
	int alarm_value;
	char msg[80];
	bool msg_is_event;
	bool enabled;

	int value;
};
struct readingLine *readingTable;


struct knxLine
{
	int id_knx_line;
	int id_knx_gateway;
	bool input_output;
	char group_address[10];
	char physical_address[10];
	char data_type;
	char description[50];
	bool enabled;
	int req_value;	//value set or velue update
	int value;
	double value_eng;
};
struct knxLine *knxTable;

struct knxGateway
{
	int id_knx_gateway;
	char address[16];
	int port;
	char physical_address[15];
	int type;
	char description[50];
	bool enabled;
	char status;
	int sockfd;
	int ch_in;
	int ch_out;
	char failures;
};
struct knxGateway *knxGateways;


struct digitalOutLine
{
	int id_digital_out;
	char description[50];
	int device_num;
	int ch_num;
	int def;				// default value
	int on_value;			// on value
	int po_delay;			// poweron delay
	int on_time;			// relay impulse length (0=no impulse means no releay involved)
	int pon_time;			// time taken to fully power on
	int poff_time;			// time taken to fully power off
	int id_digital;			// associate digital input for relay value
	int value;				// current value
	int value1;				// past value
	int req_value;			// requested value
	bool printer;
	bool sms;
	char msg_l[80];			// message when going 0
	char msg_h[80];			// message when going 1
	bool msg_is_event;
	time_t start_time;		// start timestamp 
	time_t act_time;
};
struct digitalOutLine *digitalOutTable;

struct scenariBgBnLine
{
	int id_digital_out;
	bool attivo;
	bool bg;
	int ritardo;
};
struct scenariBgBnLine *scenariBgBnTable;

struct scenariPresenzeLine
{
	int id_digital_out;
	bool attivo;
	bool ciclico;
	int tempo_on;		//minuti
	int tempo_off;		//minuti
	int ora_ini;		//minuti
	int ora_fine;		//minuti
};
struct scenariPresenzeLine *scenariPresenzeTable;
char *scenariPresenzeAttivo;
char *buonGiornoAttivo;
char *buonaNotteAttivo;
int *id_digital_pioggia;

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
	char status;
	int sockfd;
};
struct system *systems;

struct multimeter
{
	int multimeter_num;
	char address[14];
	int port;
	unsigned char in_bytes_1_length;
	unsigned char *in_bytes_1;
	unsigned char in_bytes_2_length;
	unsigned char *in_bytes_2;
	unsigned char header_in_length;
	unsigned char *header_in;
	unsigned char header_out_length;
	unsigned char *header_out;
	unsigned char out_ch_1;
	unsigned char out_ch_2;
	bool enabled;
	char status;
	char failures;
	int sockfd;
};
struct multimeter *multimeters;


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

struct irrigazioneCircuitiLine
{
	int id_digital_out;
	int id_irrigazione;
	int circuito;
	int durata;
	int validita;

	time_t tempo_on_cur;
};
struct irrigazioneCircuitiLine *irrigazioneCircuitiTable;

struct irrigazioneLine
{
	int id_irrigazione;
	int ora_start;
	int ripetitivita;
	int tempo_off;
	int num_circuiti;
	int id_digital_out;
	char msg_avvio_automativo[80];
	char msg_arresto_automatico[80];
	char msg_avvio_manuale[80];
	char msg_arresto_manuale[80];
	char msg_arresto_pioggia[80];
	char msg_pioggia_noinizio[80];
	char msg_avvio_pompa[80];
	char msg_arresto_pompa[80];

	int day;
	int attivo;
	int ora_started;
	int current_circuito;
	int current_time;
	int count;	//if count>0 => attivo
};
struct irrigazioneLine *irrigazioneTable;


int msgid;

char logPath[255];
char logFileName[80];
int logFileFd=-1;

int NUMPANELS=0;
int NUMMULTIMETERS;
int NUMKNXGATEWAYS;
int NUMSYSTEMS;
int ***deviceToid; //type,num,ch
int NUMKNXCHANNELS=0;
int ANALOGCHANNELS=0;
int DIGITALCHANNELS=0;
int DIGITALOUTCHANNELS=0;
int READINGCHANNELS=0;
int SCENARIPRESENZECOUNT=0;
int SCENARIBGBNCOUNT=0;
int IRRIGAZIONECIRCUITS=0;
int IRRIGAZIONESYSTEMS=0;
int MAXATTEMPTS=5;
int OFFSET_EFFE=0;
char LOCALITA[30];
char NOME[30];
int RECORDDATATIME=300;

int SHMSYSTEMSID;
int SHMKNXGATEWAYSID;
int SHMMULTIMETERSID;
int SHMANALOGID;
int SHMDIGITALID;
int SHMDIGITALOUTID;
int SHMREADINGID;
int SHMKNXTABLEID;
int SHMSCENARIPRESENZEID;
int SHMSCENARIPRESENZEATTIVOID;
int SHMSCENARIBGATTIVOID;
int SHMSCENARIBNATTIVOID;
int SHMSCENARIBGBNID;
int SHMBUONGIORNOATTIVOID;
int SHMBUONANOTTEATTIVOID;
int SHMIRRIGAZIONEID;
int SHMIRRIGAZIONECIRCUITIID;
int SHMPIOGGIA;

int RNDSEED=0;


//SOCKET
int serverPids[MAXCONNECTIONS];		//connessioni esterne (web,telnet)
int *pid;	//connessioni alle schede
int serverPid;	//pid del server (padre di serverPids)
int controllerPid; //pid del controller

MYSQL *connection=NULL, mysql;
int **panels=NULL;
int mypid;

int doConnect(int systemId);
int doConnectMultimeter(int multimeterId);
int doConnectKnx(int knxGatewayId);
int checkSons(int *pidArray);
int operate(int sockfd,int deviceId, int timeout);
int operateMultimeter(int sockfd,int multimeterId, int timeout,int flip);
int operateKnx(int sockfd,int knxId, int timeout);
void killSons(int *pidArray);
void *get_shared_memory_segment(const int , int *shmid, const char *); 
void doServer();
void doController();
void performAction(int nwfd,char *buf);
int systemNumToId(int systemNum,int totale);
int multmeterNumToId(int multimeterNum,int totale);
void initializeAnalogTable(bool onlyupdate);
void initializeDigitalTable();
void initializeReadingTable();
void initializeDigitalOutTable();
void initializeDigitalOutLine(int i);
void initializeScenariPresenzeTable();
void initializeScenariBgBnTable();
void initializeIrrigazioneCircuitiTables();
void systemDisconnected(int systemId);
void multimeterDisconnected(int multimeterId);
void knxGatewayDisconnected(int knxGatewayId);
double getCurveValue(char* values,double value);
void safecpy(char *dest,char *src);
void sendResetSignal(int systemId);
int getLogFileName(char *dest,char *path);
int getTimeString(char *format,char *dest);
int getTime();
void formatMessage(char *buffer,int adId,int id,int systemId,int channelId,char *msg);
void storeMessage(int adId,int id,int systemId,int channelId,char *msg);
void storeEvent(int event_type,int device_num,int ch_num,int ch_id,char *msg);
void storeTables();
void updateSystem(int device_num);
void updateMultimeter(int multimeter_num);

void updateAnalogChannel(int id_analog);
void updateDigitalChannel(int id_digital);
void updateDigitalOutChannel(int fd,int id_digital_out);
void updateReading(int fd,int id_reading);
void updateDayNight(int fd);
void updatePresenze(int fd);
void updateIrrigazioneCircuiti(int fd,int id_irrigazione);
int group_addressToId(char *group_address);
int id_knx_gatewayToId(int id_knx_gateway);
int id_analogToId(int id_analog);
int id_digitalToId(int id_digital);
int id_digitalOutToId(int id);
int id_digitalOutToId_scenariPresenze(int id_digital_out);
int id_digitalOutToId_scenariBgBn(int id_digital_out,bool bg);
int id_irrigazioneToId(int id_irrigazione);
int circuito_irrigazioneToId(int id_irrigazione,int circuito_irrigazione);
void loadPanels();
int loadSystemTable(bool onlyupdate);
int loadKnx(bool onlyupdate);
int loadMultimeterTable(bool onlyupdate);
int loadAnalogTable(bool onlyupdate);
int loadReadingTable(bool onlyupdate);
int loadDigitalTable(bool onlyupdate);
int loadDigitalOutTable(bool onlyupdate);
int loadScenariPresenzeTable(bool onlyupdate);
int loadScenariBgBnTable(bool onlyupdate);
int loadIrrigazioneTables(int id_irrigazione,int circuito);
int setOutput(int id,int value);
int setKnx(int id,double value);
void setOutputDefault(int systemId);
void resetAnalogValues(int i);
void resetDigitalOutValues(int i);
void resetDeviceDigitalOut(int device_num);
void resetDigitalValues(int i);
void resetReadingValues(int i);
void resetKnxValue(int i);
void resetScenariPresenzeValues(int i);
void resetScenariBgBnValues(int i);
void resetIrrigazioneValues(int i);
void resetIrrigazioneCircuitiValues(int i);
void set_irrigazione(int fd,char *c);
void get_irrigazione(int fd,char *c);
int h2i(char *h);
void i2h(char *h,int i);
void scriviEffemeridiSuFile(int fd);
char checkBuonGiornoAttivo();
char checkBuonaNotteAttivo();
void die(char *text);
unsigned char parse_int(char *str);
void irrigazione_attiva(int id_irrigazione,char *msg);
void irrigazione_disattiva(int id_irrigazione,char *msg);
void get_irrigazione_rt(int fd,char *c);
void reset_irrigazione(int id_irrigazione);
void irrigazione_attiva_pompa(int id_irrigazione);
void irrigazione_disattiva_pompa(int id_irrigazione);
int circuiti_attivi(int id_irrigazione);
void get_sottostato(int fd,char *buf);


int circuiti_attivi(int id_irrigazione)
/*--------------------------------------------
 * 		conta quanti circuiti saranno attivabili oggi
 * -----------------------------------------*/
{
	int out=0;
	int id_circuito;
	int i;
	time_t curtime;
	struct timeval tv;
	struct tm ts;
	int wday;

	gettimeofday(&tv, NULL); 
	curtime=tv.tv_sec;
	ts=*localtime(&curtime);
	wday=(6+ts.tm_wday)%7;

	for(i=0;i<irrigazioneTable[id_irrigazione].num_circuiti;i++)
	{
		id_circuito=circuito_irrigazioneToId(irrigazioneTable[id_irrigazione].id_irrigazione,i+1);
		if(id_circuito>=0)
		{
//			printf("%d %d %d %d\n",i+1,irrigazioneCircuitiTable[id_circuito].circuito,irrigazioneCircuitiTable[id_circuito].durata,irrigazioneCircuitiTable[id_circuito].validita);

			if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
				out++;
		}
	}
	return out;
}

void irrigazione_attiva_pompa(int id_irrigazione)
/*--------------------------------------------
 * 		attiva la pompa relativa al sistema id_irrigazione
 * -----------------------------------------*/
{
	int id;

	id=id_digitalOutToId(irrigazioneTable[id_irrigazione].id_digital_out);
	if(id!=-1)
	{
		if(digitalOutTable[id].value!=digitalOutTable[id].on_value)
		{
			formatMessage(msg.mtext,3,0,irrigazioneTable[id_irrigazione].id_irrigazione,0,irrigazioneTable[id_irrigazione].msg_avvio_pompa);
			msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
			storeEvent(1,irrigazioneTable[id_irrigazione].id_irrigazione,0,0,irrigazioneTable[id_irrigazione].msg_avvio_pompa);
	
			digitalOutTable[id].value=digitalOutTable[id].on_value;
			digitalOutTable[id].req_value=digitalOutTable[id].on_value;
			digitalOutTable[id].po_delay=0;
		}
	}
}

void irrigazione_disattiva_pompa(int id_irrigazione)
/*--------------------------------------------
 * 		disattiva la pompa relativa al sistema id_irrigazione
 * -----------------------------------------*/
{
	int id;

	id=id_digitalOutToId(irrigazioneTable[id_irrigazione].id_digital_out);
	if(id!=-1)
	{
		if(digitalOutTable[id].value!=1-digitalOutTable[id].on_value)
		{
			formatMessage(msg.mtext,3,0,irrigazioneTable[id_irrigazione].id_irrigazione,0,irrigazioneTable[id_irrigazione].msg_arresto_pompa);
			msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
			storeEvent(1,irrigazioneTable[id_irrigazione].id_irrigazione,0,0,irrigazioneTable[id_irrigazione].msg_arresto_pompa);

			digitalOutTable[id].value=1-digitalOutTable[id].on_value;
			digitalOutTable[id].req_value=1-digitalOutTable[id].on_value;
			digitalOutTable[id].po_delay=0;
		}
	}
}


void irrigazione_attiva(int id_irrigazione,char *message)
/*--------------------------------------------
 * 		inizializza irrigazione per sistema id_irrigazione
 * 		usa irrigazioneTable e irrigazioneCircuitiTable
 * -----------------------------------------*/
{
	int id_circuito;
	time_t curtime;
	int minutes;
	int seconds;
	struct timeval tv;
	struct tm ts;
	int day,wday;

	if(circuiti_attivi(id_irrigazione))
	{
		irrigazione_attiva_pompa(id_irrigazione); //se già attiva non fa nulla

		formatMessage(msg.mtext,3,0,irrigazioneTable[id_irrigazione].id_irrigazione,0,message);
		msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
		storeEvent(1,irrigazioneTable[id_irrigazione].id_irrigazione,0,0,message);

		reset_irrigazione(id_irrigazione);	// azzera i circuiti
		gettimeofday(&tv, NULL); 
		curtime=tv.tv_sec;
		ts=*localtime(&curtime);
		minutes=ts.tm_hour*60+ts.tm_min;
		seconds=minutes*60+ts.tm_sec;
		day=ts.tm_mday+32*ts.tm_mon+384*ts.tm_year;
		wday=(6+ts.tm_wday)%7;

		irrigazioneTable[id_irrigazione].day=day;
		irrigazioneTable[id_irrigazione].ora_started=seconds;
		irrigazioneTable[id_irrigazione].count=0;

		irrigazioneTable[id_irrigazione].current_circuito=1;
		id_circuito=circuito_irrigazioneToId(irrigazioneTable[id_irrigazione].id_irrigazione,1);

		if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
			irrigazioneTable[id_irrigazione].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
		else
			irrigazioneTable[id_irrigazione].current_time=0;
	}
}

void reset_irrigazione(int id_irrigazione)
/*--------------------------------------------
 * 		azzera le uscite corrispondenti ai
 * 		circuiti di irrigazione
 * -----------------------------------------*/
{
	int i,j;

	for(i=0;i<IRRIGAZIONECIRCUITS;i++)
	{
		if(irrigazioneCircuitiTable[i].id_irrigazione==irrigazioneTable[id_irrigazione].id_irrigazione)
		{
			j=id_digitalOutToId(irrigazioneCircuitiTable[i].id_digital_out);
			if(j!=-1)
			{
				digitalOutTable[j].value=1-digitalOutTable[j].on_value;
				digitalOutTable[j].req_value=1-digitalOutTable[j].on_value;
				digitalOutTable[j].po_delay=0;
			}
		}
	}
}

void irrigazione_disattiva(int id_irrigazione,char *message)
/*--------------------------------------------
 * 		ferma irrigazione per sistema id_irrigazione
 * 		usa irrigazioneTable
 * -----------------------------------------*/
{
	int id_circuito=0,current_circuito=0;
	if(irrigazioneTable[id_irrigazione].current_circuito>0)
	{
		if(irrigazioneTable[id_irrigazione].current_circuito<=irrigazioneTable[id_irrigazione].num_circuiti)
		{
			id_circuito=circuito_irrigazioneToId(irrigazioneTable[id_irrigazione].id_irrigazione,irrigazioneTable[id_irrigazione].current_circuito);
			current_circuito=irrigazioneTable[id_irrigazione].current_circuito;
		}
		formatMessage(msg.mtext,3,id_circuito,irrigazioneTable[id_irrigazione].id_irrigazione,current_circuito,message);
		msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
		storeEvent(1,irrigazioneTable[id_irrigazione].id_irrigazione,current_circuito,id_circuito,message);
	}
	reset_irrigazione(id_irrigazione);
	irrigazioneTable[id_irrigazione].count=0;
	irrigazioneTable[id_irrigazione].current_circuito=0;
	irrigazioneTable[id_irrigazione].current_time=0;
	irrigazione_disattiva_pompa(id_irrigazione);
}

char *strtoupper(char *st)
/*--------------------------------------------
 * 		converti stringa in maiuscolo
 * 		stringa deve essere lunga 2
 * -----------------------------------------*/
{
	register int i;
	for (i = 0; st[i]; i++)
		st[i] = toupper(st[i]);
	return st;
}


unsigned char parse_int(char *str)
/*--------------------------------------------
 * 		converti stringa in esadecimale
 * 		stringa deve essere lunga 2
 * -----------------------------------------*/
{
    unsigned char result = 0;
    int i;
 
    for(i=0;i<2;i++)
    {
        result = result << 4;
        if(str[i] <= '9')
            result += str[i]-'0';
        else 
			result += toupper(str[i]) - ('A'-10);
    }
    return result & 0xff;
}

int getTime()
/*--------------------------------------------
 * 		numero di secondi da mezzanotte
 * -----------------------------------------*/
{
	time_t curtime;
	time_t seconds_since_midnight;
	struct tm *loctime;

	curtime = time (NULL);
	loctime = localtime (&curtime);
	seconds_since_midnight = loctime->tm_hour*3600+loctime->tm_min*60+loctime->tm_sec;
	
	return (int)seconds_since_midnight;
}

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
	int temp;
	
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
			printf("error %s, non bene, ho ricevuto sigsegv\n",strerror(errno));
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
		
		temp=NUMPANELS;
		NUMPANELS=0;
		for(i=0;i<temp;i++)
			free(panels[i]);
		if(panels)
			free(panels);
		panels=NULL;
	}


	if(shmdt(systems))
		perror("shmdt");
	shmctl(SHMSYSTEMSID, IPC_RMID, &shmid_struct);
	if(shmdt(multimeters))
		perror("shmdt");
	shmctl(SHMMULTIMETERSID, IPC_RMID, &shmid_struct);
	if(shmdt(analogTable))
		perror("shmdt");
	shmctl(SHMKNXGATEWAYSID, IPC_RMID, &shmid_struct);
	if(shmdt(knxGateways))
		perror("shmdt");
	shmctl(SHMKNXTABLEID, IPC_RMID, &shmid_struct);
	if(shmdt(knxTable))
		perror("shmdt");
	shmctl(SHMANALOGID, IPC_RMID, &shmid_struct);
	if(shmdt(readingTable))
		perror("shmdt");
	shmctl(SHMREADINGID, IPC_RMID, &shmid_struct);
	if(shmdt(digitalTable))
		perror("shmdt");
	shmctl(SHMDIGITALID, IPC_RMID, &shmid_struct);
	if(shmdt(digitalOutTable))
		perror("shmdt");
	shmctl(SHMDIGITALOUTID, IPC_RMID, &shmid_struct);
	if(shmdt(scenariPresenzeTable))
		perror("shmdt");
	shmctl(SHMSCENARIPRESENZEID, IPC_RMID, &shmid_struct);
	if(shmdt(scenariBgBnTable))
		perror("shmdt");
	shmctl(SHMSCENARIBGBNID, IPC_RMID, &shmid_struct);
	if(shmdt(irrigazioneCircuitiTable))
		perror("shmdt");
	shmctl(SHMIRRIGAZIONECIRCUITIID, IPC_RMID, &shmid_struct);
	if(shmdt(irrigazioneTable))
		perror("shmdt");
	shmctl(SHMIRRIGAZIONEID, IPC_RMID, &shmid_struct);

	exit(1);
}


int main(int argc, char *argv[])
{
	int sockfd;
	unsigned char buf[MAXDATASIZE];
	int pause=0;
	int i=0,j=0,k,l;
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
	int knx_ch_in;
	int knx_ch_out;

	if (signal (SIGINT, termination_handler) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	if (signal (SIGSEGV, termination_handler) == SIG_IGN)
		signal (SIGSEGV, SIG_IGN);
	if (signal (SIGHUP, termination_handler) == SIG_IGN)
		signal (SIGHUP, SIG_IGN);
	if (signal (SIGTERM, termination_handler) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);

	mypid=getpid();

/*	if(getenv("HOME")!=NULL)
		sprintf(logPath,"%s/log/",getenv("HOME"));
	else
		strcpy(logPath,"./log/");*/
	strcpy(logPath,"/var/www/domotica/log/");
	
	gettimeofday(&tim, NULL);
	t1=tim.tv_sec+(tim.tv_usec/1000000.0);
//MYSQL STUFF
	mysql_init(&mysql);
	my_bool reconnect = 1;
	mysql_options(&mysql, MYSQL_OPT_RECONNECT, &reconnect);
	connection = mysql_real_connect(&mysql,"localhost","root", "minair","socket", 0, NULL,0);
	if( connection == NULL ) 
	{
		printf("%s\n",mysql_error(&mysql));
		termination_handler(0);
	}

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
		termination_handler(0);
	}
		
	result = mysql_store_result(connection);
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		effemeridiTable[atoi(row[0])-1][atoi(row[1])-1].dawn=h2i(row[2])+OFFSET_EFFE;
		effemeridiTable[atoi(row[0])-1][atoi(row[1])-1].sunset=h2i(row[3])+OFFSET_EFFE;
	}
	mysql_free_result(result);
// END READ EFFEMERIDI
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


	printf("ANALOGCHANNELS - DIGITALCHANNELS - DIGITALOUTCHANNELS - READINGCHANNELS\n");
	printf("%d - %d - %d - %d\n",ANALOGCHANNELS,DIGITALCHANNELS,DIGITALOUTCHANNELS,READINGCHANNELS);
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

	loadPanels();
	buonGiornoAttivo=(char *)get_shared_memory_segment(1, &SHMBUONGIORNOATTIVOID, "/dev/zero");
	if(!buonGiornoAttivo)
		die("buonGiornoAttivo - get_shared_memory_segment\n");
	buonaNotteAttivo=(char *)get_shared_memory_segment(1, &SHMBUONANOTTEATTIVOID, "/dev/zero");
	if(!buonaNotteAttivo)
		die("buonaNotteAttivo - get_shared_memory_segment\n");
	printf("--- PANELS ---\n");
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


	printf("--- DIGITALOUT ---\n");
	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].out_ch_d;j++)
			if(digitalOutTable[deviceToid[2][i][j]].id_digital_out!=-1)
				printf("%d %d %d %s %d %d - %d %d\n",
					digitalOutTable[deviceToid[2][i][j]].id_digital_out,
					digitalOutTable[deviceToid[2][i][j]].device_num,
					digitalOutTable[deviceToid[2][i][j]].ch_num,
					digitalOutTable[deviceToid[2][i][j]].description,
					digitalOutTable[deviceToid[2][i][j]].def,
					digitalOutTable[deviceToid[2][i][j]].value,
					digitalOutTable[deviceToid[2][i][j]].po_delay,
					digitalOutTable[deviceToid[2][i][j]].on_time);

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

	printf("pid: %d\n",getpid());
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
		termination_handler(0);
	}

	msgid=msgget(MSGKEY,0777|IPC_CREAT); /* ottiene un descrittore per la chiave KEY; coda nuova */
	msg.mtype=1;   

	for(k=0;k<NUMSYSTEMS+NUMMULTIMETERS+NUMKNXGATEWAYS;k++)
	{
		pid[k]=-1;
		
		switch(pid[k]=fork()) // fork for every system or multimeter
		{
			case -1:
				perror("xe casini mulon!");
				termination_handler(0);
				break;
			case 0:
				if(k<NUMSYSTEMS)	// tabella system
				{
					if(systems[k].enabled)
						systems[k].status='1';
					sockfd=doConnect(k);
					systems[k].sockfd=sockfd;
					printf("address %s, port %d, type %d connected\n",systems[k].address, systems[k].port, systems[k].type);
					resetDeviceDigitalOut(k);
	
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
						else
							systems[k].status='0';
						usleep(1000000);	//intervallo interrogazione scheda
					}
					while(1);
				}
				else 
				{
					if(k<NUMSYSTEMS+NUMMULTIMETERS)//multimetro
					{
						l=k-NUMSYSTEMS;
						if(multimeters[l].enabled)
							multimeters[l].status='1'; //connecting
						sockfd=doConnectMultimeter(l);
						multimeters[l].sockfd=sockfd;
						printf("address %s, port %d connected\n",multimeters[l].address, multimeters[l].port);
		
						i=0;
						do
						{
							out=operateMultimeter(sockfd,multmeterNumToId(multimeters[l].multimeter_num,NUMMULTIMETERS),2,i);
	
							if(out==-1)
							{
								formatMessage(msg.mtext,0,0,multimeters[l].multimeter_num,0,"MULTIMETER DISCONNECTED");
								msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
		
								multimeterDisconnected(l);
								close(sockfd);
								multimeters[l].sockfd=-1;
								if(multimeters[l].enabled)
								{
									printf("error in %s %d\nsleeping 5 seconds\n",multimeters[l].address,multimeters[l].port);
									sleep(5);
									printf("trying to connect\n");
								}
								sockfd=doConnectMultimeter(l);
								multimeters[l].sockfd=sockfd;
								printf("address %s port %d connected\n",multimeters[l].address, multimeters[l].port);
							}
							else
								multimeters[l].status='0';	//connected
							i=(i+1)%2;
							usleep(500000);	//intervallo interrogazione scheda
						}
						while(1);
					}
					else 	//knx
					{
						l=k-NUMSYSTEMS-NUMMULTIMETERS;
						if(knxGateways[l].enabled)
							knxGateways[l].status='1'; //connecting
						sockfd=doConnectKnx(l);
						knxGateways[l].sockfd=sockfd;
						printf("address %s, port %d connected\n",knxGateways[l].address, knxGateways[l].port);
		
						i=0;
						do
						{
							out=operateKnx(sockfd,l,2);
	
							if(out==-1)
							{
								formatMessage(msg.mtext,0,0,knxGateways[l].id_knx_gateway,0,"GATEWAY DISCONNECTED");
								msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
		
								knxGatewayDisconnected(l);
								close(sockfd);
								knxGateways[l].sockfd=-1;
								if(knxGateways[l].enabled)
								{
									printf("error in %s %d\nsleeping 5 seconds\n",knxGateways[l].address,knxGateways[l].port);
									sleep(5);
									printf("trying to connect\n");
								}
								sockfd=doConnectKnx(l);
								knxGateways[l].sockfd=sockfd;
								printf("address %s port %d connected\n",knxGateways[l].address, knxGateways[l].port);
							}
							else
								knxGateways[l].status='0';	//connected
							i=(i+1)%2;
							usleep(50000);	//intervallo interrogazione scheda
						}
						while(1);
						
					}
					
				}
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
		*buonGiornoAttivo=checkBuonGiornoAttivo();
		*buonaNotteAttivo=checkBuonaNotteAttivo();

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
				systems[systemId].status='3';
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

int doConnectMultimeter(int multimeterId)
/*--------------------------------------------
 * gestisce la connessione/disconnessione dei multimetri
 * riprova ogni 10 secondi
 * int multimeterId - identificatore multimetro [0-NUMMULTIMETERS-1]
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
		printf("trying to connect to %s\n",multimeters[multimeterId].address);
		out=0;
		if(multimeters[multimeterId].enabled==0)
			out=-1;
		else
		{
			if ((he=gethostbyname(multimeters[multimeterId].address)) == NULL)
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
					their_addr.sin_port = htons(multimeters[multimeterId].port);
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
			if(multimeters[multimeterId].enabled)
				attempt++;
			if(attempt>MAXATTEMPTS)
				multimeters[multimeterId].status='3';
			multimeterDisconnected(multimeterId);
			sleep(10);
		}
	}
	while(out!=0);

	multimeters[multimeterId].failures=0;
	formatMessage(msg.mtext,0,0,multimeters[multimeterId].multimeter_num,0,"MULTIMETER CONNECTED");
	msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);

	return sockfd;
}

int group_addressToId(char *group_address)
{
	register int i;
	for(i=0;i<NUMKNXCHANNELS;i++)
		if(strcmp(knxTable[i].group_address,group_address)==0)
			break;
	return((i<NUMKNXCHANNELS)?i:-1);
}

int doConnectKnx(int knxGatewayId)
/*--------------------------------------------
 * gestisce la connessione/disconnessione dei gateway KNX
 * riprova ogni 10 secondi
 * int knxGatewayId - identificatore gateway [0-NUMKNXGATEWAYS-1]
 * -----------------------------------------*/
{
	struct hostent *he;
	struct sockaddr_in their_addr;
	int sockfd;
	int out;
	struct timeval tv;
	int attempt=0;
	char sendBuf[50];
	int received=0,numbytes;

	do
	{
		printf("trying to connect to %s\n",knxGateways[knxGatewayId].address);
		out=0;
		if(knxGateways[knxGatewayId].enabled==0)
			out=-1;
		else
		{
			if ((he=gethostbyname(knxGateways[knxGatewayId].address)) == NULL)
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

					their_addr.sin_family = AF_INET;
					their_addr.sin_port = htons(knxGateways[knxGatewayId].port);
					their_addr.sin_addr = *((struct in_addr *)he->h_addr);
					bzero(&(their_addr.sin_zero), 8);
					if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
					{
						close(sockfd);
						out=-1;
					}
					else 
					{
						attempt=0;
						if (!send(sockfd,"connect \r",10,0)) 
							out=-1;
						else
						{
							if(recv(sockfd, sendBuf, 50,0)<1)
								out=-1;
							else
							{
								strcpy(sendBuf,"dts %00010111 \r");
		
								if (!send(sockfd,sendBuf,strlen(sendBuf),0)) 
									out=-1;
							}
						}
					}
				}
			}
		}
		if(out==-1)
		{
			if(knxGateways[knxGatewayId].enabled)
				attempt++;
			if(attempt>MAXATTEMPTS)
				knxGateways[knxGatewayId].status='3';
			knxGatewayDisconnected(knxGatewayId);
			sleep(5);
		}
	}
	while(out!=0);

	knxGateways[knxGatewayId].failures=0;
	formatMessage(msg.mtext,0,0,knxGateways[knxGatewayId].id_knx_gateway,0,"GATEWAY CONNECTED");
	msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);

	return sockfd;
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

int operateKnx(int sockfd,int knxGatewayId, int timeout)
{
	time_t inizio,curtime;
	int out=0;
	int numbytes;
	int recvLen=512;
	int sendLen=512;
	unsigned char *recvBuf;
	char *sendBuf;
	int received=0;
	char source[20];
	char dest[20];
	unsigned int value;
	int i;

	// legge da knxGateway
	// aggiorna valori

	recvBuf=(unsigned char *)malloc(recvLen);

	if(knxGateways[knxGatewayId].enabled>2)	//try to reload
	{
		knxGateways[knxGatewayId].status='1';	//loading
		inizio=time(NULL);
		do
		{
			usleep(5000);
		}
		while((knxGateways[knxGatewayId].enabled>2)&&(time(NULL)-inizio<timeout));
		if(knxGateways[knxGatewayId].enabled>2)
			knxGateways[knxGatewayId].enabled-=3;
	}
	if(knxGateways[knxGatewayId].enabled!=1)
	{
		if(knxGateways[knxGatewayId].enabled>1)
			knxGateways[knxGatewayId].enabled=1;
		else
			knxGateways[knxGatewayId].status='d';
		return -1;
	}

	//receive
	received=0;
	do
	{
		numbytes=recv(sockfd, &recvBuf[received], recvLen-received, MSG_DONTWAIT);
		if(numbytes>0)
			received+=numbytes;
	}
	while((received>0)&&(recvBuf[received-1]!=0x0d));
	if(received>0)
	{
		recvBuf[received]='\0';
		switch(recvBuf[2])
		{
			case 'i':
				if(parseKnxOutput(recvBuf,source,dest,&value)!=-1)
				{
					i=group_addressToId(dest);
					if(i!=-1)
					{
						strcpy(knxTable[i].group_address,dest);
						strcpy(knxTable[i].physical_address,source);
						knxTable[i].value=value;
						if(knxTable[i].data_type=='F')
							knxTable[i].value_eng=intToDouble(value);
						else
							knxTable[i].value_eng=value;
						printf("aggiornato %s da %s, valore %d\n",dest,source,value);
					}
					else
						printf("%s non trovato\n",dest);

				}
				break;
			default:
				printf("%s\n",recvBuf);
				break;

		}
	}
	//send request value or update
	for(i=0;i<NUMKNXCHANNELS;i++)
	{
		if((knxTable[i].enabled)&&(knxTable[i].req_value!=-1))
		{
			sendBuf=(char *)malloc(sendLen);
			if(knxTable[i].input_output==1)
			{
				sprintf(sendBuf,"trs (%s) \r",
					knxTable[i].group_address);
			}
			else
			{
				if(knxTable[i].data_type!='F')
					sprintf(sendBuf,"tds (%s %d) $%x \r",
						knxTable[i].group_address,
						1,
						knxTable[i].req_value);
				else
					sprintf(sendBuf,"tds (%s %d) $%x $%x\r",
						knxTable[i].group_address,
						2,
						(knxTable[i].req_value>>8),
						(knxTable[i].req_value&0xff));
			}
			printf("richiedo: %s\n",sendBuf);
			if (!send(sockfd,sendBuf,strlen(sendBuf),0)) 
				out=-1;
			knxTable[i].req_value=-1;
			free(sendBuf);
			break;	//one update per loop!
		}
	}
	free(recvBuf);
	return out;
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
	int id_pres;
	int id_digital_out;

	int ora_ini;
	int ora_fine;
	int tempo_on;
	int tempo_off;

	struct digitalOutLine *digital_out_line;

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

				// metto simulazione presenze
				if(*scenariPresenzeAttivo==1)
				{
					if(((id_pres=id_digitalOutToId_scenariPresenze(id_digital_out))!=-1)
						&&(scenariPresenzeTable[id_pres].attivo))
					{
						ora_ini=scenariPresenzeTable[id_pres].ora_ini;
						ora_fine=scenariPresenzeTable[id_pres].ora_fine;
	
						if((((ora_fine-minutes+1440)%1440) < ((ora_fine-ora_ini+1440)%1440))
								&&(((minutes-ora_ini+1440)%1440) <= ((ora_fine-ora_ini+1440)%1440)))
						{
							tempo_on=scenariPresenzeTable[id_pres].tempo_on;
							tempo_off=scenariPresenzeTable[id_pres].tempo_off;
							if(scenariPresenzeTable[id_pres].ciclico)
							{
								if((minutes - ora_ini) % (tempo_on + tempo_off) < tempo_on)
									digital_out_line->value=digital_out_line->on_value;
								else
									digital_out_line->value=1-digital_out_line->on_value;
							}
							else
								digital_out_line->value=digital_out_line->on_value;
						}
						else
							digital_out_line->value=1-digital_out_line->on_value;
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
					formatMessage(msg.mtext,0,digital_out_line->id_digital_out,digital_out_line->device_num,digital_out_line->ch_num,digital_out_line->msg_l);
					msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
					storeMessage(2,digital_out_line->id_digital_out,digital_out_line->device_num,digital_out_line->ch_num,digital_out_line->msg_l);
				}
				else
				{
					if((digital_out_line->value==1)&&(strlen(digital_out_line->msg_h)>0))
					{
						formatMessage(msg.mtext,0,digital_out_line->id_digital_out,digital_out_line->device_num,digital_out_line->ch_num,digital_out_line->msg_h);
						msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
						storeMessage(2,digital_out_line->id_digital_out,digital_out_line->device_num,digital_out_line->ch_num,digital_out_line->msg_h);
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
							printf("I: %d - analogRead: %d - scale_full: %d - scale_zero: %d\n",i,analogRead,analogTable[analogId].scale_full,analogTable[analogId].scale_zero);

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
//						printf("2*i+1=%d - recvLen=%d - id_analog=%d - enabled=%d\n",(2*i+1),recvLen,analogTable[i].id_analog,analogTable[i].enabled);
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
 * alloca memoria condivisa (systems,analogTable,digitalTable)
 * -----------------------------------------*/
{
	key_t key;
	int err = 0;
	int n;
	int out;
	int conta=0;

	void *temp_shm_pointer;
//	n=rand() % 255;

//	for(n=0;n<255;n++)
	for(;;)
	{
		RNDSEED++;
		n=RNDSEED;
		if(n==256)
		{
			if(conta>10)
				break;
			else
			{
				RNDSEED=0;
				conta++;
			}
		}
		if ((key = ftok(filename, 1 + n )) != -1)
		{
			out=(*shmid) = shmget(key, size, IPC_CREAT | 0600);
			if (out != -1)
				break;
		}
	}
	printf("RNDSEED: %d - SHMID: %p\n",RNDSEED,shmid);

	if(n==256)
		die("shmget - reached 256\n");
	temp_shm_pointer = (void *)shmat((*shmid), 0, 0);
	if (temp_shm_pointer == (void *)(-1))
		die("shmat\n");
	return temp_shm_pointer;
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
	time_t inizio,curtime;
	int minutes,seconds;
	struct timeval tv;
	struct tm ts;
	int pioggia=0;
	char writeBuffer[MSGLENGTH];
	int i,j,id_circuito,circuito,id_irrigazione,day,wday;

	while(1)
	{
		// inizio irrigazione
		inizio=time(NULL);
		gettimeofday(&tv, NULL); 
		curtime=tv.tv_sec;
		ts=*localtime(&curtime);
		minutes=ts.tm_hour*60+ts.tm_min;
		seconds=minutes*60+ts.tm_sec;
		day=ts.tm_mday+32*ts.tm_mon+384*ts.tm_year;
		wday=(6+ts.tm_wday)%7;

		if(*id_digital_pioggia!=-1)
		{
			if(digitalTable[*id_digital_pioggia].value!=-1)
				pioggia=(digitalTable[*id_digital_pioggia].value==1);
		}

		id_irrigazione=0;

		for(i=0;i<IRRIGAZIONESYSTEMS;i++)
		{
			if(circuiti_attivi(i)>0)
			{
				
				if((irrigazioneTable[i].attivo==0)&&	// non forzato
						(irrigazioneTable[i].day!=day)&&	// non ancora eseguito 
						(minutes-irrigazioneTable[i].ora_start>=0))	// passato ora_start
				{
					if(pioggia==0)	// non piove
						irrigazione_attiva(i,irrigazioneTable[i].msg_avvio_automativo);
					else // piove, impedisco che parta irrigazione
					{
						if(pioggia==1) // piove
						{
							irrigazioneTable[i].day=day;
							formatMessage(msg.mtext,3,0,irrigazioneTable[i].id_irrigazione,0,irrigazioneTable[i].msg_pioggia_noinizio);
							msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);
							storeEvent(1,irrigazioneTable[i].id_irrigazione,0,0,irrigazioneTable[i].msg_pioggia_noinizio);
						}
					}
				}
				else
				{
					if((pioggia==1)&&(irrigazioneTable[i].attivo==0)&&(irrigazioneTable[i].current_circuito>0))
						irrigazione_disattiva(i,irrigazioneTable[i].msg_arresto_pioggia);
				}
	
	
				if(irrigazioneTable[i].current_circuito>0) // significa che in qualche modo (manuale o automatico) irrigazione è avviata
				{
					if(irrigazioneTable[i].current_circuito<=irrigazioneTable[i].num_circuiti)
					{
						id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito);
						j=id_digitalOutToId(irrigazioneCircuitiTable[id_circuito].id_digital_out);
						if(j!=-1)
						{
							if((irrigazioneTable[i].current_time-(seconds-irrigazioneTable[i].ora_started))>0) //circuito ancora attivo
							{
								if(digitalOutTable[j].value!=digitalOutTable[j].on_value)
								{
/*									formatMessage(msg.mtext,3,id_circuito,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,"Circuito attivato");
									msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);				
									storeEvent(2,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,id_circuito,"Circuito attivato");*/
								}
								irrigazione_attiva_pompa(i);	// imposto uscita a 1 anche se già lo è per essere certo che sia attiva
								digitalOutTable[j].value=digitalOutTable[j].on_value;
								digitalOutTable[j].req_value=digitalOutTable[j].on_value;
								digitalOutTable[j].po_delay=0;
							}
							else 	//circuito non più attivo
							{
/*								formatMessage(msg.mtext,3,id_circuito,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,"Circuito disattivato");
								msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);											
								storeEvent(2,irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito,id_circuito,"Circuito disattivato");*/
								digitalOutTable[j].value=1-digitalOutTable[j].on_value;
								digitalOutTable[j].req_value=1-digitalOutTable[j].on_value;
								digitalOutTable[j].po_delay=0;
								irrigazioneTable[i].ora_started=seconds;
								irrigazioneTable[i].current_circuito++;
								if(irrigazioneTable[i].current_circuito>irrigazioneTable[i].num_circuiti)
								{
									irrigazione_disattiva_pompa(i);	// imposto uscita pompa a 0
									irrigazioneTable[i].current_time=60*irrigazioneTable[i].tempo_off;
								}
								else
								{
									id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito);
									if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
										irrigazioneTable[i].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
									else
										irrigazioneTable[i].current_time=0;
								}
							}
						}
						else //id_digital_out non trovato
						{
							irrigazioneTable[i].current_circuito++;
							irrigazioneTable[i].ora_started=seconds;
							if(irrigazioneTable[i].current_circuito>irrigazioneTable[i].num_circuiti)
								irrigazioneTable[i].current_time=60*irrigazioneTable[i].tempo_off;
							else
							{
								id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,irrigazioneTable[i].current_circuito);
								printf("circuito %d %d %d\n",id_circuito,i,irrigazioneTable[i].current_circuito);
								if(irrigazioneCircuitiTable[id_circuito].validita & (1<<wday))
									irrigazioneTable[i].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
								else
									irrigazioneTable[i].current_time=0;
							}
						}
					}
					else 	// fine circuiti, attendo tempo_off oppure termino
					{
						irrigazione_disattiva_pompa(i);
	/*					if(irrigazioneTable[i].current_circuito==1+irrigazioneTable[i].num_circuiti)
						{
							formatMessage(msg.mtext,3,0,irrigazioneTable[i].id_irrigazione,0,"Fine circuiti");
							msgsnd(msgid,&msg,MSGLENGTH,IPC_NOWAIT);				
							storeEvent(1,irrigazioneTable[i].id_irrigazione,0,0,"Fine circuiti");
							irrigazioneTable[i].current_circuito++;
						}*/
						if(irrigazioneTable[i].count>=irrigazioneTable[i].ripetitivita)	// non ripeto più
							irrigazione_disattiva(i,irrigazioneTable[i].msg_arresto_automatico);
						else	//ripeto
						{
							if((irrigazioneTable[i].current_time-(seconds-irrigazioneTable[i].ora_started))<0) // passato tempo off
							{
								irrigazioneTable[i].count++;
								irrigazioneTable[i].ora_started=seconds;
								irrigazioneTable[i].current_circuito=1;
								id_circuito=circuito_irrigazioneToId(irrigazioneTable[i].id_irrigazione,1);
								irrigazioneTable[i].current_time=60*irrigazioneCircuitiTable[id_circuito].durata;
							}
						}
					}
				}
			}
			
		}

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
 * "dig %sys %chn" restituisce corrispondente canale digitale 
 * "ana %sys %chn" restituisce corrispondente canale analogico
 * "out %sys %chn" restituisce corrispondente uscita
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
	
	if(strstr(buf,"checkBuonGiorno")==buf)
	{
		*buonGiornoAttivo=checkBuonGiornoAttivo();
		sprintf(buffer,"%d %s",*buonGiornoAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			perror("send");
		return;
	}
	if(strstr(buf,"checkBuonaNotte")==buf)
	{
		*buonaNotteAttivo=checkBuonaNotteAttivo();
		sprintf(buffer,"%d %s",*buonaNotteAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			perror("send");
		return;
	}
	if(strstr(buf,"presenze_attiva")==buf)
	{
		*scenariPresenzeAttivo=1;
		sprintf(buffer,"%d %s",*scenariPresenzeAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			perror("send");
		return;
	}
	if(strstr(buf,"irrigazione_attiva ")==buf)
	{
		i=atoi(&buf[19]);
		j=id_irrigazioneToId(i);

		if(j>=0)
		{
			if(circuiti_attivi(j)>0)
			{
				irrigazioneTable[j].attivo=1;
				irrigazione_attiva(j,irrigazioneTable[j].msg_avvio_manuale);
			}
			sprintf(buffer,"%d %s",irrigazioneTable[j].attivo,TERMINATOR);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				perror("send");
		}
		else
		{
			sprintf(buffer,"-1 %s",TERMINATOR);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				perror("send");
		}
		return;
	}
	if(strstr(buf,"bg_attiva")==buf)
	{
		*buonaNotteAttivo=0;
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg==0)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				digitalOutTable[j].start_time=0;
				digitalOutTable[j].req_value=digitalOutTable[j].value;
			}
		}

		*buonGiornoAttivo=1;
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				digitalOutTable[j].start_time=time(NULL);
				digitalOutTable[j].req_value=digitalOutTable[j].on_value;
				digitalOutTable[j].po_delay=scenariBgBnTable[i].ritardo;
			}
		}
		sprintf(buffer,"%d %s",*buonGiornoAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			perror("send");
		return;
	}
	if(strstr(buf,"bn_attiva")==buf)
	{
		*buonGiornoAttivo=0;
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				digitalOutTable[j].start_time=0;
				digitalOutTable[j].req_value=digitalOutTable[j].value;
			}
		}

		*buonaNotteAttivo=1;
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg==0)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				digitalOutTable[j].start_time=time(NULL);
				digitalOutTable[j].req_value=1-digitalOutTable[j].on_value;
				digitalOutTable[j].po_delay=scenariBgBnTable[i].ritardo;
			}
		}
		sprintf(buffer,"%d %s",*buonaNotteAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			perror("send");
		return;
	}
	if(strstr(buf,"bg_disattiva")==buf)
	{
		*buonGiornoAttivo=0;
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				digitalOutTable[j].start_time=0;
				digitalOutTable[j].req_value=digitalOutTable[j].value;
			}
		}
		sprintf(buffer,"%d %s",*buonGiornoAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			perror("send");
		return;
	}
	if(strstr(buf,"bn_disattiva")==buf)
	{
		*buonaNotteAttivo=0;
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg==0)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				digitalOutTable[j].start_time=0;
				digitalOutTable[j].req_value=digitalOutTable[j].value;
			}
		}
		sprintf(buffer,"%d %s",*buonaNotteAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			perror("send");
		return;
	}
	if(strstr(buf,"presenze_disattiva")==buf)
	{
		*scenariPresenzeAttivo=0;
		sprintf(buffer,"%d %s",*scenariPresenzeAttivo,TERMINATOR);
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			perror("send");
		return;
	}
	if(strstr(buf,"irrigazione_disattiva ")==buf)
	{
		i=atoi(&buf[21]);
		i=id_irrigazioneToId(i);
		if(i>=0)
		{
			irrigazioneTable[i].attivo=0;
			irrigazione_disattiva(i,irrigazioneTable[i].msg_arresto_manuale);
			
			sprintf(buffer,"%d %s",irrigazioneTable[i].attivo,TERMINATOR);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				perror("send");
		}
		else
		{
			sprintf(buffer,"-1 %s",TERMINATOR);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				perror("send");
		}
		return;
	}
	if(strstr(buf,"set")==buf)
	{
		if(strstr(&buf[3]," knx_value ")==&buf[3])
		{
			if(c=strchr(&buf[14],' '))
			{
				*c='\0';
				c++;
				setKnx(atoi(&buf[14]),atof(c));
			}
		}
		if(strstr(&buf[3]," output ")==&buf[3])
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
				return;
			}
		}
		if(strstr(&buf[3]," irrigazione")==&buf[3])
		{
			set_irrigazione(fd,&buf[15]);
			return;
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
	if(strstr(buf,"status")==buf)
	{
		for(i=0;i<NUMSYSTEMS;i++)
			buffer[i]=systems[i].status;
		for(j=0;j<strlen(TERMINATOR);j++)
			buffer[j+i]=TERMINATOR[j];
		if (send(fd, buffer, NUMSYSTEMS+strlen(TERMINATOR), 0) == -1)
			perror("send");
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
		loadScenariPresenzeTable(1);
		loadScenariBgBnTable(1);
		loadAnalogTable(1);
		loadIrrigazioneTables(-1,-1);
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
			*c='\0';
			c++;
			i=deviceToid[2][systemNumToId(atoi(&buf[4]),NUMSYSTEMS)][atoi(c)-1];
			printf("id_digital_out\tdescription\tdevice_num\tch_num\tvalue\tpo_delay\tstart\n");
			printf("%d\t%s\t%d\t%d\t%d\t%d\t%d\n",digitalOutTable[i].id_digital_out,
						digitalOutTable[i].description,digitalOutTable[i].device_num,
						digitalOutTable[i].ch_num,digitalOutTable[i].value,
						digitalOutTable[i].po_delay,(int)digitalOutTable[i].start_time);
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
			updateDigitalOutChannel(fd,atoi(&buf[11]));
			return;
		}
		if(strstr(&buf[6]," bg")==&buf[6])
		{
			updateDayNight(fd);
			return;
		}
		if(strstr(&buf[6]," bn")==&buf[6])
		{
			updateDayNight(fd);
			return;
		}
		if(strstr(&buf[6]," presenze")==&buf[6])
		{
			updatePresenze(fd);
			return;
		}
		if(strstr(&buf[6]," irrigazione")==&buf[6])
		{
			int id_irrigazione=atoi(&buf[18]);
			if(id_irrigazione<1)
				id_irrigazione=-1;
			updateIrrigazioneCircuiti(fd,id_irrigazione);
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
			for(i=0;i<NUMMULTIMETERS;i++)
			{
				if(multimeters[i].enabled)
				{
					for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
					{
						if(readingTable[deviceToid[3][i][j]].enabled)
						{
							k=deviceToid[3][i][j];
							sprintf(buffer,"3 %d %d %d %d`",
								readingTable[k].multimeter_num,
								readingTable[k].ch_num,
								readingTable[k].value,
								(readingTable[k].enabled && multimeters[i].enabled));
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
		if(strstr(&buf[3]," latest_values")==&buf[3])
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
							sprintf(buffer,"0 %d %d %f %d`",
								analogTable[k].device_num,
								analogTable[k].ch_num,
								analogTable[k].value_eng,
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
							sprintf(buffer,"1 %d %d %d %d`",
								digitalTable[k].device_num,
								digitalTable[k].ch_num,
								digitalTable[k].value,
								(digitalTable[k].enabled && systems[i].enabled));
							if (send(fd, buffer, strlen(buffer), 0) == -1)
								perror("send");
						}
					}
					for(j=0;j<systems[i].out_ch_d;j++)
					{
						k=deviceToid[2][i][j];
						sprintf(buffer,"2 %d %d %d %d`",
							digitalOutTable[k].device_num,
							digitalOutTable[k].ch_num,
							digitalOutTable[k].value,
							systems[i].enabled);
						if (send(fd, buffer, strlen(buffer), 0) == -1)
							perror("send");
					}
				}
			}
			for(i=0;i<NUMMULTIMETERS;i++)
			{
				if(multimeters[i].enabled)
				{
					for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
					{
						if(readingTable[deviceToid[3][i][j]].enabled)
						{
							k=deviceToid[3][i][j];
							sprintf(buffer,"3 %d %d %d %d`",
								readingTable[k].multimeter_num,
								readingTable[k].ch_num,
								readingTable[k].value,
								(readingTable[k].enabled && multimeters[i].enabled));
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


		if(strstr(&buf[3]," mm_names")==&buf[3])
		{
			for(i=0;i<NUMMULTIMETERS;i++)
			{
				if(multimeters[i].enabled)
				{
					for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
					{
						if(readingTable[deviceToid[3][i][j]].enabled)
						{
							k=deviceToid[3][i][j];
							sprintf(buffer,"%d %d %s`",readingTable[k].multimeter_num,
									readingTable[k].ch_num,readingTable[k].description);
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
		if(strstr(&buf[3]," mm_values")==&buf[3])
		{
			for(i=0;i<NUMMULTIMETERS;i++)
			{
				if(multimeters[i].enabled)
				{
					for(j=0;j<multimeters[i].out_ch_1+multimeters[i].out_ch_2;j++)
					{
						if(readingTable[deviceToid[3][i][j]].enabled)
						{
							k=deviceToid[3][i][j];
							sprintf(buffer,"%d %d %d %d`",
								readingTable[k].multimeter_num,
								readingTable[k].ch_num,
								readingTable[k].value,
								(readingTable[k].enabled && multimeters[i].enabled));

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
		if(strstr(&buf[3]," knx_values")==&buf[3])
		{
			for(i=0;i<NUMKNXCHANNELS;i++)
			{
				j=id_knx_gatewayToId(knxTable[i].id_knx_gateway);
				if((knxTable[i].input_output==1)&&
					(knxGateways[j].enabled))
				{
					if(knxTable[i].data_type=='F')
						sprintf(buffer,"%d %d %f %d`",
							knxTable[i].id_knx_gateway,
							knxTable[i].id_knx_line,
							knxTable[i].value_eng,
							knxTable[i].enabled);
					else
						sprintf(buffer,"%d %d %d %d`",
							knxTable[i].id_knx_gateway,
							knxTable[i].id_knx_line,
							knxTable[i].value,
							knxTable[i].enabled);

					if (send(fd, buffer, strlen(buffer), 0) == -1)
						perror("send");
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
							rem_time=digitalOutTable[deviceToid[2][i][j]].po_delay - 
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
		if(strstr(&buf[3]," panel ")==&buf[3])
		{
			if((strlen(&buf[10])>=0)&&(atoi(&buf[10])<NUMPANELS))
			{
				for(i=0;i<16;i++)
				{
					sprintf(buffer,"%d %f`",panels[atoi(&buf[10])][i],
							analogTable[id_analogToId(panels[atoi(&buf[10])][i])].value_eng);
					if (send(fd, buffer, strlen(buffer), 0) == -1)
						perror("send");
				}
				if (send(fd, TERMINATOR, strlen(TERMINATOR), 0) == -1)
					perror("send");
			}
			return;
		}
		if(strstr(&buf[3]," presenze_attivo")==&buf[3])
		{
			sprintf(buffer,"%d %s",*scenariPresenzeAttivo,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				perror("send");
			return;
		}
		if(strstr(&buf[3]," irrigazione_attivo ")==&buf[3])
		{
			i=atoi(&buf[23]);
			i=id_irrigazioneToId(i);
			if(i>=0)
			{
				sprintf(buffer,"%d %s",(irrigazioneTable[i].current_circuito>0),TERMINATOR);
				if (send(fd, buffer, strlen(buffer), 0) == -1)
					perror("send");
			}
			else
			{
				sprintf(buffer,"-1 %s",TERMINATOR);
				if (send(fd, buffer,strlen(buffer), 0) == -1)
					perror("send");
			}


			return;
		}
		if(strstr(&buf[3]," buongiorno_attivo")==&buf[3])
		{
			*buonGiornoAttivo=checkBuonGiornoAttivo();
			sprintf(buffer,"%d %s",*buonGiornoAttivo,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				perror("send");
			return;
		}
		if(strstr(&buf[3]," buonanotte_attivo")==&buf[3])
		{
			*buonaNotteAttivo=checkBuonaNotteAttivo();
			sprintf(buffer,"%d %s",*buonaNotteAttivo,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				perror("send");
			return;
		}
		if(strstr(&buf[3]," irrigazione_rt ")==&buf[3])
		{
			get_irrigazione_rt(fd,&buf[18]);
			return;
		}
		if(strstr(&buf[3]," irrigazione ")==&buf[3])
		{
			get_irrigazione(fd,&buf[15]);
			return;
		}
		if(strstr(&buf[3]," pioggia")==&buf[3])
		{
			int pioggia=-1;
			if(*id_digital_pioggia!=-1)
			{
				if(digitalTable[*id_digital_pioggia].value!=-1)
					pioggia=(digitalTable[*id_digital_pioggia].value==1);
			}
			sprintf(buffer,"%d %s",pioggia,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				perror("send");
			return;
		}
		if(strstr(&buf[3]," effemeridi")==&buf[3])
		{
			time_t timer;
			struct tm now;
			char dawn[6];
			char sunset[6];
			timer=time(NULL);
			now=*localtime(&timer);
	
			i2h(dawn,effemeridiTable[now.tm_mon][now.tm_mday - 1].dawn);
			i2h(sunset,effemeridiTable[now.tm_mon][now.tm_mday - 1].sunset);
			sprintf(buffer,"%s %s %s",dawn,sunset,TERMINATOR);
			if (send(fd, buffer, strlen(buffer), 0) == -1)
				perror("send");
			return;
		}
		if(strstr(&buf[3]," sottostato ")==&buf[3])
		{
			get_sottostato(fd,&buf[15]);
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
				{
					systems[systemId].enabled=0;
					systems[systemId].status='d';
				}
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


void initializeAnalogTable(bool onlyupdate)
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
		if(onlyupdate && (analogTable[i].curve!=NULL))
			free(analogTable[i].curve);
		analogTable[i].curve=NULL;
		analogTable[i].no_linear=0;
		analogTable[i].printer=0;
		analogTable[i].sms=0;
		strcpy(analogTable[i].msg_l,"");
		strcpy(analogTable[i].msg_h,"");
		analogTable[i].msg_is_event=0;
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


void initializeIrrigazioneCircuitiTables()
/*--------------------------------------------
 * tutti valori iniziali su tabella irrigazione
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<IRRIGAZIONECIRCUITS;i++)
	{
		irrigazioneCircuitiTable[i].id_irrigazione=-1;
		irrigazioneCircuitiTable[i].circuito=-1;
		irrigazioneCircuitiTable[i].durata=-1;
		irrigazioneCircuitiTable[i].id_digital_out=-1;
		irrigazioneCircuitiTable[i].validita=-1;
		resetIrrigazioneCircuitiValues(i);
	}
	for(i=0;i<IRRIGAZIONESYSTEMS;i++)
	{
		irrigazioneTable[i].ora_start=-1;
		irrigazioneTable[i].ripetitivita=-1;
		irrigazioneTable[i].tempo_off=-1;
		irrigazioneTable[i].id_digital_out=-1;
		irrigazioneTable[i].num_circuiti=0;
		irrigazioneTable[i].attivo=0;
		resetIrrigazioneValues(i);
	}
}


void resetIrrigazioneCircuitiValues(int i)
{
	irrigazioneCircuitiTable[i].tempo_on_cur=0;
}

void resetIrrigazioneValues(int i)
{
	irrigazioneTable[i].count=0;
	irrigazioneTable[i].day=-1;
	irrigazioneTable[i].current_circuito=-1;
	irrigazioneTable[i].current_time=-1;
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
		digitalTable[i].sms=0;
		digitalTable[i].time_delay_on=0;
		digitalTable[i].time_delay_off=0;
		digitalTable[i].alarm_value=-1;
		strcpy(digitalTable[i].msg,"");
		digitalTable[i].msg_is_event=0;
		digitalTable[i].enabled=0;

		resetDigitalValues(i);
	}
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

void resetReadingValues(int i)
{
	readingTable[i].value=-1;
}

void resetKnxValue(int i)
{
	knxTable[i].value=-1;
	knxTable[i].value_eng=-1;
	knxTable[i].req_value=-1;
}

void initializeDigitalOutLine(int i)
{
	digitalOutTable[i].id_digital_out=-1;
	strcpy(digitalOutTable[i].description,"");
	digitalOutTable[i].device_num=-1;
	digitalOutTable[i].ch_num=-1;
	digitalOutTable[i].def=-1;
	digitalOutTable[i].po_delay=-1;
	digitalOutTable[i].on_value=-1;
	digitalOutTable[i].on_time=-1;
	digitalOutTable[i].pon_time=-1;
	digitalOutTable[i].poff_time=-1;
	digitalOutTable[i].req_value=0;
	digitalOutTable[i].value=0;
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
		initializeDigitalOutLine(i);
		resetDigitalOutValues(i);
	}
}


void initializeScenariPresenzeTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella scenariPresenze
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<SCENARIPRESENZECOUNT;i++)
	{
		scenariPresenzeTable[i].id_digital_out=-1;
		resetScenariPresenzeValues(i);
	}
}

void initializeScenariBgBnTable()
/*--------------------------------------------
 * tutti valori iniziali su tabella scenariPresenze
 * chiamata all'inizio
 * -----------------------------------------*/
{
	int i;
	for(i=0;i<SCENARIBGBNCOUNT;i++)
	{
		scenariBgBnTable[i].id_digital_out=-1;
		resetScenariBgBnValues(i);
	}
}


void resetDigitalOutValues(int i)
{
	digitalOutTable[i].value=0;
	digitalOutTable[i].req_value=0;
	digitalOutTable[i].start_time=time(NULL);
	digitalOutTable[i].act_time=0;
}

void resetDeviceDigitalOut(int systemId)
{
	int j,id;
	for(j=0;j<systems[systemId].out_ch_d;j++)
	{
		id=deviceToid[2][systemId][j];
		resetDigitalOutValues(id);
	}
}


void resetScenariPresenzeValues(int i)
{
	scenariPresenzeTable[i].attivo=0;
	scenariPresenzeTable[i].ciclico=0;
	scenariPresenzeTable[i].tempo_on=0;
	scenariPresenzeTable[i].tempo_off=0;
	scenariPresenzeTable[i].tempo_on=0;
	scenariPresenzeTable[i].tempo_off=0;
}

void resetScenariBgBnValues(int i)
{
	scenariBgBnTable[i].attivo=0;
	scenariBgBnTable[i].ritardo=0;
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


void knxGatewayDisconnected(int knxGatewayId)
/*--------------------------------------------
 * scrive su knxTable -1
 * chiamata quando non "sento" la scheda
 * int knxGatewayId - identificativo device [0-NUMKNXGATEWAYS-1]
 * -----------------------------------------*/
{
	int i,j,id;

	knxGateways[knxGatewayId].failures=0;
	if((knxGateways[knxGatewayId].enabled)
			&&(knxGateways[knxGatewayId].status!='3'))
		knxGateways[knxGatewayId].status='2';
	for(j=0;j<knxGateways[knxGatewayId].ch_in;j++)
	{
		id=deviceToid[4][knxGatewayId][j];
		resetKnxValue(id);
	}
	for(j=0;j<knxGateways[knxGatewayId].ch_out;j++)
	{
		id=deviceToid[5][knxGatewayId][j];
		resetKnxValue(id);
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
		perror("Error creating socket for reset");
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
		perror("Error sending first reset packet");
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
		perror("Error sending  second reset packet");
		close(sock);//close the socket
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
							"device_num,ch_num,no_linear,sms,msg_is_event "
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
			analogTable[i].sms=atoi(row[25]);
			analogTable[i].msg_is_event=atoi(row[26]);
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
							"device_num,ch_num,sms,msg_is_event "
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
			digitalTable[i].sms=atoi(row[12]);
			digitalTable[i].msg_is_event=atoi(row[13]);
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

void updateDayNight(int fd)
{
	char buffer[255];
	char ok[3];

	if(loadScenariBgBnTable(1))
		strcpy(ok,"ko");
	else
		strcpy(ok,"ok");

	sprintf(buffer,"%s %s",ok,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		perror("send");
}

void updatePresenze(int fd)
{
	char buffer[255];
	char ok[3];

	if(loadScenariPresenzeTable(1))
		strcpy(ok,"ko");
	else
		strcpy(ok,"ok");

	sprintf(buffer,"%s %s",ok,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		perror("send");
}

void updateIrrigazioneCircuiti(int fd,int id_irrigazione)
{
	char buffer[255];
	char ok[3];

	if(loadIrrigazioneTables(id_irrigazione,-1))
		strcpy(ok,"ko");
	else
		strcpy(ok,"ok");

	sprintf(buffer,"%s %s",ok,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		perror("send");
}


void updateDigitalOutChannel(int fd,int id_digital_out)
{
	int state;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[255];
	int i,j;
	int systemId;
	char buffer[255];
	char ok[3];

	sprintf(query,"SELECT description,device_num,ch_num,`default`,po_delay,on_time,pon_time,poff_time "
						"FROM digital_out WHERE id_digital_out=%d",id_digital_out);

	state = mysql_query(connection,query);
	printf("%s %d\n",query,state);

	if( state != 0 )
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
	
	strcpy(ok,"ko");
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
					digitalOutTable[i].def=digitalOutTable[j].def;
					digitalOutTable[i].po_delay=digitalOutTable[j].po_delay;
					digitalOutTable[i].on_time=digitalOutTable[j].on_time;
					digitalOutTable[i].pon_time=digitalOutTable[j].pon_time;
					digitalOutTable[i].poff_time=digitalOutTable[j].poff_time;
					digitalOutTable[i].req_value=digitalOutTable[j].req_value;
				}
				else
					initializeDigitalOutLine(i);

				if(i!=j)
				{
					digitalOutTable[j].id_digital_out=-1;
					initializeDigitalOutLine(j);
					printf("%d %s %d %d\n",digitalOutTable[j].id_digital_out,digitalOutTable[j].description,
						digitalOutTable[j].device_num,digitalOutTable[j].ch_num);
				}
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
			digitalOutTable[i].po_delay=atoi(row[4]);
			digitalOutTable[i].on_time=atoi(row[5]);
			digitalOutTable[i].pon_time=atoi(row[6]);
			digitalOutTable[i].poff_time=atoi(row[7]);
			digitalOutTable[i].req_value=digitalOutTable[i].def;
			if(digitalOutTable[i].po_delay > 0)
			{
				digitalOutTable[i].start_time=time(NULL);
				digitalOutTable[i].value=1-digitalOutTable[i].req_value;
			}

			printf("%d %s %d %d\n",digitalOutTable[i].id_digital_out,digitalOutTable[i].description,
						digitalOutTable[i].device_num,digitalOutTable[i].ch_num);
		}
		else //canale di destinazione=0
		{
			if(j!=-1) //era associato
			{
				digitalOutTable[j].id_digital_out=-1;
				initializeDigitalOutLine(j);
				printf("%d %s %d %d\n",digitalOutTable[j].id_digital_out,digitalOutTable[j].description,
						digitalOutTable[j].device_num,digitalOutTable[j].ch_num);
			}
		}
		strcpy(ok,"ok");
	}
	mysql_free_result(result);	

	sprintf(buffer,"%s %s",ok,TERMINATOR);
	if (send(fd, buffer, strlen(buffer), 0) == -1)
		perror("send");
}

int id_knx_lineToId(int id_knx_line)
{
	int i;
	
	for(i=0;i<NUMKNXCHANNELS;i++)
		if(knxTable[i].id_knx_line==id_knx_line)
			break;
	return((i<NUMKNXCHANNELS)?i:-1);
}


int id_knx_gatewayToId(int id_knx_gateway)
{
	int i;
	
	for(i=0;i<NUMKNXGATEWAYS;i++)
		if(knxGateways[i].id_knx_gateway==id_knx_gateway)
			break;
	return((i<NUMKNXGATEWAYS)?i:-1);
}

int id_analogToId(int id_analog)
{
	int i;
	
	for(i=0;i<ANALOGCHANNELS;i++)
		if(analogTable[i].id_analog==id_analog)
			break;
	return((i<ANALOGCHANNELS)?i:-1);
}

int id_irrigazioneToId(int id_irrigazione)
{
	int i;
	
	for(i=0;i<IRRIGAZIONESYSTEMS;i++)
		if(irrigazioneTable[i].id_irrigazione==id_irrigazione)
			break;
	return((i<IRRIGAZIONESYSTEMS)?i:-1);
}

int circuito_irrigazioneToId(int id_irrigazione,int circuito_irrigazione)
{
	int i;
	
	for(i=0;i<IRRIGAZIONECIRCUITS;i++)
		if((irrigazioneCircuitiTable[i].id_irrigazione==id_irrigazione)&&
				(irrigazioneCircuitiTable[i].circuito==circuito_irrigazione))
			break;
	return((i<IRRIGAZIONECIRCUITS)?i:-1);
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

int id_readingToId(int id_reading)
{
	int i;
	
	for(i=0;i<READINGCHANNELS;i++)
		if(readingTable[i].id_reading==id_reading)
			break;
	return((i<READINGCHANNELS)?i:-1);
}

int id_digitalOutToId_scenariPresenze(int id_digital_out)
{
	int i;
	
	for(i=0;i<SCENARIPRESENZECOUNT;i++)
		if(scenariPresenzeTable[i].id_digital_out==id_digital_out)
			break;
	return((i<SCENARIPRESENZECOUNT)?i:-1);
}

int id_digitalOutToId_scenariBgBn(int id_digital_out,bool bg)
{
	int i;
	
	for(i=0;i<SCENARIBGBNCOUNT;i++)
		if((scenariBgBnTable[i].id_digital_out==id_digital_out) && (scenariBgBnTable[i].bg==bg))
			break;
	return((i<SCENARIBGBNCOUNT)?i:-1);
}

void loadPanels()
{
	int curPanel=0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,j;
	int temp;

	
	temp=NUMPANELS;
	NUMPANELS=0;
	for(i=0;i<temp;i++)
		free(panels[i]);
	if(panels)
		free(panels);
	panels=NULL;
	
	state = mysql_query(connection, "SELECT max(panel_num) FROM panel");
	if( state != 0 )
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
	if( ( row = mysql_fetch_row(result)) != NULL )
		NUMPANELS=atoi(row[0]);
		
//	printf("%d\n",NUMPANELS);
	mysql_free_result(result);
	panels=(int **)malloc(NUMPANELS);
	for(i=0;i<NUMPANELS;i++)
	{
		panels[i]=(int *)malloc(16*sizeof(int));
		for(j=0;j<16;j++)
			panels[i][j]=0;
	}
	

	state = mysql_query(connection, "SELECT panel_num,id_analog FROM panel ORDER BY panel_num,id");
	if( state != 0 )
		printf("%s\n",mysql_error(connection));
	result = mysql_store_result(connection);
	
	i=0;

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		if(atoi(row[0])!=curPanel)
		{
			curPanel=atoi(row[0]);
			i=0;
		}
		panels[curPanel-1][i]=atoi(row[1]);
		i++;
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
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

	if(!onlyupdate)
	{
		NUMSYSTEMS=mysql_num_rows(result);
		systems=(struct system *)get_shared_memory_segment(NUMSYSTEMS * sizeof(struct system), &SHMSYSTEMSID, "/dev/zero");
		if(!systems)
			die("systems - get_shared_memory_segment\n");
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
		multimeters=(struct multimeter *)get_shared_memory_segment(NUMMULTIMETERS * sizeof(struct multimeter), &SHMMULTIMETERSID, "/dev/zero");
		if(!multimeters)
			die("multimeters - get_shared_memory_segment\n");
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


int loadKnx(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_RES *resultLines;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	char query[512];
	char querylines[512];
	
	strcpy(query,"SELECT knx_gateway.id,"
					"knx_gateway.address,"
					"knx_gateway.port,"
					"knx_gateway.physical_address,"
					"knx_gateway.type,"
					"knx_gateway.description,"
					"knx_gateway.enabled"
					" FROM knx_gateway"
					" ORDER BY knx_gateway.id");
	state = mysql_query(connection, query);

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);



	strcpy(querylines,"SELECT knx_in_out.id,"
					"knx_in_out.id_knx_gateway,"
					"knx_in_out.input_output,"
					"knx_in_out.group_address,"
					"knx_in_out.data_type,"
					"knx_in_out.description,"
					"knx_in_out.enabled"
					" FROM knx_in_out"
					" ORDER BY knx_in_out.id");
	state = mysql_query(connection, querylines);

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	resultLines = mysql_store_result(connection);


	if(!onlyupdate)
	{
		NUMKNXGATEWAYS=mysql_num_rows(result);
		NUMKNXCHANNELS=mysql_num_rows(resultLines);
		knxGateways=(struct knxGateway *)get_shared_memory_segment(NUMKNXGATEWAYS * sizeof(struct knxGateway), &SHMKNXGATEWAYSID, "/dev/zero");
		if(!knxGateways)
			die("knxGateways - get_shared_memory_segment\n");
		knxTable=(struct knxLine *)get_shared_memory_segment(NUMKNXCHANNELS * sizeof(struct knxLine), &SHMKNXTABLEID, "/dev/zero");
		if(!knxTable)
			die("knxTable - get_shared_memory_segment\n");
	}
	i=0;

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		knxGateways[i].id_knx_gateway=atoi(row[0]);
		if(row[1])
			strcpy(knxGateways[i].address,row[1]);
		else
			strcpy(knxGateways[i].address,"");
		knxGateways[i].port=atoi(row[2]);
		if(row[3])
			strcpy(knxGateways[i].physical_address,row[3]);
		else
			strcpy(knxGateways[i].physical_address,"");

		knxGateways[i].type=atoi(row[4]);
		if(row[5])
			strcpy(knxGateways[i].description,row[5]);
		else
			strcpy(knxGateways[i].description,"");
		
		knxGateways[i].enabled=atoi(row[6]);

		if(!onlyupdate)
		{
			if(knxGateways[i].enabled)
				knxGateways[i].status='1';
			else
				knxGateways[i].status='d';
		}
		knxGateways[i].ch_in=0;
		knxGateways[i].ch_out=0;
		knxGateways[i].failures=0;
		i++;
	}
	mysql_free_result(result);

	i=0;
	while( ( row = mysql_fetch_row(resultLines)) != NULL )
	{
		knxTable[i].id_knx_line=atoi(row[0]);
		knxTable[i].id_knx_gateway=atoi(row[1]);
		knxTable[i].input_output=atoi(row[2]);
		if(row[3])
			strcpy(knxTable[i].group_address,row[3]);
		else
			strcpy(knxTable[i].group_address,"");
		if(row[4])
			knxTable[i].data_type=row[4][0];
		else
			knxTable[i].data_type=0;
		if(row[5])
			strcpy(knxTable[i].description,row[5]);
		else
			strcpy(knxTable[i].description,"");
		knxTable[i].enabled=atoi(row[6]);

		k=id_knx_gatewayToId(knxTable[i].id_knx_gateway);
		if(k!=-1)
		{
			if(knxTable[i].input_output==1)
				knxGateways[k].ch_in++;
			else
				knxGateways[k].ch_out++;
		}
		strcpy(knxTable[i].physical_address,"");
		knxTable[i].value=-1;
		knxTable[i].value_eng=-1;
		knxTable[i].req_value=-1;
		i++;
	}
	mysql_free_result(resultLines);
	return(0);
}



int loadAnalogTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;

	if(!onlyupdate)
	{
		analogTable=(struct analogLine *)get_shared_memory_segment(ANALOGCHANNELS * sizeof(struct analogLine), &SHMANALOGID, "/dev/zero");
		if(!analogTable)
			die("analogTable - get_shared_memory_segment\n");
	}

	if(!analogTable)
		die("analogTable - get_shared_memory_segment\n");
	initializeAnalogTable(onlyupdate);
	state = mysql_query(connection, "SELECT analog.id_analog,analog.form_label,"
							"analog.description,analog.label,analog.sinottico,"
							"analog.device_num,analog.ch_num,analog.scale_zero,"
							"analog.scale_full,analog.range_zero,analog.range_full,"
							"analog.bipolar,analog.al_high_active,analog.al_high,"
							"analog.al_low_active,analog.al_low,analog.offset,"
							"analog.unit,analog.time_delay_high,analog.time_delay_low,"
							"analog.time_delay_high_off,analog.time_delay_low_off,analog.curve,"
							"analog.no_linear,analog.printer,analog.msg_l,"
							"analog.msg_h,analog.enabled,analog.sms,analog.msg_is_event "
						"FROM analog JOIN system ON analog.device_num=system.device_num "
						"WHERE system.removed=0 AND analog.ch_num!=0"
						" ORDER BY analog.device_num, analog.ch_num");
	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
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
			{
				analogTable[idRow].curve=(char *)malloc(strlen(row[22])+1);
				safecpy(analogTable[idRow].curve,row[22]);
			}

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
			analogTable[idRow].sms=atoi(row[28]);
			analogTable[idRow].msg_is_event=atoi(row[29]);
		
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
	return(0);
}

int loadIrrigazioneTables(int id_irrigazione,int circuito)
{
	// circuito=0 => crea tabelle irrigazione e irrigazione_circuiti
	// id_irrigazione>0,circuito>0 => aggiorna circuito su irrigazione_circuiti
	// id_irrigazione>0,circuito<0 => aggiorna id_irrigazione su irrigazione
	// id_irrigazione<0,circuito<0 => aggiorna completamente tabelle

	char query[1024];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	int id_row_irrigazione;

	if(circuito==0)
		id_irrigazione=0;
	if(id_irrigazione==0)
		circuito=0;

	if(id_irrigazione<0)
		circuito=-1;

	if(circuito==0)
	{
		sprintf(query,"SELECT id AS id_irrigazione "
						"FROM irrigazione");
		state = mysql_query(connection,query);	
		if( state != 0 )
		{
			printf("%s\n",mysql_error(connection));
			return(1);
		}
		result = mysql_store_result(connection);

		IRRIGAZIONESYSTEMS=mysql_num_rows(result);
		irrigazioneTable=(struct irrigazioneLine *)get_shared_memory_segment(IRRIGAZIONESYSTEMS * sizeof(struct irrigazioneLine), &SHMIRRIGAZIONEID, "/dev/zero");
		if(!irrigazioneTable)
			die("irrigazioneTable - get_shared_memory_segment\n");

		idRow=0;
		while((row = mysql_fetch_row(result)) != NULL )
		{
			irrigazioneTable[idRow].id_irrigazione=atoi(row[0]);
			idRow++;
		}
		mysql_free_result(result);

		sprintf(query,"SELECT count(*) as num_circuiti "
						"FROM irrigazione_circuiti ");
		state = mysql_query(connection,query);	
		if( state != 0 )
		{
			printf("%s\n",mysql_error(connection));
			return(1);
		}
		result = mysql_store_result(connection);
		if((row = mysql_fetch_row(result)) != NULL )
			IRRIGAZIONECIRCUITS=atoi(row[0]);
		mysql_free_result(result);

		irrigazioneCircuitiTable=(struct irrigazioneCircuitiLine *)get_shared_memory_segment(IRRIGAZIONECIRCUITS * sizeof(struct irrigazioneCircuitiLine), &SHMIRRIGAZIONECIRCUITIID, "/dev/zero");
		if(!irrigazioneCircuitiTable)
			die("irrigazioneCircuitiTable - get_shared_memory_segment\n");
	}

	if((circuito<=0)&&(id_irrigazione<=0))
	{
		initializeIrrigazioneCircuitiTables();
		sprintf(query,"SELECT irrigazione.id as id_irrigazione,"
							"irrigazione_circuiti.circuito,"
							"irrigazione.ora_start,"
							"irrigazione_circuiti.durata,"
							"irrigazione_circuiti.validita,"
							"irrigazione.ripetitivita,"
							"irrigazione.tempo_off,"
							"irrigazione_circuiti.id_digital_out,"
							"irrigazione.id_digital_out,"
							"irrigazione.msg_avvio_automativo,"
							"irrigazione.msg_arresto_automatico,"
							"irrigazione.msg_avvio_manuale,"
							"irrigazione.msg_arresto_manuale,"
							"irrigazione.msg_arresto_pioggia,"
							"irrigazione.msg_pioggia_noinizio,"
							"irrigazione.msg_avvio_pompa,"
							"irrigazione.msg_arresto_pompa "
						"FROM irrigazione LEFT JOIN irrigazione_circuiti "
						"ON irrigazione.id=irrigazione_circuiti.id_irrigazione "
						"ORDER BY irrigazione.id,irrigazione_circuiti.circuito");

		state = mysql_query(connection,query);	
		if( state != 0 )
		{
			printf("%s\n%s\n",query,mysql_error(connection));
			return(1);
		}
		result = mysql_store_result(connection);
	
		idRow=0;
		while( ( row = mysql_fetch_row(result)) != NULL )
		{
			id_irrigazione=id_irrigazioneToId(atoi(row[0]));
			irrigazioneTable[id_irrigazione].id_irrigazione=atoi(row[0]);
			irrigazioneCircuitiTable[idRow].id_irrigazione=atoi(row[0]);
			irrigazioneCircuitiTable[idRow].circuito=atoi(row[1]);
			irrigazioneTable[id_irrigazione].ora_start=atoi(row[2]);
			irrigazioneCircuitiTable[idRow].durata=atoi(row[3]);
			irrigazioneCircuitiTable[idRow].validita=atoi(row[4]);
			irrigazioneTable[id_irrigazione].ripetitivita=atoi(row[5]);
			irrigazioneTable[id_irrigazione].tempo_off=atoi(row[6]);
			irrigazioneCircuitiTable[idRow].id_digital_out=atoi(row[7]);
			irrigazioneTable[id_irrigazione].id_digital_out=atoi(row[8]);

			strcpy(irrigazioneTable[id_irrigazione].msg_avvio_automativo,row[9]);
			strcpy(irrigazioneTable[id_irrigazione].msg_arresto_automatico,row[10]);
			strcpy(irrigazioneTable[id_irrigazione].msg_avvio_manuale,row[11]);
			strcpy(irrigazioneTable[id_irrigazione].msg_arresto_manuale,row[12]);
			strcpy(irrigazioneTable[id_irrigazione].msg_arresto_pioggia,row[13]);
			strcpy(irrigazioneTable[id_irrigazione].msg_pioggia_noinizio,row[14]);
			strcpy(irrigazioneTable[id_irrigazione].msg_avvio_pompa,row[15]);
			strcpy(irrigazioneTable[id_irrigazione].msg_arresto_pompa,row[16]);

			irrigazioneTable[id_irrigazione].count=0;
			irrigazioneTable[id_irrigazione].attivo=0;
			irrigazioneTable[id_irrigazione].current_circuito=0;
			irrigazioneTable[id_irrigazione].current_time=0;
			irrigazioneTable[id_irrigazione].num_circuiti++;
			irrigazioneCircuitiTable[idRow].tempo_on_cur=0;
			idRow++;
		}
		mysql_free_result(result);
	}
	else //id_irrigazione > 0
	{
		id_row_irrigazione=id_irrigazioneToId(id_irrigazione);
		if(circuito>0)
		{
			sprintf(query,"SELECT irrigazione_circuiti.durata,"
							"irrigazione_circuiti.validita,"
							"irrigazione_circuiti.id_digital_out "
						"FROM irrigazione_circuiti "
						"WHERE id_irrigazione=%d and circuito=%d",id_irrigazione,circuito);
			state = mysql_query(connection,query);	
			if( state != 0 )
			{
				printf("%s\n",mysql_error(connection));
				return(1);
			}
			result = mysql_store_result(connection);
		
			idRow=circuito_irrigazioneToId(id_irrigazione,circuito);
			if( ( row = mysql_fetch_row(result)) != NULL )
			{
				irrigazioneCircuitiTable[idRow].durata=atoi(row[0]);
				irrigazioneCircuitiTable[idRow].validita=atoi(row[1]);
				irrigazioneCircuitiTable[idRow].id_digital_out=atoi(row[2]);
				irrigazioneCircuitiTable[idRow].tempo_on_cur=0;
			}
			mysql_free_result(result);
		}
		else
		{
			sprintf(query,"SELECT irrigazione.ora_start,"
							"irrigazione.ripetitivita,"
							"irrigazione.tempo_off,"
							"count(irrigazione_circuiti.id_irrigazione),"
							"irrigazione.id_digital_out "
						"FROM irrigazione LEFT JOIN irrigazione_circuiti "
						"ON irrigazione.id=irrigazione_circuiti.id_irrigazione "
						"WHERE id=%d "
						"GROUP BY irrigazione_circuiti.id_irrigazione",id_irrigazione);
			state = mysql_query(connection,query);	
			if( state != 0 )
			{
				printf("%s\n",mysql_error(connection));
				return(1);
			}
			result = mysql_store_result(connection);
		
			idRow=id_row_irrigazione;
			if( ( row = mysql_fetch_row(result)) != NULL )
			{
				irrigazioneTable[idRow].ora_start=atoi(row[0]);
				irrigazioneTable[idRow].ripetitivita=atoi(row[1]);
				irrigazioneTable[idRow].tempo_off=atoi(row[2]);
				irrigazioneTable[idRow].num_circuiti=atoi(row[3]);
				irrigazioneTable[idRow].id_digital_out=atoi(row[4]);
				irrigazioneTable[idRow].attivo=0;
			}
			mysql_free_result(result);


			sprintf(query,"SELECT irrigazione_circuiti.circuito,"
							"irrigazione_circuiti.durata,"
							"irrigazione_circuiti.validita,"
							"irrigazione_circuiti.id_digital_out "
						"FROM irrigazione_circuiti "
						"WHERE id_irrigazione=%d "
						"ORDER BY irrigazione_circuiti.circuito",id_irrigazione);
			state = mysql_query(connection,query);	
			if( state != 0 )
			{
				printf("%s\n",mysql_error(connection));
				return(1);
			}
			result = mysql_store_result(connection);
		
			while( ( row = mysql_fetch_row(result)) != NULL )
			{
				idRow=circuito_irrigazioneToId(id_irrigazione,atoi(row[0]));
				irrigazioneCircuitiTable[idRow].durata=atoi(row[1]);
				irrigazioneCircuitiTable[idRow].validita=atoi(row[2]);
				irrigazioneCircuitiTable[idRow].id_digital_out=atoi(row[3]);
				irrigazioneCircuitiTable[idRow].tempo_on_cur=0;
			}
			mysql_free_result(result);
			
		}
		if(circuiti_attivi(id_row_irrigazione)==0)
			irrigazione_disattiva(id_row_irrigazione,irrigazioneTable[id_row_irrigazione].msg_arresto_manuale);
	}
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
		readingTable=(struct readingLine *)get_shared_memory_segment(READINGCHANNELS * sizeof(struct readingLine), &SHMREADINGID, "/dev/zero");
		if(!readingTable)
			die("readingTable - get_shared_memory_segment\n");
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


int loadDigitalOutTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;

	if(!onlyupdate)
	{
		digitalOutTable=(struct digitalOutLine *)get_shared_memory_segment(DIGITALOUTCHANNELS * sizeof(struct digitalOutLine), &SHMDIGITALOUTID, "/dev/zero");
		if(!digitalOutTable)
			die("digitalOutTable - get_shared_memory_segment\n");
	}
	initializeDigitalOutTable();
	state = mysql_query(connection, "SELECT digital_out.id_digital_out,"
							"digital_out.description,"
							"digital_out.device_num,"
							"digital_out.ch_num,"
							"digital_out.`default`,"
							"digital_out.po_delay,"
							"digital_out.on_value,"
							"digital_out.on_time,"
							"digital_out.pon_time,digital_out.poff_time,"
							"digital_out.printer,digital_out.msg_l,digital_out.msg_h,"
							"digital_out.sms,digital_out.msg_is_event "
						"FROM digital_out JOIN system ON digital_out.device_num=system.device_num "
						"WHERE system.removed=0 AND digital_out.ch_num!=0"
						" ORDER BY digital_out.device_num,digital_out.ch_num");

	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
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
			digitalOutTable[idRow].po_delay=atoi(row[5]);
			digitalOutTable[idRow].on_time=atoi(row[7]);
			digitalOutTable[idRow].pon_time=atoi(row[8]);
			digitalOutTable[idRow].poff_time=atoi(row[9]);
			digitalOutTable[idRow].printer=atoi(row[10]);
			if(row[11])
				safecpy(digitalOutTable[idRow].msg_l,row[11]);
			else
				safecpy(digitalOutTable[idRow].msg_l,"");
			if(row[12])
				safecpy(digitalOutTable[idRow].msg_h,row[12]);
			else
				safecpy(digitalOutTable[idRow].msg_h,"");
			digitalOutTable[idRow].req_value=digitalOutTable[idRow].def;
			if((digitalOutTable[idRow].po_delay > 0)
				&&(digitalOutTable[idRow].def==digitalOutTable[idRow].on_value))
			{
				digitalOutTable[idRow].value=1-digitalOutTable[idRow].req_value;
				digitalOutTable[idRow].start_time=time(NULL);
			}
			else
				digitalOutTable[idRow].value=digitalOutTable[idRow].def;
			digitalOutTable[idRow].on_value=atoi(row[6]);
			digitalOutTable[idRow].sms=atoi(row[13]);
			digitalOutTable[idRow].msg_is_event=atoi(row[14]);
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
	return(0);
}

int loadScenariPresenzeTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	int out;

	
	state = mysql_query(connection, "SELECT id_digital_out,attivo,ciclico,"
							"tempo_on,tempo_off,ora_ini,ora_fine "
							"FROM scenari_presenze "
							"ORDER BY id");
	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);
	SCENARIPRESENZECOUNT=mysql_num_rows(result);

	if(!onlyupdate)
	{
		scenariPresenzeTable=(struct scenariPresenzeLine *)get_shared_memory_segment(SCENARIPRESENZECOUNT * sizeof(struct scenariPresenzeLine), &SHMSCENARIPRESENZEID, "/dev/zero");
		if(!scenariPresenzeTable)
			die("scenariPresenzeTable - get_shared_memory_segment\n");

		scenariPresenzeAttivo=(char *)get_shared_memory_segment(2, &SHMSCENARIPRESENZEATTIVOID, "/dev/zero");
		if(!scenariPresenzeAttivo)
			die("scenariPresenzeAttivo - get_shared_memory_segment\n");

	}
	initializeScenariPresenzeTable();
	*scenariPresenzeAttivo=0;

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		scenariPresenzeTable[idRow].id_digital_out=atoi(row[0]);
		scenariPresenzeTable[idRow].attivo=atoi(row[1]);
		scenariPresenzeTable[idRow].ciclico=atoi(row[2]);
		scenariPresenzeTable[idRow].tempo_on=atoi(row[3]);
		scenariPresenzeTable[idRow].tempo_off=atoi(row[4]);
		scenariPresenzeTable[idRow].ora_ini=h2i(row[5]);
		scenariPresenzeTable[idRow].ora_fine=h2i(row[6]);
		idRow++;

	}
	mysql_free_result(result);
	return(0);
}


int loadScenariBgBnTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;
	int out;

	
	state = mysql_query(connection, "SELECT id_digital_out,attivo,ritardo,"
							"giorno "
							"FROM scenari_giorno_notte "
							"ORDER BY id");
	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);
	SCENARIBGBNCOUNT=mysql_num_rows(result);

	if(!onlyupdate)
	{
		scenariBgBnTable=(struct scenariBgBnLine *)get_shared_memory_segment(SCENARIBGBNCOUNT * sizeof(struct scenariPresenzeLine), &SHMSCENARIBGBNID, "/dev/zero");
		if(!scenariBgBnTable)
			die("scenariBgBnTable - get_shared_memory_segment\n");

		buonGiornoAttivo=(char *)get_shared_memory_segment(1, &SHMSCENARIBGATTIVOID, "/dev/zero");
		if(!buonGiornoAttivo)
			die("buonGiornoAttivo - get_shared_memory_segment\n");
		buonaNotteAttivo=(char *)get_shared_memory_segment(1, &SHMSCENARIBNATTIVOID, "/dev/zero");
		if(!buonaNotteAttivo)
			die("buonaNotteAttivo - get_shared_memory_segment\n");
	}
	initializeScenariBgBnTable();
	*buonGiornoAttivo=0;
	*buonaNotteAttivo=0;

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		scenariBgBnTable[idRow].id_digital_out=atoi(row[0]);
		scenariBgBnTable[idRow].attivo=atoi(row[1]);
		scenariBgBnTable[idRow].ritardo=atoi(row[2]);
		scenariBgBnTable[idRow].bg=atoi(row[3]);
		idRow++;
	}
	mysql_free_result(result);
	return(0);
}


int loadDigitalTable(bool onlyupdate)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	int i,k;
	int idRow;

	if(!onlyupdate)
	{
		digitalTable=(struct digitalLine *)get_shared_memory_segment(DIGITALCHANNELS * sizeof(struct digitalLine), &SHMDIGITALID, "/dev/zero");
		if(!digitalTable)
			die("digitalTable - get_shared_memory_segment\n");
		id_digital_pioggia=(int *)get_shared_memory_segment(sizeof(int), &SHMPIOGGIA, "/dev/zero");
		if(!id_digital_pioggia)
			die("id_digital_pioggia - get_shared_memory_segment\n");
	}

	initializeDigitalTable();
	
	state = mysql_query(connection, "SELECT digital.id_digital,digital.form_label,"
							"digital.description,digital.label,digital.sinottico,"
							"digital.device_num,digital.ch_num,digital.printer,"
							"digital.time_delay_on,digital.time_delay_off,"
							"digital.alarm_value,digital.msg,digital.enabled,"
							"digital.sms,digital.msg_is_event "
						"FROM digital JOIN system ON digital.device_num=system.device_num "
						"WHERE system.removed=0 AND digital.ch_num!=0"
						" ORDER BY digital.device_num, digital.ch_num");
	if( state != 0 )
	{
		printf("%s\n",mysql_error(connection));
		return(1);
	}
	result = mysql_store_result(connection);

	*id_digital_pioggia=-1;
		
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
			digitalTable[idRow].sms=atoi(row[13]);
			digitalTable[idRow].msg_is_event=atoi(row[14]);

			if(strstr(strtoupper(digitalTable[idRow].label),"PIOGGIA"))
				*id_digital_pioggia=idRow;
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
	return(0);
}

int setKnx(int id,double value)
{
	int i;
	i=id_knx_lineToId(id);

	if(i==-1)
		return -1;

	if(knxTable[i].data_type!='F')
		knxTable[i].req_value=(unsigned int)value;
	else
		knxTable[i].req_value=doubleToInt(value);
	return 1;
}

int setOutput(int id,int value)
{
	int i;
	i=id_digitalOutToId(id);
	
	if(i==-1)
		return -1;

	digitalOutTable[i].req_value=value;
	digitalOutTable[i].start_time=time(NULL);

	if(digitalOutTable[i].po_delay==0)
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
			digitalOutTable[idRow].start_time=time(NULL);
			digitalOutTable[idRow].req_value=digitalOutTable[idRow].def;
			if(digitalOutTable[idRow].po_delay>0)
				digitalOutTable[idRow].value=1-digitalOutTable[idRow].def;
			else
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
	sprintf(buffer,"#caricate effemeridi: alba %s - tramonto %s\n",dawn,sunset);
	write(fd,buffer,strlen(buffer));
}

char checkBuonGiornoAttivo()
{
	int i,j;
	time_t inizio;
	int out=0;

	if(*buonGiornoAttivo)
	{
		inizio=time(NULL);
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				if(inizio - digitalOutTable[j].start_time <= digitalOutTable[j].po_delay+digitalOutTable[j].on_time)
					out++;
				else
					digitalOutTable[j].value=digitalOutTable[j].req_value;
				if(digitalOutTable[j].value != digitalOutTable[j].req_value)
					out++;
			}
		}
	}
	return (out>0?1:0);
}

char checkBuonaNotteAttivo()
{
	int i,j;
	time_t inizio;
	int out=0;

	if(*buonaNotteAttivo)
	{
		inizio=time(NULL);
		for(i=0;i<SCENARIBGBNCOUNT;i++)
		{
			if((scenariBgBnTable[i].bg==0)&&(scenariBgBnTable[i].attivo))
			{
				j=id_digitalOutToId(scenariBgBnTable[i].id_digital_out);
				if(inizio - digitalOutTable[j].start_time <= digitalOutTable[j].po_delay+digitalOutTable[j].on_time)
					out++;
				else
					digitalOutTable[j].value=digitalOutTable[j].req_value;
				if(digitalOutTable[j].value != digitalOutTable[j].req_value)
					out++;
			}
		}
	}
	return (out>0?1:0);
}

void set_irrigazione(int fd,char *c)
{
	char query[255];
	char s1[127],s2[127];
	int state;
	char *pch;
	int id_irrigazione,circuito,ora_start,durata,validita,ripetitivita,tempo_off,conta;
	int id;

	id_irrigazione=0;
	conta=0;
	circuito=0;
	ora_start=0;
	durata=0;
	validita=0;
	ripetitivita=0;
	tempo_off=0;
	
	pch = strtok (c," ");
	while (pch != NULL)
	{
		switch(conta)
		{
			case 0:
				id_irrigazione=atoi(pch);
				break;
			case 1:
				circuito=atoi(pch);
				break;
			case 2:
				ora_start=atoi(pch);
				break;
			case 3:
				ripetitivita=atoi(pch);
				break;
			case 4:
				tempo_off=atoi(pch);
				break;
			case 5:
				durata=atoi(pch);
				break;
			case 6:
				validita=atoi(pch);
				break;
		}
		conta++;
		pch = strtok (NULL, " ");
	}

	if(circuito>0)
	{
		id=circuito_irrigazioneToId(id_irrigazione,circuito);
		if(id!=-1)
		{
			sprintf(s1,"%d %d %d\n",circuito,durata,validita);
			sprintf(query,"UPDATE irrigazione_circuiti SET durata=%d,validita=%d "
						"WHERE circuito=%d AND id_irrigazione=%d"
						,durata,validita,circuito,id_irrigazione);


			state = mysql_query(connection, query);
			if(state==0)
				loadIrrigazioneTables(id_irrigazione,circuito);
			sprintf(s2,"%d %d %d\n",irrigazioneCircuitiTable[id].circuito,
				irrigazioneCircuitiTable[id].durata,
				irrigazioneCircuitiTable[id].validita);
		}
	}
	else
	{
		id=id_irrigazioneToId(id_irrigazione);
		if(id!=-1)
		{
			sprintf(s1,"%d %d %d\n",ora_start,ripetitivita,tempo_off);
			sprintf(query,"UPDATE irrigazione SET ora_start=%d,ripetitivita=%d,tempo_off=%d "
							"WHERE id=%d"
						,ora_start,ripetitivita,tempo_off,id_irrigazione);

			state = mysql_query(connection, query);
			if(state==0)
				loadIrrigazioneTables(id_irrigazione,-1);
			sprintf(s2,"%d %d %d\n",irrigazioneTable[id].ora_start,
				irrigazioneTable[id].ripetitivita,
				irrigazioneTable[id].tempo_off);
		}

	}
};

void get_irrigazione(int fd,char *c)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	char query[511];
	char buffer[255];
	int state;

	strcpy(query,"SELECT irrigazione.id,"
				"irrigazione_circuiti.circuito,"
				"irrigazione.ora_start,"
				"irrigazione.ripetitivita,"
				"irrigazione.tempo_off,"
				"irrigazione_circuiti.durata,"
				"irrigazione_circuiti.validita"
				" FROM irrigazione LEFT JOIN irrigazione_circuiti"
				" ON irrigazione.id=irrigazione_circuiti.id_irrigazione");
	if(atoi(c)>0)
		sprintf(query,"%s WHERE irrigazione.id=%d",query,atoi(c));


	state = mysql_query(connection, query);

	if( state != 0 )
	{
		perror(mysql_error(connection));
		return;
	}

	result = mysql_store_result(connection);

	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		sprintf(buffer,"%d %d %d %d %d %d %d\n",
			atoi(row[0]),atoi(row[1]),atoi(row[2]),atoi(row[3]),
			atoi(row[4]),atoi(row[5]),atoi(row[6]));
		if (send(fd, buffer,strlen(buffer), 0) == -1)
			perror("send");
	}
	mysql_free_result(result);
	strcpy(buffer,TERMINATOR);
	if (send(fd, buffer,strlen(buffer), 0) == -1)
		perror("send");
	return;
};

void get_irrigazione_rt(int fd,char *c)
{
	char buffer[255];
	int i,id_irrigazione,id_circuito;

	if(atoi(c)>0)
	{
		id_irrigazione=id_irrigazioneToId(atoi(c));
		if(id_irrigazione!=-1)
		{
			sprintf(buffer,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
				"id_irr","ora_st","ripet",
				"num_cir","day","attivo","started",
				"cur_c","cur_t","count");
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				perror("send");
			sprintf(buffer,"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
				irrigazioneTable[id_irrigazione].id_irrigazione,
				irrigazioneTable[id_irrigazione].ora_start,
				irrigazioneTable[id_irrigazione].ripetitivita,
				irrigazioneTable[id_irrigazione].num_circuiti,
				irrigazioneTable[id_irrigazione].day,
				irrigazioneTable[id_irrigazione].attivo,
				irrigazioneTable[id_irrigazione].ora_started,
				irrigazioneTable[id_irrigazione].current_circuito,
				irrigazioneTable[id_irrigazione].current_time,
				irrigazioneTable[id_irrigazione].count);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				perror("send");

			sprintf(buffer,"%s\t%s\t%s\n",
				"circ","durata","valid");
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				perror("send");

			for(i=0;i<irrigazioneTable[id_irrigazione].num_circuiti;i++)
			{
				id_circuito=circuito_irrigazioneToId(irrigazioneTable[id_irrigazione].id_irrigazione,i+1);
				if(id_circuito!=-1)
				{
					sprintf(buffer,"%d\t%d\t%d\n",
						irrigazioneCircuitiTable[id_circuito].circuito,
						irrigazioneCircuitiTable[id_circuito].durata,
						irrigazioneCircuitiTable[id_circuito].validita);
					if (send(fd, buffer,strlen(buffer), 0) == -1)
						perror("send");
				}
			}
		}
			
	}
	else
	{
		for(i=0;i<IRRIGAZIONESYSTEMS;i++)
		{
			sprintf(buffer,"%d %d %d %d %d %d %d %d %d %d\n",
				irrigazioneTable[i].id_irrigazione,
				irrigazioneTable[i].ora_start,
				irrigazioneTable[i].ripetitivita,
				irrigazioneTable[i].num_circuiti,
				irrigazioneTable[i].day,
				irrigazioneTable[i].attivo,
				irrigazioneTable[i].ora_started,
				irrigazioneTable[i].current_circuito,
				irrigazioneTable[i].current_time,
				irrigazioneTable[i].count);
			if (send(fd, buffer,strlen(buffer), 0) == -1)
				perror("send");
		}
	}
	strcpy(buffer,TERMINATOR);
	if (send(fd, buffer,strlen(buffer), 0) == -1)
		perror("send");
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

