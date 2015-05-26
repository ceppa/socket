#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <mysql/mysql.h>

#define BACKLOG 10 
#define MYPORT 3480
#define MAXDATASIZE 5120
#define MAXCONNECTIONS 20
#define TERMINATOR "FiNe"
#define MAXDATASIZE 5120
#define BACKLOG 10    
                       

struct shmid_ds shmid_struct;	

typedef unsigned char bool;

struct analogLine
{
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
	
	bool currently_high;
	bool currently_low;
	
	char curve[255];
	bool no_linear;
	bool printer;
	char msg_l[80];
	char msg_h[80];
	bool enabled;

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
	char msg[80];
	bool enabled;

	int value;
	int value1;
	int value2;
	int value3;
	int value4;
};
struct digitalLine *digitalTable;


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
};
struct system *systems;

int NUMSYSTEMS;
int ***deviceToid; //type,num,ch
int ANALOGCHANNELS=0;
int DIGITALCHANNELS=0;

int SHMSYSTEMSID;
int SHMANALOGID;
int SHMDIGITALID;

int serverPids[MAXCONNECTIONS];
int mypid;
int doConnect(int systemId);
int checkSons(int *pidArray);
int operate(int sockfd,int deviceId, int timeout);
void killSons(int *pidArray);
void *get_shared_memory_segment(const int , int *shmid, const char *); 
void doServer();
void performAction(int nwfd,char *buf);
int systemNumToId(int systemNum,int totale);
void initializeAnalogTable();
void initializeDigitalTable();
void systemDisconnected(int systemId);
double getCurveValue(char* values,double value);
int getSystemId(int port);
void safecpy(char *dest,char *src);

void sigchld_handler(int s)
{
	int i;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	if(getppid()==mypid)
	{
//		printf("child dying\n");
		for(i=0;i<MAXCONNECTIONS;i++)
			if((serverPids[i]!=-1)&&(kill(serverPids[i],0)==-1))
				serverPids[i]=-1;
	}
}


void termination_handler (int signum)
{
	int i;
	
	if(getppid()==mypid)
		for(i=0;i<MAXCONNECTIONS;i++)
			if(serverPids[i]!=-1)
				kill(serverPids[i],SIGTERM);
	
	if(shmdt(systems))
		perror("shmdt");
	shmctl(SHMSYSTEMSID, IPC_RMID, &shmid_struct);
	if(shmdt(analogTable))
		perror("shmdt");
	shmctl(SHMANALOGID, IPC_RMID, &shmid_struct);
	if(shmdt(digitalTable))
		perror("shmdt");
	shmctl(SHMDIGITALID, IPC_RMID, &shmid_struct);

	exit(1);
}


