#include "socket.h"
#include <utmp.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <stdio.h>

static udp_data UDP_STACK[500000];
static int UDP_STACK_INDEX = 0;
static pthread_mutex_t UDP_LOCK;

fntpSocket *create_socket(int tcp_sock, int udp_sock, remote_addr udp_addr, BOOL isServer)
{
    fntpSocket * retSock;
    retSock = (fntpSocket *) malloc(sizeof(fntpSocket));
    retSock->closed = FALSE;
    retSock->DataIn = NULL; // Setting NULL for add chain on use
    retSock->DataOut = NULL;
    retSock->remote_udp = udp_addr;
    retSock->tcp_socket = tcp_sock;
    retSock->udp_socket = udp_sock;
    retSock->isServer = isServer;

    retSock->udp_thread = fntp_thread_start(fntp_listen_udp, retSock);

    return retSock;
}

MetaData *tcp_create_receive_meta(int dataId, int len)
{
    MetaData *retMeta;
    retMeta = (MetaData *)malloc(sizeof(MetaData));
    if(retMeta == NULL)
        return NULL;

    retMeta->DataId = dataId;
    retMeta->Length = len;
    retMeta->PocketCount = calc_pocket_count(retMeta->Length, POCKET_LENGTH);
    retMeta->DataStack = (MapData *) malloc(MAP_SIZEOF(retMeta));
    retMeta->StopUDP = FALSE;
    retMeta->next = NULL;
    retMeta->currentPocketCount = 0;

    return retMeta;
}

MetaData *tcp_create_send_meta(BYTE * data, int data_length, BYTE *tcp_data)
{
    MetaData *retMeta;
    retMeta = (MetaData *)malloc(sizeof(MetaData));
    if(retMeta == NULL || data == NULL || tcp_data == NULL)
        return NULL;
    BYTE dataID[4], length[4], pocketIndex[4];
    int i, j, id, dataIndex;

    //Generating Random ID
    struct timeval t;
    gettimeofday(&t,NULL);
    srand((t.tv_sec * 1000) + (t.tv_usec / 1000));
    id = rand();

    int_to_bytes(data_length, length);
    int_to_bytes(id, dataID);

    for(i=0; i<4; i++) // tcp_data MUST BE 8 Byte
    {
        tcp_data[i] = dataID[i];
        tcp_data[i+4] = length[i];
    }

    // Making meta
    retMeta->DataId = id;
    retMeta->Length = data_length;
    retMeta->PocketCount = calc_pocket_count(data_length, POCKET_LENGTH);
    retMeta->DataStack = (MapData *) malloc(MAP_SIZEOF(retMeta));
    retMeta->StopUDP = FALSE;
    retMeta->next = NULL;

    dataIndex = 0;
    BYTE tempSet;
    for(i=0; i < retMeta->PocketCount; i++)
    {
        int_to_bytes(i, pocketIndex);
        memcpy(retMeta->DataStack[i].value, dataID, 4);
        memcpy(&retMeta->DataStack[i].value[4], pocketIndex, 4);

        for(j=8; j < UdpPocketLength; j++)
        {
            if(dataIndex >= data_length)
            {
                tempSet = 15;
            }
            else
            {
                tempSet = data[dataIndex];
                dataIndex++;
            }
            retMeta->DataStack[i].value[j] = tempSet;
        }
    }

    return retMeta;
}

void *fntp_read_tcp(void *s)
{
    fntpSocket * sock;
    sock = (fntpSocket *) s;
    BYTE buf[8], dataId[4], length[4];
    int data_id, lengthInt;
    ssize_t read_size;
    while ( (read_size = recv(sock->tcp_socket, buf, 8, 0)) > 0)
    {
        if(sock->closed)
            break;

        switch (read_size)
        {
            case 5:
            {
                // END of UDP send
                memcpy(dataId, buf, 4);
                data_id = bytes_to_int(dataId);
                MetaData *m;
                m = meta_search_point(sock->DataOut, data_id);
                if(m != NULL)
                {
                    m->StopUDP = TRUE;
                }
            };
                break;
            case 8:
            {
                memcpy(dataId, buf, 4);
                memcpy(length, &buf[4], 4);
                data_id = bytes_to_int(dataId);
                lengthInt = bytes_to_int(length);
                MetaData *m = tcp_create_receive_meta(data_id, lengthInt);
                if(sock->DataIn == NULL)
                {
                    sock->DataIn = m;
                }
                else
                {
                    meta_set_data(&sock->DataIn, data_id, m);
                }
            };
                break;
        }
    }
    if(sock->TCPDisconnected) sock->TCPDisconnected();
    delete_meta(sock->DataIn);
    delete_meta(sock->DataOut);
    free(sock);
    return NULL;
}

void fntp_write_tcp(fntpSocket *s, BYTE *data, int len)
{
    int flag = 1;
    setsockopt(s->tcp_socket,            /* socket affected */
            IPPROTO_TCP,     /* set option at TCP level */
            TCP_NODELAY,     /* name of option */
            (char *) &flag,  /* the cast is historical
                                                         cruft */
            sizeof(int));    /* length of option value */
    if(send(s->tcp_socket, data, len, 0) < 0)
    {
        error e;
        e.code = ERROR_SENDING_TCP;
        e.message = "Error Sending TCP data";
        s->Error(e);
    }
}

void *thread_callback(void *c)
{
    data_callback *cb = (data_callback*)c;
    if(cb->funcS) cb->funcS(cb->data, cb->len, cb->s);
    else if(cb->funcC) cb->funcC(cb->data, cb->len);
//    free(cb->data);
//    free(cb->freeIn->DataStack);
//    free(cb->freeIn);
    return NULL;
}

