
#ifndef HANDLECLIENT_H
#define HANDLECLIENT_H
#include <netinet/in.h>

void error(char *msg);
void handleClient(int newsockfd);

#endif