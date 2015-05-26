#include "util.h"

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

int HextoDec(char *hex)
{
    if (*hex==0) 
		return 0;
    return HextoDec(hex-1)*16 + xtod(*hex) ; 
}
 

unsigned char knxValToInt(char *in)
{
	register int i;
	int out=0;
	in=trim(in);

	switch(in[0])
	{
		case '$':
			in[0]='\0';
			in+=strlen(&in[1]);
			out=HextoDec(in);
			break;
		case '%':
			for(i=1;i<strlen(in);i++)
				out=((out<<1)+(in[i]=='1'?1:0));
			break;
		default:
			out=atoi(in);
			break;
	}
	return out;
}

int parseKnxOutput(char *string,char *source,char *dest,unsigned int *data)
{
	int l;
	char *c,*d,*e,*f,*g;
	char lb[4];
	int out=-1;
	strcpy(source,"");
	strcpy(dest,"");
	*data=0;
	register int i;
	unsigned char v;
	
	l=strlen(string);
	if(c=strchr(string,'('))
	{
		c++;
		if(d=strchr(c,')'))
		{
			if(e=strchr(c,' '))
			{
				memcpy(source,c,e-c);
				source[e-c]='\0';
				e++;
				if(f=strchr(e,' '))
				{
					memcpy(dest,e,f-e);
					dest[f-e]='\0';
					f++;
					memcpy(lb,f,d-f);
					lb[d-f]='\0';
					l=knxValToInt(lb);
					for(d++;(*d)!=0;d++)
						if(*d!=' ')
							break;

					for(i=0;i<(l-1);i++)
					{
						if(c=strchr(d,' '))
						{
							*c='\0';

							v=knxValToInt(d);
							(*data)=((*data)<<8)+v;
							d=c+1;
						}
						else
							return(-1);
					}
					v=knxValToInt(d);
					(*data)=((*data)<<8)+v;
					out=1;
				}
			}
		}
	}
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
		printf("error in sending\n");
		return -1;
	}
	else 
		printf("sent: %s\n",buf);

	received=0;

	printf("receiving\n");

	received=receive(sockfd,buf,len,"OK",2);

	if(received<0)
		return -1;

	printf("received: %s\n",buf);
	numbytes=sprintf(buf,"AT+CMGS=\"%s\"%c",number,13);

	if (!send(sockfd,buf,numbytes,0)) 
	{
		printf("error in sending\n");
		return -1;
	}
	printf("sent: %s\n",buf);

	received=receive(sockfd,buf,len,">",2);

	if(received<0)
		return -1;

	printf("received: %s\n",buf);
	numbytes=sprintf(buf,"%s %c",message,26);


	if (!send(sockfd,buf,numbytes,0)) 
	{
		printf("error in sending\n");
		return -1;
	}
	printf("sent: %s\n",buf);

	received=receive(sockfd,buf,len,message,2);

	if(received<0)
		return -1;
	printf("received: %s\n",buf);
	return 1;
}


int socketConnect(char *address,int port)
{
	struct hostent *he;
	struct sockaddr_in their_addr;
	struct timeval tv;
	int sockfd;

	if ((he=gethostbyname(address)) == NULL)
	{
		perror("gethostbyname");
		return -1;
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		return -1;
	}
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv))
		perror("setsockopt");

	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,  sizeof tv))
		perror("setsockopt");

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);
	if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
		return -1;
	return sockfd;
}
