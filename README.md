#About Protocol
FNTP is a combination of TCP and UDP as a mixed transport layer protocol. TCP works slower than UDP but it's reliable and the basic idea of Flaxton FNTP protocol is to combine TCP reliability feature and UDP speed together.
Using this combination of TCP and UDP makes it possible to transfer all the data faster using UDP and be informed about the sent data using reliable TCP.

#Protocol Implementation
Let's consider the case when client sends 1000 bytes data to server.
In client-side FNTP creates TCP package where the first 4 bytes contain the generated random ID (for the connection) and the next 4 bytes contain the length of sending data (i.e. 1000 bytes). 
<img src="http://flaxton.io/img/fntp/1.jpg" />
 
Then FNTP creates packages with 108 length, where the first 4 bytes contain the generated ID, next 4 bytes contain the number (index) of package, the rest 100 bytes are the sending data.

<img src="http://flaxton.io/img/fntp/2.jpg" />

When server responses that it is ready to recieve the data, the client sends packages via UDP protocol. The client sends the packages untill the server gets the packages with all indexes for the given ID. After getting all the packages the server sends to client a signal (TCP package) to stop the sending of data. 
<img src="http://flaxton.io/img/fntp/3.jpg" />


<b>The server sorts all the packages by indexes for given ID.</b>

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

<b>For further details don't hesitate to contact us tigran@flaxton.io</b>
