#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#define MYPORT 3490    
#define MAXDATASIZE 5120
#define BACKLOG 10    
                       
int main()
{
	int sockfd, new_fd;
	struct sockaddr_in my_addr;  
	struct sockaddr_in their_addr;
	int sin_size, gelesen;
	char buf[MAXDATASIZE];
	char ret[MAXDATASIZE];
	char comm[MAXDATASIZE];
	FILE *popen(), *p1; /* per la  pipe */
   

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

//	while(1)
	{  
		sin_size = sizeof(struct sockaddr_in);

		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
		{
			perror("accept");
	//		continue;
		}

		printf("Connect!! %s is on the Server!\n",inet_ntoa(their_addr.sin_addr));

		//if (!fork())
		while(1)
        {
        	// Processo figlio
            if (recv(new_fd,buf,MAXDATASIZE,0) == -1)
            	perror("recv");
			
			buf[MAXDATASIZE-1]='\0';

			fprintf(stderr,"User Command: %s\n",buf);


			sprintf(comm,"%s",buf);

			if(p1 = popen(comm, "r"))
				printf("Opening input pipe.\n");
			else
            {
            	fprintf(stderr,"Cannot open input pipe.\n");
				exit(1);
			}
			gelesen=fread(ret,1,MAXDATASIZE,p1);

			printf("Ba[k ha letto : %i Byte \n",gelesen);

			pclose(p1);

			if (send(new_fd, ret, gelesen, 0) == -1)
                   perror("send");
			if (send(new_fd, "gino", 4, 0) == -1)
                   perror("send");
//			close(new_fd);
//			exit(0);
		}
		close(new_fd);  
       
//		while(waitpid(-1,NULL,WNOHANG) > 0);
	}
}