int main(int argc, char *argv[])
{
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
	char temp[10];
	unsigned char *sendBuf;
	int idRow;
	int serverPid;
	int values[8];

	int sockfd, new_fd;
	struct sockaddr_in my_addr;  
	struct sockaddr_in their_addr;
	int sin_size, gelesen, nbytes,curbytes;
	char ret[MAXDATASIZE];
	char comm[MAXDATASIZE];

	int MYPORTA=3491;
	unsigned char statusbuf[18];
	int systemid;

	int bene;
	int male;

    struct sigaction sa;
	
	int *pid;	

	MYSQL_RES *result;
	MYSQL_ROW row;
	MYSQL *connection, mysql;
	int state;

	if (signal (SIGINT, termination_handler) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	if (signal (SIGHUP, termination_handler) == SIG_IGN)
		signal (SIGHUP, SIG_IGN);
	if (signal (SIGTERM, termination_handler) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);


	mypid=getpid();
	
	gettimeofday(&tim, NULL);
   	t1=tim.tv_sec+(tim.tv_usec/1000000.0);
//MYSQL ROBA    
	mysql_init(&mysql);
	connection = mysql_real_connect(&mysql,"localhost","root", "minair","socket", 0, NULL,0);
	if( connection == NULL ) 
	{
		printf(mysql_error(&mysql));
		return 1;
	}

	state = mysql_query(connection, "SELECT system.device_type,system.device_num,system.tcp_ip,system.port,device_type.in_num_bytes,device_type.in_ch_an,device_type.in_ch_d,device_type.in_eos,device_type.out_num_bytes,device_type.out_ch_d,device_type.out_ch_per_byte,device_type.out_sos,device_type.out_eos FROM system LEFT JOIN device_type ON system.device_type=device_type.type WHERE system.removed=0");
    
    if( state != 0 ) 
    {
        printf(mysql_error(connection));
        return 1;
    }
    result = mysql_store_result(connection);

	NUMSYSTEMS=mysql_num_rows(result);
	pid=(int *)malloc(NUMSYSTEMS * sizeof(int));
	systems=(struct system *)get_shared_memory_segment(NUMSYSTEMS * sizeof(struct system), &SHMSYSTEMSID, argv[0]);
	
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

    	ANALOGCHANNELS+=systems[i].in_ch_an;
    	DIGITALCHANNELS+=systems[i].in_ch_d;
    	i++;
    }
    mysql_free_result(result);


	analogTable=(struct analogLine *)get_shared_memory_segment(ANALOGCHANNELS * sizeof(struct analogLine), &SHMANALOGID, argv[0]);
	initializeAnalogTable();

	digitalTable=(struct digitalLine *)get_shared_memory_segment(DIGITALCHANNELS * sizeof(struct digitalLine), &SHMDIGITALID, argv[0]);
	initializeDigitalTable();


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
						"WHERE system.removed=0");
	if( state != 0 ) 
		printf(mysql_error(connection));
	result = mysql_store_result(connection);

	deviceToid=(int ***)malloc(2*sizeof(int **));
	deviceToid[0]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //analog
	for(i=0;i<NUMSYSTEMS;i++)
	{
		deviceToid[0][i]=(int *)malloc(systems[i].in_ch_an*sizeof(int));
		for(k=0;k<systems[i].in_ch_an;k++)
			deviceToid[0][i][k]=-1;
	}

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		i=systemNumToId(atoi(row[5]),NUMSYSTEMS);
		if((i!=-1) && (atoi(row[6])<=systems[i].in_ch_an))
		{
			deviceToid[0][i][atoi(row[6]) - 1]=idRow;

			if(row[1])
				safecpy(analogTable[idRow].form_label,row[1]);
			else
				strcpy(analogTable[idRow].form_label,"");
			if(row[2])
				safecpy(analogTable[idRow].description,row[2]);
			else
				safecpy(analogTable[idRow].description,"");
			if(row[3])
				safecpy(analogTable[idRow].label,row[3]);
			else
				safecpy(analogTable[idRow].label,"");
			if(row[4])
				safecpy(analogTable[idRow].sinottico,row[4]);
			else
				safecpy(analogTable[idRow].sinottico,"");
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
	for(i=0;i<NUMSYSTEMS;i++)
		for(k=0;k<systems[i].in_ch_an;k++)
			if(deviceToid[0][i][k]==-1)
			{
				deviceToid[0][i][k]=idRow;
				idRow++;
			}


	//digital
	state = mysql_query(connection, "SELECT digital.id_digital,digital.form_label,"
							"digital.description,digital.label,digital.sinottico,"
							"digital.device_num,digital.ch_num,digital.printer,"
							"digital.time_delay_on,digital.time_delay_off,"
							"digital.alarm_value,digital.msg,digital.enabled "
						"FROM digital JOIN system ON digital.device_num=system.device_num "
						"WHERE system.removed=0");
	if( state != 0 )
		printf(mysql_error(connection));
	result = mysql_store_result(connection);
	deviceToid[1]=(int **)malloc(NUMSYSTEMS * sizeof(int *)); //digital
	for(i=0;i<NUMSYSTEMS;i++)
	{
		deviceToid[1][i]=(int *)malloc(systems[i].in_ch_d*sizeof(int));
		for(k=0;k<systems[i].in_ch_d;k++)
			deviceToid[1][i][k]=-1;
	}

	idRow=0;
	while( ( row = mysql_fetch_row(result)) != NULL )
	{
		i=systemNumToId(atoi(row[5]),NUMSYSTEMS);
		if((i!=-1) && (atoi(row[6])<=systems[i].in_ch_d))
		{
			deviceToid[1][i][atoi(row[6]) - 1]=idRow;

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
	for(i=0;i<NUMSYSTEMS;i++)
		for(k=0;k<systems[i].in_ch_d;k++)
			if(deviceToid[1][i][k]==-1)
			{
				deviceToid[1][i][k]=idRow;
				idRow++;
			}

    mysql_close(connection);
	gettimeofday(&tim, NULL);


	if(argc>1)
		MYPORTA=atoi(argv[1]);
	

	systemid=getSystemId(MYPORTA);
	

		for(j=0;j<systems[systemid].in_ch_an;j++)
				printf("%d %d %s %f\n",analogTable[deviceToid[0][systemid][j]].device_num,
					analogTable[deviceToid[0][systemid][j]].ch_num,
					analogTable[deviceToid[0][systemid][j]].description,
					analogTable[deviceToid[0][systemid][j]].range_full);

	for(i=0;i<NUMSYSTEMS;i++)
		for(j=0;j<systems[i].in_ch_d;j++)
			printf("%d %d %s\n",digitalTable[deviceToid[1][i][j]].device_num,
					digitalTable[deviceToid[1][i][j]].ch_num,
					digitalTable[deviceToid[1][i][j]].description);

	printf("%.6f seconds to initialise\n",(tim.tv_sec+(tim.tv_usec/1000000.0))-t1);


//FINE MYSQL ROBA

	

  

	printf("Server \n");

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		perror("socket");
		exit(1);
	}

	my_addr.sin_family = AF_INET;        
	my_addr.sin_port = htons(MYPORTA);    
	my_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(my_addr.sin_zero), 8);        


	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) 
	{
		perror("bind");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) 
	{
		perror("listen");
		exit(1);
	}
	printf("Listen on Port : %d\n",MYPORTA);

	sin_size = sizeof(struct sockaddr_in);
	
	for(i=0;i<8;i++)
		values[i]=analogTable[deviceToid[0][systemid][i]].scale_full;

	while(1)
	{
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
			perror("accept");

		printf("Connect!! %s is on the Server!\n",inet_ntoa(their_addr.sin_addr));

	
		curbytes=0;
		do
		{
	       	nbytes=0;
       		do
       		{
	           	if ((curbytes=recv(new_fd,buf,MAXDATASIZE,0)) != -1)
	        		nbytes+=curbytes;
        	}
        	while((curbytes>0)&&(nbytes<12));
	
			if(curbytes>0)
			{
				for(i=0;i<12;i++)
					printf("%02x ",buf[i]);
				printf("\n");
				statusbuf[17]=0x55;


				for(i=0;i<8;i++)
				{
					statusbuf[2*i]=values[i]>>8;
					statusbuf[2*i+1]=values[i]&0xFF;
					values[i]=(int)((1-(double)(random())/(10*(double)RAND_MAX))*((analogTable[deviceToid[0][systemid][i]].scale_full-analogTable[deviceToid[0][systemid][i]].scale_zero)/2+
								analogTable[deviceToid[0][systemid][i]].scale_zero+
							(int)((analogTable[deviceToid[0][systemid][i]].scale_full-analogTable[deviceToid[0][systemid][i]].scale_zero)*sin(((double)time(NULL))/40000)/2)));

				}
				statusbuf[16]=(random() & 0x03);

				if (send(new_fd, statusbuf, 18, 0) == -1)
					perror("send");
			}
			usleep(100000);
		}
		while(curbytes>0);
		close(new_fd);
		
	}
}

