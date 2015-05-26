#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#define MAXDATASIZE 5120
#define BACKLOG 10    
                       
int main(int argc, char **argv)
{
	int sockfd, new_fd;
	struct sockaddr_in my_addr;  
	struct sockaddr_in their_addr;
	int sin_size, gelesen, nbytes,curbytes;
	char buf[MAXDATASIZE];
	char ret[MAXDATASIZE];
	char comm[MAXDATASIZE];
	int MYPORT=3490;
	char status[49];
	unsigned char length;
   

	if(argc>1)
		MYPORT=atoi(argv[1]);
	printf("Server \n");
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		perror("socket");
		exit(1);
	}

	my_addr.sin_family = AF_INET;        
	my_addr.sin_port = htons(MYPORT);    
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
	printf("Listen on Port : %d\n",MYPORT);

	sin_size = sizeof(struct sockaddr_in);

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
        	while((curbytes>0)&&(nbytes<1));

			if(curbytes==8)
			{
				for(i=0;i<8;i++)
					printf("%02x ",buf[i]);
				printf("\n");

				if((buf[0]==0x1f)
					&&(buf[1]==0x04)
					&&(buf[2]==0x10))
				{
					if((buf[3]==0x00)
							&&(buf[4]==0x00)
							&&(buf[5]==0x16)
							&&(buf[6]==0xC3)
							&&(buf[7]==0x75))
						length=11;
					else
						if((buf[3]==0x2E)
								&&(buf[4]==0x00)
								&&(buf[5]==0x08)
								&&(buf[6]==0x23)
								&&(buf[7]==0x7B))
							length=4;
						else
							length=0;
					if(length>0)
					{
						status[0]=0x1F;
						status[1]=0x03;
						status[2]=length;
						for(i=0;i<length;i++)
						{
							status[3+4*i]=(random() & 0xFF);
							status[3+4*i+1]=(random() & 0xFF);
							status[3+4*i+2]=(random() & 0xFF);
							status[3+4*i+3]=(random() & 0xFF);
						}
						status[3+4*length]=(random() & 0xFF);
						status[3+4*length+1]=(random() & 0xFF);
					}						
					if (send(new_fd, status, 49, 0) == -1)
						perror("send");
				}
				else
					printf("input buf: %x %x\n",buf[0],buf[1]);
			}
		}
		while(curbytes>0);
		close(new_fd);
		sleep(2);
	}
}
