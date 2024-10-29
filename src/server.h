#ifndef server_h_
#define server_h_
#include <winsock2.h>
#include "settings.h"
char BeTheServer(struct sockaddr_in *server);
void * ServerRecvCallback(void *args);
struct ServerRecvData
{
	char names[MAX_CLIENTS][MAX_CLIENT_NAME_LENGTH];
	SOCKET clients[MAX_CLIENTS];
};
#endif