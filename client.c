#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "client.h"

fntpClient *CreateFntpClient(char *host, int port)
{
    fntpClient *client = (fntpClient*)malloc(sizeof(fntpClient));
    client->host = host;
    client->port = port;

    return client;
}

void FntpClientConnect(fntpClient *client)
{
    int socket_desc, socket_udp;
    struct sockaddr_in server, servaddr;

    socket_udp = socket(AF_INET,SOCK_DGRAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(client->host);
    servaddr.sin_port=htons((uint16_t)client->port);

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        if(client->Error)
        {
            error e;
            e.code = ERROR_CREATE_SOCKET;
            e.message = "Error creating socket";
            client->Error(e);
            return;
        }
    }
    int flag = 1;
    setsockopt(socket_desc,            /* socket affected */
            IPPROTO_TCP,     /* set option at TCP level */
            TCP_NODELAY,     /* name of option */
            (char *) &flag,  /* the cast is historical
                                                         cruft */
            sizeof(int));    /* length of option value */
    server.sin_addr.s_addr = inet_addr(client->host);
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)client->port);

    //Connect to remote server
    if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        if(client->Error)
        {
            error e;
            e.code = ERROR_CONNECTING;
            e.message = "Error connecting Server socket";
            client->Error(e);
            return;
        }
        return;
    }

    client->socket = create_socket(socket_desc, socket_udp, servaddr, FALSE);
    client->socket->DataRecievedClient = client->DataRecieved;
    client->socket->DataSentClient = client->DataSent;
    client->socket->Error = client->Error;
    client->socket->Disconnected = client->Disconnected;
    client->socket->TCPDisconnected = client->TCPDisconnected;
    client->socket->UdpStopped = client->UdpStopped;
    fntp_thread_start(fntp_read_tcp, client->socket);
}

void FntpClientSend(fntpClient *client, BYTE *data, int len)
{
    fntp_send(client->socket, data, len);
}