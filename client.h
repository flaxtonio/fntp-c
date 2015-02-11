#ifndef CLIENT_H
#define CLIENT_H

#include "socket.h"

typedef struct _fntp_client{
    fntpSocket *socket;
    char * host;
    int port;

    // Events
    void (*DataRecieved) (BYTE *data, int len);
    void (*DataSent) ();
    void (*Error) (error e);
    void (*UdpStopped) (error e);
    void (*TCPDisconnected) ();
    void (*Disconnected)();

} fntpClient;

fntpClient *CreateFntpClient(char *host, int port);
void FntpClientConnect(fntpClient *client);
void FntpClientSend(fntpClient *client, BYTE *data, int len);

#endif