void fntp_read_udp(fntpSocket *s, BYTE *data, remote_addr remote_udp)
{
    BYTE dataId[4], indexKey[4];
    int data_id, index;
    memcpy(dataId, data, 4);
    data_id = bytes_to_int(dataId);

    // if DataID doesn't exists return
    MetaData *m = meta_search_point(s->DataIn, data_id);
    if(m == NULL) return;

    s->remote_udp = remote_udp; // UDP address could be changed during receiving

    if(m->StopUDP) return; // Maybe we will want to block some incomming data

    // Calculate If All data received
    if(m->currentPocketCount == m->PocketCount)
    {
        m->StopUDP = TRUE;
        BYTE end[5];
        data_callback cb;
        cb.data = malloc(sizeof(BYTE) * m->Length);
        cb.len = m->Length;
        cb.s = s;
        cb.freeIn = s->DataIn;
        memcpy(end, dataId, 4);
        end[4] = STOP_DATA; // 1 Byte end symbol
        combine_map_stack(m, cb.data);

        if(s->isServer)
        {
            if(s->DataRecievedServer) {
                cb.funcS = s->DataRecievedServer;
                fntp_thread_start(thread_callback, &cb);
            }
        }
        else
        {
            if(s->DataRecievedClient) {
                cb.funcC = s->DataRecievedClient;
                fntp_thread_start(thread_callback, &cb);
            }
        }
//        m = s->DataIn; // free function works not sync, that's why we must free memory from another pointer
        s->DataIn = NULL;
        //TODO: meta_delete_key function not works properly
//        meta_delete_key(&m, data_id);
        fntp_write_tcp(s, end, 5);
        return;
    }

    memcpy(indexKey, &data[4], 4);
    index = bytes_to_int(indexKey);
    if(index < 0 || index > m->PocketCount)
        return;

    if(m->DataStack[index].key == data_id) return; // This data already added
    memcpy(m->DataStack[index].value, &data[8], POCKET_LENGTH);
    m->currentPocketCount++;
    m->DataStack[index].key = data_id;
}

void fntp_write_udp(fntpSocket *s, BYTE *data, int len)
{
    if(sendto(s->udp_socket, data, len, 0, (struct sockaddr*)&s->remote_udp, sizeof(s->remote_udp)) < 0)
    {
        error e;
        e.code = ERROR_SENDING_UDP;
        e.message = "Error Sending UDP data";
        if(s->Error) s->Error(e);
    }
}

void *StartReader(void *s)
{
    fntpSocket *sock = (fntpSocket*) s;
    while(TRUE)
    {
        usleep(1000);
        if(UDP_STACK_INDEX == 0) continue;
//        pthread_mutex_lock(&UDP_LOCK);
        while(UDP_STACK_INDEX >= 0)
        {
            fntp_read_udp(sock, UDP_STACK[UDP_STACK_INDEX].read_buf, UDP_STACK[UDP_STACK_INDEX].remote_udp);
            UDP_STACK_INDEX--;
        }
        UDP_STACK_INDEX = 0;
//        pthread_mutex_unlock(&UDP_LOCK);
    }
    return NULL;
}

void *fntp_listen_udp(void *s)
{
    fntpSocket *sock = (fntpSocket *) s;
    ssize_t recv_len;
    BYTE read_buf[UdpPocketLength];
    remote_addr remote_udp;
    if (pthread_mutex_init(&UDP_LOCK, NULL) != 0)
    {
        printf("\n mutex init failed\n");
    }
    socklen_t socklen;
//    fntp_thread_start(StartReader, sock);
    bzero(&remote_udp, sizeof(remote_udp));
    socklen = sizeof(remote_udp);
    while (TRUE)
    {
        recv_len = recvfrom(sock->udp_socket, read_buf, UdpPocketLength, 0, (struct sockaddr*)&remote_udp, &socklen);
        if(recv_len < 0)
            break;
        if(recv_len < UdpPocketLength) continue;
        fntp_read_udp(sock, read_buf, remote_udp);
//        if(UDP_STACK_INDEX >= 500000) continue;
//        pthread_mutex_lock(&UDP_LOCK);
//        memcpy(UDP_STACK[UDP_STACK_INDEX].read_buf, read_buf, UdpPocketLength);
//        UDP_STACK[UDP_STACK_INDEX].remote_udp = remote_udp;
//        UDP_STACK_INDEX++;
//        pthread_mutex_unlock(&UDP_LOCK);
    }
    return NULL;
}

void fntp_send(fntpSocket *sock, BYTE *data, int len)
{
    BYTE tcp_data[8];
    MetaData *m = tcp_create_send_meta(data, len, tcp_data);
    fntp_write_tcp(sock, tcp_data, 8);
    if(sock->DataOut == NULL)
    {
        sock->DataOut = m;
    }
    else
    {
        meta_set_data(&sock->DataOut, m->DataId, m);
    }
    int i;
    while (TRUE)
    {
        if(sock->closed)
        {
            break;
        }
        for(i=0; i < m->PocketCount; i++)
        {
            if(m->StopUDP)
            {
                error e;
                e.code = SUCCESS;
                e.message = "";
                if(sock->UdpStopped) sock->UdpStopped(e);
                meta_delete_key(&sock->DataOut, m->DataId);
                sock->DataOut = NULL;
                return;
            }
            fntp_write_udp(sock, m->DataStack[i].value, UdpPocketLength);
        }
        usleep(100);
    }
    meta_delete_key(&sock->DataOut, m->DataId);
    sock->DataOut = NULL;
}