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

typedef unsigned char bool;
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
	bool high_delay;
	int time_delay_high;
	bool delay_low;
	int time_delay_low;
	char curve[255];
	bool no_linear;
	bool printer;
	char msg[80];
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
};
struct analogLine *analogTable;

int NUMSYSTEMS;
int ***deviceToid; //type,num,ch
int ANALOGCHANNELS=0;

                       
int main(int argc, char **argv)
{
	int sockfd, new_fd;
	struct sockaddr_in my_addr;  
	struct sockaddr_in their_addr;
	int sin_size, gelesen, nbytes,curbytes;
	char buf[MAXDATASIZE];
	char ret[MAXDATASIZE];
	char comm[MAXDATASIZE];
	int MYPORT=3491;
	int i;

	char status[18];

   
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
        	while((curbytes>0)&&(nbytes<12));
	
			if(curbytes>0)
			{
				for(i=0;i<12;i++)
					printf("%02x ",buf[i]);
				printf("\n");
				status[17]=0x55;

				for(i=0;i<16;i+=2)
				{
					status[i]=(random() & 0x3F);
					status[i+1]=(random() & 0xFF);
				}
				status[16]=(random() & 0x03);

				if (send(new_fd, status, 18, 0) == -1)
					perror("send");
			}
			sleep(1);
		}
		while(curbytes>0);
		close(new_fd);
		
	}
}
