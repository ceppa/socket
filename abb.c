#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <time.h>

unsigned int getDateBytes();
unsigned int getDateBytesFromDate(int dd,int mm,int yy);
unsigned char doCheckSum(unsigned char *buf,int from, int to);
void setDate(unsigned int date,int sockfd);
int pmRecvReply(int sockfd,unsigned char *buf,int len,int timeout);

int main(int argc, char *argv[])
{
	struct hostent *he;
	struct sockaddr_in their_addr;
	int sockfd;
	int out;
	int attempt=0;
	char address[16];
	int port;
	unsigned char message[20];
	struct timeval tv;


	int recvLen;
	unsigned char *recvBuf;
	int received;
	int numbytes;
	register int i;
	unsigned int datebytes;

	datebytes=getDateBytes();
	printf("datebytes: %x\n",datebytes);
	port=6021;
	strcpy(address,"192.168.1.195");

	if ((he=gethostbyname(address)) == NULL)
		printf("error gethostbyname\n");
	else
	{
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			printf("error creating socket\n");
		else
		{
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,  sizeof tv))
				perror("setsockopt");

			their_addr.sin_family = AF_INET;
			their_addr.sin_port = htons(port);
			their_addr.sin_addr = *((struct in_addr *)he->h_addr);
			bzero(&(their_addr.sin_zero), 8);
			if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
			{
				close(sockfd);
				out=-1;
			}
			else 
			{
				recvLen=255;
				recvBuf=(unsigned char *)malloc(recvLen);
				received=0;

				printf("connected\n");

				message[0]=0x10;
				message[1]=0x40;	//snd_nke
				message[2]=0xFE;	//address
				message[3]=(message[1]+message[2])&0xff;	//checksum
				message[4]=0x16;	//stop
				printf("sending NKE\n");
				if (!send(sockfd,message,5,0)) 
				{
					printf("error in sending\n");
					exit(-1);
				}
				received=0;
				printf("receiving\n");
				if ((numbytes=recv(sockfd, &recvBuf[received], recvLen-received, 0)) > 0)
					received+=numbytes;

				if(received<=0)
				{
					printf("no way\n");
					exit(-1);
				}
				else
					printf("received %x\n",recvBuf[0]);

				message[0]=0x10;
				message[1]=0x40;
				message[2]=0xFE;
				message[3]=0x3E;
				message[4]=0x16;

				for(i=0;i<5;i++)
					printf("%02x ",message[i]);
				printf("\n");


				if (!send(sockfd,message,5,0)) 
				{
					printf("error in sending\n");
					exit(-1);
				}


				received=pmRecvReply(sockfd,recvBuf,255,2);
				if(received)
				{
					for(i=0;i<received;i++)
						printf("%02x ",recvBuf[i]);
					printf("\n");
				}

				free(recvBuf);

				close(sockfd);
			}
		}
	}
	printf("\n");
}


unsigned int getDateBytes()
{
	struct timeval tv;
	struct tm *tm;
	unsigned char dd;
	unsigned char mm;
	unsigned char yy;
	unsigned int out;
	register int i;

	int recvLen;
	unsigned char *recvBuf;
	int received;
	int numbytes;

 	gettimeofday(&tv, NULL); 
	tm = localtime(&tv.tv_sec);

	dd=tm->tm_mday;
	mm=tm->tm_mon+1;
	yy=tm->tm_year%100;

/*	dd=23;
	mm=9;
	yy=6;
*/
	
	out=getDateBytesFromDate(dd,mm,yy);
	
	for(i=15;i>=0;i--)
	{
		if(i%4==3)
			printf(" ");
		if(out & (1<<i))
			printf("1");
		else
			printf("0");
	}

	printf("\n%x %x\n",out>>8,out&0xff);
	return out;
}

unsigned int getDateBytesFromDate(int dd,int mm,int yy)
{
	unsigned int out=0;

	out+=(dd%0x1f);
	out+=((mm%0x0f)<<8);
	out+=((yy%0x07)<<5);
	out+=((yy>>3)<<12);
	return out;
}

unsigned char doCheckSum(unsigned char *buf,int from, int to)
{
	register int i;

	unsigned char out;
	unsigned long temp=0;
	
	for(i=from;i<=to;i++)
		temp+=buf[i];
	out=temp&0xff;
}

void setDate(unsigned int datebytes,int sockfd)
{
	int received=0;
	unsigned char message[20];
	unsigned char *recvBuf;
	int numbytes;


	int recvLen=255;

	recvBuf=(unsigned char *)malloc(recvLen);

	message[0]=0x68;
	message[1]=0x07;
	message[2]=0x07;
	message[3]=0x68;
	message[4]=0x73;
	message[5]=0xfe;
	message[6]=0x51;
	message[7]=0x02;
	message[8]=0x6c;
	message[9]=datebytes&0xff;
	message[10]=datebytes>>8;
	message[11]=doCheckSum(message,4,10);
	message[12]=0x16;

	
	printf("sending setdate\n");

	if (!send(sockfd,message,13,0)) 
	{
		printf("error in sending\n");
		exit(-1);
	}
	received=0;
	printf("receiving\n");
	if ((numbytes=recv(sockfd, &recvBuf[received], recvLen-received, 0)) > 0)
		received+=numbytes;

	if(received<=0)
	{
		printf("no way\n");
		exit(-1);
	}
	else
		printf("received %x\n",recvBuf[0]);
	free(recvBuf);
}


int pmRecvReply(int sockfd,unsigned char *buf,int len,int timeout)
{
	time_t inizio;
	unsigned char l;

	int numbytes,received;
	received=0;
	inizio=time(NULL);
	do
	{
		if ((numbytes=recv(sockfd, &buf[received], len-received, 0)) > 0)
			received+=numbytes;
		if(numbytes==0)
			usleep(1000);
	}
	while((received<3)&&(time(NULL)-inizio<timeout));	

	if(received<3)
		return 0;
	if((buf[1]!=buf[2])||(buf[1]<15))
		return 0;
	l=4+buf[1];

	inizio=time(NULL);
	while(received<l)
	{
		if ((numbytes=recv(sockfd, &buf[received], len-received, 0)) > 0)
			received+=numbytes;
		if(numbytes==0)
			usleep(1000);
	}
	if(received<l)
		return 0;
	return received;
}
