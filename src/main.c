#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "server.h"
#include "client.h"

SOCKET serverSocket;

void Cleanup()
{
	if(shutdown(serverSocket, SD_BOTH) == SOCKET_ERROR)
	{
        printf("Shutdown failed.\n");
    }

	closesocket(serverSocket);
	WSACleanup(); //un semplice cleanup di winsock, va chiamato quando non serve più	
	printf("Program terminated, cleanup done. Press ENTER to exit.");
	getchar();
}

int main()
{
	atexit(Cleanup);

	WSADATA wsa; //Info sull'implementazione di winsock, tipo un log di inizializzazione

	//winsock va inizializzato
	//il primo argomento è la versione richiesta, in questo caso 2.2
	//il secondo è un puntatore alla struct WSADATA, è puramente un feedback a noi
	//se il valore di ritorno è diverso da zero, qualcosa è andato storto
	if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Something went wrong when initializing winsock.\n");
		return 1;
	}

	//il primo argomento è il tipo di indirizzo, AF_INET è l'ipv4, AF_INET6 è l'ipv6
	//il secondo argomento è il tipo di socket da creare, SOCK_STREAM invia i dati secondo il protocollo TCP e fa altre cose, leggere documentazione
	//il terzo argomento è il protocolo di invio dati, 0 lo sceglie il provider di servizi automaticamente
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//controllo se il socket è stato creato correttamente
	if(serverSocket == INVALID_SOCKET)
	{
		printf("Socket creation failed.\n");
		return 1;
	}

	//this part above is needed for both server and client, now i do the 2 cases
	printf("Are you a client or a server? 0/1: ");
	char hostType = getchar();
	getchar(); //remove trailing new line char

	if(hostType == '0') //client
	{
		struct sockaddr_in server; //the thing we want to connect to (tcp port and ip, for the AF_INET family)
		server.sin_family = AF_INET; //must be always AF_INET
		
		printf("Server's port: ");
		unsigned short port;
		scanf("%hu", &port);
		server.sin_port = htons(port); //port, htons must be used, read doc to understand

		printf("Server's IP address (4 octects, dot separated): ");
		char IP[16];
		scanf("%s", &IP);
		server.sin_addr.s_addr = inet_addr(IP);

		if(ConnectToServer(&server) == 0)
			return 1;
	}

	else if(hostType == '1')
	{
		struct sockaddr_in server; //in this case, us(tcp port and ip, for the AF_INET family)
		server.sin_family = AF_INET; //must be always AF_INET
		
		printf("Server's port: ");
		unsigned short port;
		scanf("%hu", &port);
		server.sin_port = htons(port); //port, htons must be used, read doc to understand

		server.sin_addr.s_addr = INADDR_ANY;

		if(BeTheServer(&server) == 0)
			return 1;
	}

	else
	{
		printf("Wrong answer.\n");
		return 1;
	}
}