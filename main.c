#include <stdio.h>
#include <signal.h>
#include "client.h"
#include "server.h"

void receive (BYTE *data, int len)
{
    puts(data);
}

void receiveS (BYTE *data, int len, fntpSocket *s)
{
//    puts(data);
    FILE *fp;

    fp = fopen("big2.zip", "a+");
    fwrite(data, 1, len, fp);
    fclose(fp);
}

void err (error e)
{
    puts(e.message);
}

void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    printf("Caught segfault at address %d\n", si->si_errno);
    exit(0);
}

int main(int argc , char **argv)
{

    struct sigaction sa;

    memset(&sa, 0, sizeof(sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sigaction;
    sa.sa_flags   = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);

    if(argc >= 2)
    {
        switch (argv[1][0])
        {
            case 'c':
            {
                fntpClient *client = CreateFntpClient("192.168.107.33", 8888);
                client->DataRecieved = receive;
                client->Error = err;
                FntpClientConnect(client);

                FILE *fp;
                char read_buffer[50000];
                size_t read_size;
                int index=0;

                fp = fopen("big.zip", "r");
                while(TRUE)
                {
                    fseek(fp, index, SEEK_SET);
                    if((read_size = fread(read_buffer, 1, 50000, fp)) <= 0)
                    {
                        puts("ends");
                        return 0;
                    }
                    FntpClientSend(client, read_buffer, read_size);
                    index += read_size;
                    printf("Index :%d\n", index);
                }

//                while(TRUE)
//                {
//                    int i;
//                    fgets(str, 80, stdin);
//                    i = strlen(str)-1;
//                    if( str[ i ] == '\n')
//                        str[i] = '\0';
//                    FntpClientSend(client, str, strlen(str));
//                }
            };
                break;
            case 's':
            {
                fntpServer *server = CreateFntpServer("0.0.0.0", 8888, 1024);
                server->DataRecieved = receiveS;
                server->Error = err;
                FntpServerListen(server);
            };
                break;
                default:
                    puts("Type [c - client, s - server] !");
        }
    }

    return 0;
}