int checkSons(int *pidArray)
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
}
	

void killSons(int *pidArray)
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
{
	struct hostent *he;
	struct sockaddr_in their_addr;
	int sockfd;
	int out;
	do
	{
		out=0;
		if ((he=gethostbyname(systems[systemId].address)) == NULL)
			out=-1;
		else
		{
			if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
				out=-1;
			else
			{
				their_addr.sin_family = AF_INET;          
				their_addr.sin_port = htons(systems[systemId].port);
				their_addr.sin_addr = *((struct in_addr *)he->h_addr);
				bzero(&(their_addr.sin_zero), 8);          
				if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
					out=-1;
			}
		}
		if(out==-1)
		{
			systemDisconnected(systemId);
			sleep(5);
		}
	}
	while(out!=0);
	
	return sockfd;
}



void *get_shared_memory_segment(const int size, int *shmid, const char *filename)
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


int systemNumToId(int systemNum, int totale)
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
		analogTable[i].time_delay_high=0;
		analogTable[i].time_delay_low=0;
		analogTable[i].time_delay_high_cur=0;
		analogTable[i].time_delay_low_cur=0;
		analogTable[i].time_delay_high_off_cur=0;
		analogTable[i].time_delay_low_off_cur=0;
		analogTable[i].currently_high=0;
		analogTable[i].currently_low=0;
		strcpy(analogTable[i].curve,"");
		analogTable[i].no_linear=0;
		analogTable[i].printer=0;
		strcpy(analogTable[i].msg_l,"");
		strcpy(analogTable[i].msg_h,"");
		analogTable[i].enabled=0;

		analogTable[i].value=-1;
		analogTable[i].value1=-1;
		analogTable[i].value2=-1;
		analogTable[i].value3=-1;
		analogTable[i].value4=-1;
	}
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
		
		digitalTable[i].value=-1;
		digitalTable[i].value1=-1;
		digitalTable[i].value2=-1;
		digitalTable[i].value3=-1;
		digitalTable[i].value4=-1;
	}
}

void safecpy(char *dest,char *src)
{
	char *c;
	
	strcpy(dest,src);
	if(c=strstr(dest,TERMINATOR))
		*c='f';
}


void systemDisconnected(int systemId)
{
	int i,j,id;

	for(j=0;j<systems[systemId].in_ch_an;j++)
	{
		id=deviceToid[0][systemId][j];
		analogTable[id].value4=analogTable[id].value3;
		analogTable[id].value3=analogTable[id].value2;
		analogTable[id].value2=analogTable[id].value1;
		analogTable[id].value1=analogTable[id].value;
		analogTable[id].value=-1;
		analogTable[id].value_eng4=analogTable[id].value_eng3;
		analogTable[id].value_eng3=analogTable[id].value_eng2;
		analogTable[id].value_eng2=analogTable[id].value_eng1;
		analogTable[id].value_eng1=analogTable[id].value_eng;
		analogTable[id].value_eng=-1;

	}
	for(j=0;j<systems[systemId].in_ch_d;j++)
	{
		id=deviceToid[1][systemId][j];
		digitalTable[id].value4=digitalTable[id].value3;
		digitalTable[id].value3=digitalTable[id].value2;
		digitalTable[id].value2=digitalTable[id].value1;
		digitalTable[id].value1=digitalTable[id].value;
		digitalTable[id].value=-1;
	}
}


double getCurveValue(char* values,double value)
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


int getSystemId(int port)
{
	int i=0;
	while((i<NUMSYSTEMS) && (systems[i].port!=port))
		i++;
	return i;
}
