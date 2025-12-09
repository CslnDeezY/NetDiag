#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

extern int errno;
#define message_len 512
int port;

int recive_Message(int fd, char* buffer);
int send_Message(int sd, char* buffer);

int main (int argc, char *argv[])
{
    int sd;			// socket descriptor
    struct sockaddr_in server;
    char msg[message_len];		

    if(argc != 3){
        printf ("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi (argv[2]);

    if((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
        perror ("[client] Error socket())\n");
        return errno;
    }


    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons (port);


    if(connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1){
        perror ("[client]Errorconnect()\n");
        return errno;
    }

    while(true){
        //citirea mesajului 
        bzero (msg, message_len);
        printf ("[client]Introduceti comanda: ");
        fflush (stdout);
        read(0, msg, message_len-1);

        //triitem comanda
        if(send(sd, msg, strlen(msg), 0) < 0){
            perror ("[client]Eroare la send() spre server.\n");
            return errno;
        }
        
        //citim raspunsul dat de server
        if(recive_Message(sd, msg) <= 0){
            perror ("[client]Error read() from server\n");
            return errno;
        }


    }

    close (sd);
}

int recive_Message(int fd, char* buffer){
    int bytes;
    bzero(buffer, sizeof(&buffer));

    //citim lungimea mesajului
    if(recv(fd, &bytes, sizeof(bytes), 0) < 0){
        perror ("[client] Error recv length from server\n");
        return -1;
    }

    //citim mesajul
    if(bytes > 0){
        if(recv(fd, buffer, bytes, 0) < 0){
            perror ("[client] Error recv() from server\n");
            return -1;
        }
    }

    printf ("[client]Am citit: %d bytes: %s\n", bytes, buffer);
    return bytes;
}

int send_Message(int sd, char* buffer){
    int bytes = strlen(buffer);
    if(send(sd, &bytes, sizeof(bytes), 0) < 0){
        perror("[client] Error send length to server\n");
        return -1;
    }
    if(bytes > 0){
        if(send(sd, buffer, bytes, 0) < 0){
            perror("[client] Error send to server\n");
            return -1;
        }

    }
    return bytes;
}