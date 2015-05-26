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

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
 
int main(int argc, char *argv[])
{
    int sockfd, portno, n, c;
    struct sockaddr_in serv_addr;
    struct hostent *server;
 	fd_set myset; 
	struct timeval tv;
	socklen_t lon;
	int valopt;
	int t;
	char *ch;

    char buffer[1024];
    if (argc < 3) {
       fprintf(stderr,"usages %s hostname port\n", argv[0]);
       exit(0);
    }


    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) 
        error("ERROR opening socket");

	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv))
	{
		perror("setsockopt");
        exit(0);
	}
    server = gethostbyname(argv[1]);
    if (server == NULL) 
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
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
    c = 1;

    //bzero(buffer,256);
    //fgets(buffer,255,stdin);
    //n = write(sockfd,buffer,strlen(buffer));
    //printf("%s",buffer);
    //if (n < 0) 
    //     error("ERROR writing to socket");
    while (c > 0) 
    {
	    printf("GET INPUT:");
	    bzero(buffer,1024);
	    fgets(buffer,1023,stdin);

		if(ch=strchr(buffer,'^'))
			*ch=26;
		else
		{
//			buffer=trim(buffer);
			buffer[strlen(buffer)-1]=13;
		}
	    n = write(sockfd,buffer,strlen(buffer));
	    printf("%d",n);
	    printf(" characters input reading: ");
	    printf("%s",buffer,"\n");

		printf("Buffer Clear \n");
        bzero(buffer,1024);
        printf("Buffer Reading ...\n");
        t=0;
		do
		{
			bzero(buffer,1024);
			n = read(sockfd,buffer,1023);
			printf("%s",buffer);
			printf("\n");
			printf("\n");
			if(n>0)
				t+=n;
		}
		while(n>=0);

		printf("%d",t);
		printf(" BUFFER DONE : \n");
    }
    close(sockfd);
    return 0;
}
