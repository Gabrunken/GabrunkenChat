#ifndef client_h_
#define client_h_
#include <winsock2.h>
char ConnectToServer(struct sockaddr_in *server);
void * ClientRecvCallback(void *args);
#endif