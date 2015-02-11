#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "server.h"

//void listen_udp(fntpServer *server, int udp_socket, struct sockaddr_in addr)
//{
//
//}

fntpServer * CreateFntpServer(char *host, int port, int connections)
{
    fntpServer *server = (fntpServer *) malloc(sizeof(fntpServer));
    server->connections = 0;
    server->max_connections = connections;
    server->host = host;
    server->port = port;
    server->sockets = (fntpSocket**) malloc(connections * sizeof(fntpSocket*));

    return server;
}

void FntpServerAddClient(fntpServer *server, int tcp_socket, int udp_socket, remote_addr addr)
{
    if(server->connections >= server->max_connections) return;

    server->sockets[server->connections] = create_socket(tcp_socket, udp_socket, addr, TRUE);
    server->sockets[server->connections]->DataRecievedServer = server->DataRecieved;
    server->sockets[server->connections]->DataSentServer = server->DataSent;
    server->sockets[server->connections]->Error = server->Error;
    server->sockets[server->connections]->Disconnected = server->Disconnected;
    server->sockets[server->connections]->TCPDisconnected = server->TCPDisconnected;
    server->sockets[server->connections]->UdpStopped = server->UdpStopped;
    fntp_thread_start(fntp_read_tcp, server->sockets[server->connections]);
    server->connections++;
}

void FntpServerListen(fntpServer *server){
    int socket_desc, socket_udp;
    struct sockaddr_in server_adr, servaddr;

    socket_udp = socket(AF_INET,SOCK_DGRAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(server->host);
    servaddr.sin_port=htons((uint16_t)server->port);
    bind(socket_udp,(struct sockaddr *)&servaddr,sizeof(servaddr));

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        if(server->Error)
        {
            error e;
            e.code = ERROR_CREATE_SOCKET;
            e.message = "Error creating socket";
            server->Error(e);
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
    server_adr.sin_addr.s_addr = inet_addr(server->host);
    server_adr.sin_family = AF_INET;
    server_adr.sin_port = htons((uint16_t)server->port);

    if (bind(socket_desc, (struct sockaddr*)&server_adr, sizeof(server_adr)) < 0)
    {
        if(server->Error)
        {
            error e;
            e.code = ERROR_CREATE_SOCKET;
            e.message = "Error Binding socket...";
            server->Error(e);
        }
        exit(1);
    }
    if(server->Error)
    {
        error e;
        e.code = SUCCESS;
        e.message = "Binded !";
        server->Error(e);
    }
    if (listen(socket_desc, server->max_connections) < 0)
    {
        if(server->Error)
        {
            error e;
            e.code = ERROR_CREATE_SOCKET;
            e.message = "Error Listenning Socket...";
            server->Error(e);
        }
        exit(1);
    }
    if(server->Error)
    {
        error e;
        e.code = SUCCESS;
        e.message = "Listenning !";
        server->Error(e);
    }
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    remote_addr remote_udp;
    int connfd;
    for(;;)
    {
        clilen=sizeof(cliaddr);
        bzero(&remote_udp, sizeof(remote_udp));
        connfd = accept(socket_desc,(struct sockaddr *)&cliaddr,&clilen);
        FntpServerAddClient(server, connfd, socket_udp, remote_udp);
    }
}