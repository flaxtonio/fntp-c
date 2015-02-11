#ifndef SOCKET_H
#define SOCKET_H

#include <netinet/in.h>
#include "lib.h"

typedef struct sockaddr_in remote_addr;

typedef struct _socket {
    int tcp_socket; // fntpSocket for TCP Connection Server or Client
    int udp_socket; // Udp binded socket
    remote_addr remote_udp; // Remote UDP address
    BOOL closed;
    BOOL isServer; // Some functionality differences for client and server
    MetaData *DataIn; // Map for Data income/receive
    MetaData *DataOut; // Map for Data outcome/sending
    pthread_t udp_thread; // after creating Socket UDP will start listening
    pthread_mutex_t lock;

    // Events
    void (*DataRecievedClient) (BYTE *data, int len);
    void (*DataRecievedServer) (BYTE *data, int len, struct _socket *s);
    void (*DataSentClient) ();
    void (*DataSentServer) (void *s);
    void (*Error) (error e);
    void (*UdpStopped) (error e);
    void (*TCPDisconnected) ();
    void (*Disconnected)();
} fntpSocket;

typedef struct {
    int len;
    BYTE *data;
    remote_addr remoteAddr;
} udp_receive;

typedef struct {
    int len;
    char *data;
    fntpSocket *s;
    void (*funcS) (BYTE *data, int len, struct _socket *s);
    void (*funcC) (BYTE *data, int len);
    MetaData *freeIn;
} data_callback;

typedef struct {
    BYTE read_buf[UdpPocketLength];
    remote_addr remote_udp;
} udp_data;

// Init socket
fntpSocket *create_socket(int tcp_sock, int udp_sock, remote_addr udp_addr, BOOL isServer);

// Creates MetaData from TCP received bytes
MetaData *tcp_create_receive_meta(int dataId, int len);

// Creates MetaData from bytes which needs to send, and tcp_data which would be sent by TCP
MetaData *tcp_create_send_meta(BYTE * data, int data_length, BYTE *tcp_data);

void *fntp_read_tcp(void *s); // Maybe could run as seperate Thread
void fntp_write_tcp(fntpSocket *s, BYTE *data, int len);

void fntp_read_udp(fntpSocket *s, BYTE *data, remote_addr remote_udp);
void fntp_write_udp(fntpSocket *s, BYTE *data, int len);

// Starts receiving data from UDP socket
void *fntp_listen_udp(void *s); // Maybe could run as seperate Thread

// Sends using FNTP socket
void fntp_send(fntpSocket *sock, BYTE *data, int len);

#endif