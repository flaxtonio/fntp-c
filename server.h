#ifndef SERVER_H
#define SERVER_H

#include "client.h"

typedef struct _fntp_server {
    fntpSocket **sockets; // We need Sockets array for server
    char *host;
    int port;
    int connections; // active connections count
    int max_connections; // max connections count

    // Events
    void (*OnNewClient) (fntpClient *client);
    void (*DataRecieved) (BYTE *data, int len, fntpSocket *client);
    void (*DataSent) (void *client);
    void (*Error) (error e);
    void (*UdpStopped) (error e);
    void (*TCPDisconnected) ();
    void (*Disconnected)();
} fntpServer;

// connections - maximimum number of active connections
fntpServer * CreateFntpServer(char *host, int port, int connections);

void FntpServerAddClient(fntpServer *server, int tcp_socket, int udp_socket, remote_addr addr);
void FntpServerListen(fntpServer *s);

#endif