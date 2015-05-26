#include "util.h"

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


char *trim(char *in)
{
	register int i,j;

	for(i=0;i<strlen(in);i++)
		if((in[i]!=0x20)
				&&(in[i]!=0x0d)
					&&(in[i]!=0x0a))
			break;
	for(j=strlen(in)-1;j>0;j--)
		if((in[j]!=0x20)
				&&(in[j]!=0x0d)
					&&(in[j]!=0x0a))
			break;

	memcpy(in,&in[i],j-i+1);
	in[j-i+1]='\0';

	return in;
}


double intToDouble(unsigned int in)
{
	int m;
	unsigned char e;
	int l=sizeof(int);
	register int i;
	unsigned char b;

	e=((in>>11)&0x0f);
	memcpy(&m,&in,l);
	m&=(0x07ff);
	b=in>>15;
	for(i=11;i<l*8;i++)
		m|=(b<<i);

	return (0.01*m)*(1<<e);
}

unsigned int doubleToInt(double in)
{
	unsigned char e;
	int m;
	unsigned int out=0x7fff;
	if((in>=-671088.64)&&(in<=670760.96))
	{
		for(e=0;e<16;e++)
		{
			m=(int)((in*100)/(1<<e));
			if((m>=-2048)&&(m<=2047))
				break;
		}
		if(e<16)
		{
			out=m&(0x87ff);
			out|=(e<<11);
		}
	}
	return out;
}


int receive(int sockfd,char *buf,int len,char *stringok,int timeout)
{
	time_t inizio;
	int received=0;
	int out=-1;
	int numbytes;

	memset(buf,0,len);
	inizio=time(NULL);
	while(time(NULL)-inizio<timeout)
	{
		numbytes=recv(sockfd, &buf[received], len-received, MSG_DONTWAIT);
		if(numbytes>0)
			received+=numbytes;
		if(strstr(buf,stringok))
			return numbytes;
		usleep(1000);
	}
	return out;
}

int sendsms(int sockfd,char *number,char *message)
{
	int numbytes,received;
	char buf[255];
	int len=255;

	numbytes=sprintf(buf,"AT+CMGF=1%c",13);
	if (!send(sockfd,buf,numbytes,0)) 
	{
		my_printf("error in sending\n");
		return -1;
	}
	else 
		my_printf("sent: %s\n",buf);

	received=0;

	my_printf("receiving\n");

	received=receive(sockfd,buf,len,"OK",2);

	if(received<0)
		return -1;

	my_printf("received: %s\n",buf);
	numbytes=sprintf(buf,"AT+CMGS=\"%s\"%c",number,13);

	if (!send(sockfd,buf,numbytes,0)) 
	{
		my_printf("error in sending\n");
		return -1;
	}
	my_printf("sent: %s\n",buf);

	received=receive(sockfd,buf,len,">",2);

	if(received<0)
		return -1;

	my_printf("received: %s\n",buf);
	numbytes=sprintf(buf,"%s %c",message,26);


	if (!send(sockfd,buf,numbytes,0)) 
	{
		my_printf("error in sending\n");
		return -1;
	}
	my_printf("sent: %s\n",buf);

	received=receive(sockfd,buf,len,message,2);

	if(received<0)
		return -1;
	my_printf("received: %s\n",buf);
	return 1;
}


int socketConnect(char *address,int port)
{
	struct hostent *he;
	struct sockaddr_in their_addr;
	struct timeval tv;
	int sockfd;
	fd_set myset; 
	socklen_t lon;
	int valopt;

	if ((he=gethostbyname(address)) == NULL)
	{
		my_perror("gethostbyname");
		return -1;
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		my_perror("socket");
		return -1;
	}
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv))
		my_perror("setsockopt");

	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,  sizeof tv))
		my_perror("setsockopt");

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);
	if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
	{
		if (errno == EINPROGRESS) 
		{ 
			FD_ZERO(&myset); 
			FD_SET(sockfd, &myset); 
			if (select(sockfd+1, NULL, &myset, NULL, &tv) > 0) 
			{ 
				lon = sizeof(int); 
				getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon); 
				if (valopt) 
				{ 
					my_perror("Error in connection()"); 
					close(sockfd);
					return -1;
				} 
			} 
			else 
			{ 
				my_perror("Timeout or error()"); 
				close(sockfd);
				return -1;
			}
		}
		else 
		{ 
			my_perror("Error connecting"); 
			close(sockfd);
			return -1;
		} 
	}
	return sockfd;
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

void *get_shared_memory_segment(const int size, int *shmid,
		const char *filename)
/*--------------------------------------------
 * alloca memoria condivisa (systems,analogTable,digitalTable)
 * -----------------------------------------*/
{
	key_t key;
	int err = 0;
	int n;
	int out;
	int conta=0;
//	RNDSEED=0;

	void *temp_shm_pointer;
//	n=rand() % 255;

/*
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
			out=(*shmid) = shmget(key, size, IPC_CREAT | IPC_EXCL | 0600);
//			out=(*shmid) = shmget(key, size, IPC_CREAT | 0600);
 
			if (out != -1)
				break;
		}
	}
	my_printf("RNDSEED: %d - SHMID: %d - pid: %d\n",RNDSEED,*shmid,getpid());
	if(n==256)
		die("shmget - reached 256\n");
*/
	out=(*shmid) = shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL | 0600);
	if(out==-1)
	{
		my_perror("shmget ");
		die("shmget - out=-1\n");
	}

	my_printf("SHMID: %d\n",*shmid);

	temp_shm_pointer = (void *)shmat((*shmid), 0, 0);
	if (temp_shm_pointer == (void *)(-1))
		die("shmat\n");
	return temp_shm_pointer;
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

void readEffemeridi(MYSQL *connection)
{
	int i,j;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int state;
	
	//READ EFFEMERIDI (needs mysql);

	for(i=0;i<12;i++)
		for(j=0;j<31;j++)
		{
			effemeridiTable[i][j].dawn=-1;
			effemeridiTable[i][j].sunset=-1;
		}

	state = mysql_query(connection, "SELECT month,day,dawn,sunset FROM effemeridi ORDER BY month,day");
	if( state != 0 )
	{
		my_printf("%s\n",mysql_error(connection));
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
}

void my_printf(const char *format, ...) 
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	fflush(stdout);
}

void my_perror(const char *s)
{
	perror(s);
	fflush(stderr);
}
