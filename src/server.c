#include "server.h"
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static char quit;
unsigned int clientsConnected;
extern SOCKET serverSocket;

char BeTheServer(struct sockaddr_in *server)
{
	int result = bind(serverSocket, (struct sockaddr *)server, sizeof(*server));
	if(result == SOCKET_ERROR)
	{
		printf("Socket binding failed.\n");
		return 0;
	}

	listen(serverSocket, 10);

	printf("Activated client listening...\n");

	struct ServerRecvData clientsData; //it will be populated by the recv thread
	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		clientsData.clients[i] = INVALID_SOCKET; //initialize the array, in case there was some trash memory still left in the stack
		memset(clientsData.names[i], '\0', MAX_CLIENT_NAME_LENGTH);
	}

	//the receiving thread polls messages sent by clients and sends them to everyone
	pthread_t recvThreadID;
	pthread_create(&recvThreadID, NULL, ServerRecvCallback, &clientsData);

	while(quit == 0)
	{
		char message[MAX_MESSAGE_LENGTH + 1];
		memset(message, '\0', MAX_MESSAGE_LENGTH + 1);
	    fgets(message, MAX_MESSAGE_LENGTH + 1, stdin);

	    if(message[0] == '\n'){continue;}

	    if(strcmp(message, "SERVER_CLOSE\n") == 0)
	    {
	    	quit = 1;
	    	printf("Shutting down...\n");
	    	strcpy(message, "Shutting down...\n");
	    }

	    message[MAX_MESSAGE_LENGTH] = '\n';

	    for(int i = 0; i < MAX_CLIENTS; i++)
	    {
	    	if(clientsData.clients[i] == INVALID_SOCKET) continue;

	    	char namedMessage[MAX_MESSAGE_LENGTH + MAX_CLIENT_NAME_LENGTH + 3]; //+ 3 because 2 is ": " and 1 is "\n"
	    	strcpy(namedMessage, "SERVER: ");
	    	strcat(namedMessage, message);

		    if(send(clientsData.clients[i], namedMessage, strlen(namedMessage), 0) == SOCKET_ERROR)
		    {
		    	printf("Something went wrong when sending the message to '%s'.\n", clientsData.names[i]);
		    	
		    	if(shutdown(clientsData.clients[i], SD_BOTH) == SOCKET_ERROR)
		    	{
        			printf("Shutdown of '%s' failed.\n", clientsData.names[i]);
    			}

		    	closesocket(clientsData.clients[i]);
			    clientsData.clients[i] = INVALID_SOCKET;
		    }
	    }
	}

	pthread_join(recvThreadID, NULL);
	
	return 1;
}

