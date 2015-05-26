#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include "_experimental/util.h"

typedef unsigned long DWORD;
typedef unsigned int WORD;
typedef unsigned char BYTE;
WORD Crc16(BYTE *Buffer,WORD Len);
int socketConnect(char *address,int port);
int putcrc(BYTE *buffer,WORD crc);



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
		perror("gethostbyname");
		printf("%s %d\n",address,port);
		return -1;
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		printf("%s %d\n",address,port);
		return -1;
	}
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv))
	{
		perror("setsockopt");
		printf("%s %d\n",address,port);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,  sizeof tv))
	{
		perror("setsockopt");
		printf("%s %d\n",address,port);
	}


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
//					perror("Error in connection()"); 
					close(sockfd);
					return -1;
				} 
			} 
			else 
			{ 
//				perror("Timeout or error()"); 
				close(sockfd);
				return -1;
			}
		}
		else 
		{ 
			perror("Error connecting"); 
			close(sockfd);
			return -1;
		} 
	}
	return sockfd;
}

WORD Crc16(BYTE *Buffer,WORD Len)
{
	WORD d0,d1,d2;
	d0=0xffff;
	for(d1=0;d1<Len;d1++)
	{
		d2=Buffer[d1] & 0xff;
		d0^=d2;
		for(d2=0;d2<8;d2++)
		{
			if(d0 & 1)
			{
				d0 >>= 1;
				d0 ^= 0xA001;
			}
			else
			d0 >>= 1;
		}
	}
	return (d0);
}

void main(int argc, char *argv[])
{
	BYTE buffer[512];
	BYTE msg[4]={0x00,0x49,0x00,0x01};
	WORD crc;
	int l;
	int fd;
	char address[20]="192.168.2.107";
	int i;
	int received;
	int numbytes;
	int buflen;
	time_t inizio;
	int timeout;
	int crclength;
	int sendlength;

	timeout=1;


	l=sizeof(msg);
	buflen=sizeof(buffer);
	crc=Crc16(msg,l);
	bzero(buffer,buflen);
	buffer[0]=0x02;
	for(i=0;i<l;i++)
		buffer[1+i]=msg[i];

	crclength=putcrc(&buffer[1+l],crc);
	buffer[1+l+crclength]=0x03;
	sendlength=2+l+crclength;

	printf("connecting to %s port 4001\n",address);
	
	fd=socketConnect(address,4001);
	
	if(fd<1)
		exit(0);
	printf("sending:\n");
	for(i=0;i<sendlength;i++)
		printf("%02x ",buffer[i]);
	printf("\n");

	if (!send(fd,buffer,sendlength,0)) 
		exit(0);

	bzero(buffer,buflen);
	received=0;
	numbytes=0;
	inizio=time(NULL);
	do
	{
		numbytes=recv(fd, &buffer[received], buflen-received, MSG_DONTWAIT);
		if(numbytes>0)
			received+=numbytes;
	}
	while(time(NULL)-inizio<timeout);


	if(received>0)
	{
		printf("output:\n");
		for(i=0;i<received;i++)
			printf("%02x ",buffer[i]);
		printf("\n");
	}
	else
		printf("%d\n",received);
	close(fd);
}

int putcrc(BYTE *buffer,WORD crc)
{
	int out=0;
	BYTE cur;
	cur=crc>>8;
	if((cur==0x10)||(cur==0x02)||(cur==0x03))
	{
		buffer[0]=0x10;
		out++;
	}
	buffer[out]=cur;
	out++;

	cur=crc & 0xff;
	if((cur==0x10)||(cur==0x02)||(cur==0x03))
	{
		buffer[out]=0x10;
		out++;
	}
	buffer[out]=cur;
	out++;
	return out;
}
