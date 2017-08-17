#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

int _GetHostName(char *buffer, int length);

const char MESSAGE[] = "HELLO WORLD";
const int BACK_LOG = 5;

int main(int argc, char *argv[])
{
	int serverSocket = 0, on = 0, port = 0, status = 0, childPid = 0;
	struct hostent *hostPtr = NULL;
	char hostname[60] = "";
	struct sockaddr_in serverName = {0};
	
	if (2 != argc)
	{
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		exit(1);
	}
	
	port = atoi(argv[1]);
	serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if  (-1 == serverSocket)
	{
		perror("socket()");
		exit(1);
	}
	
	on = 1;
	status = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
	
	if (-1 == status)
	{
		perror("setsockopt(...,SO_REUSEADDR,...)");
	}
	
	{
		struct linger linger = {0};
		linger.l_onoff = 1;
		linger.l_linger = 30;
		status = setsockopt(serverSocket, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));
		
		if (-1 == status)
		{
			perror("setsockopt(...,SO_LINGER,...)");
		}
	}
	
	status = _GetHostName(hostname, sizeof(hostname));
	
	if (-1 == status)
	{
		perror("_GetHostName()");
		exit(1);
	}
	
	hostPtr = gethostbyname(hostname);
	
	if (NULL == hostPtr)
	{
		perror("gethostbyname()");
		exit(1);
	}
	
	(void)memset(&serverName, 0, sizeof(serverName));
	(void)memcpy(&serverName.sin_addr, hostPtr->h_addr, hostPtr->h_length);
	
	serverName.sin_family = AF_INET;
	serverName.sin_port = htons(port);
	status = bind(serverSocket, (struct sockaddr*)&serverName, sizeof(serverName));
	
	if (-1 == status)
	{
		perror("bind()");
		exit(1);
	}
	
	status = listen(serverSocket, BACK_LOG);
	
	if (-1 == status)
	{
		perror("listen()");
		exit(1);
	}
	
	for(;;)
	{
		struct sockaddr_in clientName={0};
		int slaveSocket, clientLength = sizeof(clientName);
		(void)memset(&clientName, 0, sizeof(clientName));
		slaveSocket = accept(serverSocket, (struct sockaddr*)&clientName, &clientLength);
		
		if (-1 == slaveSocket)
		{
			perror("accept()");
			exit(1);
		}
		
		childPid = fork();
		
		switch (childPid)
		{
			case -1:
				perror("fork()");
				exit(1);
			case 0:
				close(serverSocket);
				
				if (-1 == getpeername(slaveSocket, (struct sockaddr*)&clientName, &clientLength))
				{
					perror("getpeername()");
				}
				else
				{
					printf("connection request from %s\n", inet_ntoa(clientName.sin_addr));
				}
				
				write(slaveSocket, MESSAGE, strlen(MESSAGE));
				close(slaveSocket);
				exit(0);
			default:
				close(slaveSocket);
		}
	}
	
	return 0;
}

int _GetHostName(char *buffer, int length)
{
	struct utsname sysname = {0};
	int status = 0;
	status = uname(&sysname);
	
	if (-1 == status)
	{
		strncpy(buffer, sysname.nodename, length);		
	}
	
	return(status);
}