#About Protocol
FNTP is a combination of TCP and UDP as a mixed transport layer protocol. The base  thing is that TCP works slow than UDP but it's reliable and the base idea of Flaxton FNTP protocol is to combine TCP reliable future and UDP speed together.
Using this combination of TCP and UDP it makes possible to transfer all data faster using UDP and stay informed about sent data using reliable TCP.

#Protocol Implementation
Let's consider the case when client sends 1000 bytes data to server.
In client-side, FNTP creates TCP package where first 4 byte is generated random ID (for the connection) and next 4 bytes are length of sending data (i.e. 1000 bytes). 
<img src="http://flaxton.io/img/fntp/1.jpg" />
 
Then FNTP creates  packages with 108 length, where first 4 byte is generated ID, next 4 bytes are the number (index) of package, the rest 100 bytes are sending data.

<img src="http://flaxton.io/img/fntp/2.jpg" />

When server response that it is ready to recieve the data, client sends packages via UDP protocol. Client sends the packages untill the server get the packages with all indexes for a given ID. After getting all the packages server send to client, signal (TCP package), to stop the sending of data. 
<img src="http://flaxton.io/img/fntp/3.jpg" />


<b>The server sorting all the packages by index for a given ID.</b>

<img src="http://flaxton.io/img/fntp/4.jpg" />
#Usage 
<b>With main.c one can test file transfer between client and server via FNTP protocol.<b>
<b>Client-side</b>
```c
void receive (BYTE *data, int len)
{
    puts(data);
}
.
.
.
fntpClient *client = CreateFntpClient("192.168.107.33", 8888);
                client->DataRecieved = receive;
                client->Error = err;
                FntpClientConnect(client);
```

<b>Server-side</b>
```c
void receiveS (BYTE *data, int len, fntpSocket *s)
{
//    puts(data);
    FILE *fp;

    fp = fopen("big2.zip", "a+");
    fwrite(data, 1, len, fp);
    fclose(fp);
}
.
.
.
fntpServer *server = CreateFntpServer("0.0.0.0", 8888, 1024);
                server->DataRecieved = receiveS;
                server->Error = err;
                FntpServerListen(server);
```

<b>For further details contact us tigran@flaxton.io</b>