void * ServerRecvCallback(void *args)
{
	struct ServerRecvData *data = (struct ServerRecvData *)args;
	
	fd_set fdRead;

	struct timeval timeout = {0, 1};  //Set to non-blocking

	while(quit == 0)
	{
		FD_ZERO(&fdRead);

		FD_SET(serverSocket, &fdRead); //listening for client connection requests (accept function)

		select(serverSocket + 1, &fdRead, NULL, NULL, &timeout);
		
		if(FD_ISSET(serverSocket, &fdRead)) //if there is 1 or more clients waiting to be accepted
		{
			struct sockaddr_in clientIP;
			int clientSize = sizeof(struct sockaddr_in);
			SOCKET newClient = accept(serverSocket, (struct sockaddr *)&clientIP, &clientSize);
			
			if(newClient == INVALID_SOCKET)
			{
				printf("Failed to accept a client's request to connect.\n");
				
				if(shutdown(newClient, SD_BOTH) == SOCKET_ERROR)
		    	{
        			printf("Shutdown of an unknown client failed.\n");
    			}

				closesocket(newClient);
			}

			else if(clientsConnected == MAX_CLIENTS)
			{
				//recv its name (in the client code there is a send call as soon as it connects to this server)
				char name[MAX_CLIENT_NAME_LENGTH + 1];
				memset(name, '\0', MAX_CLIENT_NAME_LENGTH + 1);
				int bytesReceived = recv(newClient, name, sizeof(name), 0);

			    if(bytesReceived == SOCKET_ERROR)
			    {
			        printf("Something went wrong upon receiving the name from the newly connected client. Kicking him.\n");

			        if(shutdown(newClient, SD_BOTH) == SOCKET_ERROR)
			    	{
	        			printf("Shutdown of the client failed.\n");
	    			}

			        closesocket(newClient);
			    }

			    else if(bytesReceived == 0)
			    {
			    	printf("Client disconnected as soon as it connected.\n");

			    	if(shutdown(newClient, SD_BOTH) == SOCKET_ERROR)
			    	{
	        			printf("Shutdown of the client failed.\n");
	    			}

			        closesocket(newClient);
			    }

			    else
			    {
					printf("Client '%s' tried to connect, but we reached the maximum number of users.\n", name);
					
					char noSpaceLeftMessage[50 + MAX_CLIENT_NAME_LENGTH] = "SERVER: I'm sorry ";
					strcat(noSpaceLeftMessage, name);
					strcat(noSpaceLeftMessage, ", there is no space left for new clients.\n");
					send(newClient, noSpaceLeftMessage, strlen(noSpaceLeftMessage), 0);

					if(shutdown(newClient, SD_BOTH) == SOCKET_ERROR)
			    	{
	        			printf("Shutdown of '%s' failed.\n", name);
	    			}

					closesocket(newClient);
			    }
			}

			else
			{
				//here we found the next empty slot
				//recv its name (in the client code there is a send call as soon as it connects to this server)
				char name[MAX_CLIENT_NAME_LENGTH + 1];
				memset(name, '\0', MAX_CLIENT_NAME_LENGTH + 1);
				int bytesReceived = recv(newClient, name, sizeof(name), 0);

			    if(bytesReceived == SOCKET_ERROR)
			    {
			        printf("Something went wrong upon receiving the name from the newly connected client. Kicking him.\n");

			        if(shutdown(newClient, SD_BOTH) == SOCKET_ERROR)
			    	{
	        			printf("Shutdown of '%s' failed.\n", name);
	    			}

			        closesocket(newClient);
			    }

			    else if(bytesReceived == 0)
			    {
			    	printf("Client disconnected as soon as it connected.\n");

			    	if(shutdown(newClient, SD_BOTH) == SOCKET_ERROR)
			    	{
	        			printf("Shutdown of '%s' failed.\n", name);
	    			}

			        closesocket(newClient);
			    }

			    else
			    {
			    	int index;
			    	for(int i = 0; i < MAX_CLIENTS; i++) {if(data->clients[i] == INVALID_SOCKET) {index = i; break;}}
			    	data->clients[index] = newClient;
			    	data->names[index][bytesReceived] = '\0'; //remove new line from name
			    	strcpy(data->names[index], name);
			    	printf("Client with name '%s' connected.\n", data->names[index]);
			    	const char *welcomeMessage = "SERVER: Hello user, you can send messages now.\n";
					send(newClient, welcomeMessage, strlen(welcomeMessage), 0);
			    	clientsConnected++;
				}
			}
		}

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(data->clients[i] == INVALID_SOCKET) continue;

			FD_ZERO(&fdRead);

			FD_SET(data->clients[i], &fdRead);

			select(data->clients[i] + 1, &fdRead, NULL, NULL, &timeout);			

			if(FD_ISSET(data->clients[i], &fdRead))
			{
				char message[MAX_MESSAGE_LENGTH + 1];
				memset(message, '\0', MAX_MESSAGE_LENGTH + 1);
				int bytesReceived = recv(data->clients[i], message, sizeof(message), 0);

			    if(bytesReceived == SOCKET_ERROR)
			    {
			    	//Here i remove a client if recv from him goes wrong.
			        printf("Something went wrong upon receiving a message from '%s'.\n", data->names[i]);

			        if(shutdown(data->clients[i], SD_BOTH) == SOCKET_ERROR)
			    	{
	        			printf("Shutdown of '%s' failed.\n", data->names[i]);
	    			}

			        closesocket(data->clients[i]);
			        data->clients[i] = INVALID_SOCKET;
			        clientsConnected--;
			        continue;
			    }

			    else if(bytesReceived == 0)
			    {
			    	//Here i remove a client if he disconnects.
			    	printf("Client '%s' disconnected.\n", data->names[i]);

			    	if(shutdown(data->clients[i], SD_BOTH) == SOCKET_ERROR)
			    	{
	        			printf("Shutdown of '%s' failed.\n", data->names[i]);
	    			}

			        closesocket(data->clients[i]);
			        data->clients[i] = INVALID_SOCKET;
			        clientsConnected--;
			        continue;
			    }

			    char namedMessage[MAX_CLIENT_NAME_LENGTH + MAX_MESSAGE_LENGTH + 3];
		    	strcpy(namedMessage, data->names[i]);
		    	strcat(namedMessage, ": ");
		    	strcat(namedMessage, message);

			    printf("%s", namedMessage);
			    //now send it to every client, except the one who sent it first
			    for(int j = 0; j < MAX_CLIENTS; j++)
			    {
			    	if(data->clients[j] == INVALID_SOCKET || j == i) continue;

			    	if(send(data->clients[j], namedMessage, strlen(namedMessage), 0) == SOCKET_ERROR)
				    {
				    	printf("Something went wrong when sending the message to '%s'.\n", data->names[j]);

				    	if(shutdown(data->clients[j], SD_BOTH) == SOCKET_ERROR)
				    	{
		        			printf("Shutdown of '%s' failed.\n", data->names[j]);
		    			}

				    	closesocket(data->clients[j]);
				        data->clients[j] = INVALID_SOCKET;
				    }
			    }
			}
		}
	}
}