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

	port=6021;
	strcpy(address,"192.168.1.10");

	if ((he=gethostbyname(address)) == NULL)
		out=-1;
	else
	{
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			out=-1;
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
				printf("connected\n");

/*				message[0]=0x10;
				message[1]=0x40;	//snd_nke
				message[2]=0xFE;	//address
				message[3]=(message[1]+message[2])%0xff;	//checksum
				message[4]=0x16;	//stop
*/

				message[0]=0x68;
				message[1]=0x0a;	
				message[2]=0x0a;	
				message[3]=0x68;	
				message[4]=0x73;	
				message[5]=0xfe;
				message[6]=0x51;	
				message[7]=0x02;	
				message[8]=0xec;	
				message[9]=0xff;	
				message[10]=0xf9;
				message[11]=0x10;	
				message[12]=0xc5;	
				message[13]=0x04;	
				message[14]=doCheckSum(message,4,13);
				message[15]=0x16;	
				if (!send(sockfd,message,16,0)) 
				{
					printf("error in sending\n");
					exit(-1);
				}

				recvLen=255;
				recvBuf=(unsigned char *)malloc(recvLen);
				received=0;

				printf("receiving\n");
				if ((numbytes=recv(sockfd, &recvBuf[received], recvLen-received, 0)) > 0)
					received+=numbytes;

				if((received)&&(recvBuf[0]==0xe5))
				{
					printf("%d\n",recvBuf[0]);
					message[0]=0x10;
					message[1]=0x7b;
					message[2]=0xfe;
					message[3]=doCheckSum(message,1,2);
					message[4]=0x16;


					if (!send(sockfd,message,16,0)) 
					{
						printf("error in sending\n");
						exit(-1);
					}

					if ((numbytes=recv(sockfd, &recvBuf[received], recvLen-received, 0)) > 0)
					{
						for(i=0;i<numbytes;i++)
						{
							printf("%02x ",recvBuf[i]);
							if(i % 8==0)
								printf("\n");	
								
						}
					}

//					printf("%d bytes received\n",received);
				}


				close(sockfd);
			}
		}
	}
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

	dd=23;
	mm=9;
	yy=6;
	
	
	out=getDateBytesFromDate(dd,mm,yy);
	
	for(i=15;i>=0;i--)
		if(out & (1<<i))
			printf("1");
		else
			printf("0");
			

	printf("\n%x %x\n",out>>8,out&0xff);
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
