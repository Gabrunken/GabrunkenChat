#include "client.h"
#include "settings.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static char quit;
extern SOCKET serverSocket;

char ConnectToServer(struct sockaddr_in *server)
{
	//first argument is the socket, second is the destination (must be casted to a normal sockaddr because it is simulating polymorphism),
	//it will later find out that it is a _in type sockadrr.
	//then the last is simply the size of the dest.
	int connection = connect(serverSocket, (struct sockaddr*)server, sizeof(*server));
	if(connection == SOCKET_ERROR)
	{
		printf("Failed to connect to server.\n");
		return 0;
	}

	printf("Connected to server.\n");
	printf("Write your name: ");

	char name[MAX_CLIENT_NAME_LENGTH + 1];
	fgets(name, MAX_CLIENT_NAME_LENGTH + 1, stdin);
	fgets(name, MAX_CLIENT_NAME_LENGTH + 1, stdin);
	name[strcspn(name, "\n")] = 0;

	if(send(serverSocket, name, strlen(name), 0) == SOCKET_ERROR)
    {
    	printf("Something went wrong when sending your name to the server.\n");
    	return 0;
    }

    //the receiving thread polls messages and prints them
	pthread_t recvThreadID;
	pthread_create(&recvThreadID, NULL, ClientRecvCallback, NULL);

	while(quit == 0)
	{
		char message[MAX_MESSAGE_LENGTH + 1];
		memset(message, '\0', MAX_MESSAGE_LENGTH + 1);
	    fgets(message, MAX_MESSAGE_LENGTH + 1, stdin);

	    if(message[0] == '\n'){continue;}

	    message[MAX_MESSAGE_LENGTH] = '\n';
	    if(send(serverSocket, message, strlen(message), 0) == SOCKET_ERROR)
	    {
	    	if(quit == 0) //If we are not in a quit state (server is still working)
	    	{
	    		printf("Something went wrong when sending the message to the server.\n");
	    	}
	    	
	    	quit = 1;
	    	pthread_join(recvThreadID, NULL);
	    	return 0;
	    }
	}

	pthread_join(recvThreadID, NULL);
	return 1;
}

void * ClientRecvCallback(void *args)
{
	while(quit == 0)
	{
		char message[MAX_MESSAGE_LENGTH + 1];
		memset(message, '\0', MAX_MESSAGE_LENGTH + 1);
		int bytesReceived = recv(serverSocket, message, sizeof(message), 0);

	    if(bytesReceived == SOCKET_ERROR)
	    {
	        printf("Something went wrong upon receiving a message.\n");
	        quit = 1;
	        break;
	    }

	    else if(bytesReceived == 0)
	    {
	    	printf("Server disconnected.\n");
	        quit = 1;
	        break;
	    }

	    printf("%s", message);
	}
}