#include "main.h"

MYSQL *mysqlConnect()
{
//MYSQL STUFF
	MYSQL *connection;

	mysql_init(&mysql);
	my_bool reconnect = 1;
	mysql_options(&mysql, MYSQL_OPT_RECONNECT, &reconnect);
	connection = mysql_real_connect(&mysql,"localhost","root", "minair","socket", 0, NULL,0);
	if( connection == NULL ) 
	{
		printf("%s\n",mysql_error(&mysql));
		return(0);
	}
	return connection;
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

	void *temp_shm_pointer;
	out=(*shmid) = shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL | 0600);
	if(out==-1)
	{
		perror("shmget ");
		return 0;
	}

	printf("SHMID: %d\n",*shmid);

	temp_shm_pointer = (void *)shmat((*shmid), 0, 0);
	if (temp_shm_pointer == (void *)(-1))
		return 0;
	return temp_shm_pointer;
}


void dumpBuffer(unsigned char *buf,int len)
{
	int i;
	for(i=0;i<len;i++)
		printf("%02x ",buf[i]);
	printf("\n");
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


void killAllSystems()
{
	if(CEIABIGateways&&(shmdt(CEIABIGateways)==-1))
		perror("shmdt CEIABIGateways");
	CEIABIGateways=0;
	shmctl(SHMCEIABIGATEWAYSID, IPC_RMID, &shmid_struct);
}

int main(int argc, char *argv[])
{
	int i;

	connection=mysqlConnect();
	NUMCEIABIGATEWAYS=0;

	if(loadCEIABI(0))
	{
		killAllSystems();
		exit(0);
	}


	for(i=0;i<NUMCEIABIGATEWAYS;i++)
	{
		printf("%d %s\n",CEIABIGateways[i].id_CEIABI_gateway,CEIABIGateways[i].address);
		printf("\n");
	}

	printf("pid: %d\n",getpid());
	
	doCEIABI(0);
}

