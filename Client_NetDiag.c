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


    if(connect(sd,(struct sockaddr *) &server, sizeof(struct sockaddr)) == -1){
        perror("[client]Errorconnect()\n");
        return errno;
    }
    //astept fie stdin si trimit , fie socket si afisez
    fd_set rfds;
    int maxfd;

    printf("[client] Conectat la %s:%d\nPuteti incepe a trimite comenzi\n", argv[1], port);

    while(true){

// varianta veche de functionare nu mai este potrivita
///acum verifica constant daca vrem sa trimitem o comanda sau sa primim un raspuns
        FD_ZERO(&rfds);
        FD_SET(sd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);

        if( sd > STDIN_FILENO )
            maxfd = sd;
        else
            maxfd = STDIN_FILENO;
        
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) {
            perror("[client] Error select()");
            return errno;
        }

        if(FD_ISSET(sd, &rfds)){
            //clientul trebuie sa afiseze raspunsurile:
            bzero(msg,sizeof(msg));
            recive_Message(sd, msg);
            if(strlen(msg) == 0){
                printf("[client] Conexiunea la server s-a inchis.\n");
                break;
            }
            fflush(stdout);
        }

        if(FD_ISSET(STDIN_FILENO, &rfds)){
            //clientru trebuie sa citeasca de la tastatura
            bzero(msg,sizeof(msg));
            if(fgets(msg, sizeof(msg)-1, stdin) == NULL){
                perror("[client] Error fgets()\n");
                continue;
            }
            char copy[message_len];
            strcpy(copy, msg);
            char *token = strtok(copy, " \n");
            if(token == NULL){//comanda goala
                continue;
            }else if(strcmp(token, "quit") == 0){
                printf("[client] Inchid conexiunea...\n");
                if(send(sd, msg, strlen(msg), 0) <= 0){
                    perror("[client]Eroare la send\n");
                    errno;
                }
                recive_Message(sd, msg);
                if(strcmp(msg, "Quit Accepted\n") == 0){
                    printf("[client] Conexiunea la server s-a inchis.\n");
                } else printf("[client] Raspuns neasteptat de la server: %s mai stau un pic\n", msg);
                break;
            }else if(send(sd, msg, strlen(msg), 0) <= 0){
                perror("[client]Eroare la send\n");
                errno;
            }

        }

    }

    close (sd);
}


int recive_Message(int fd, char* buffer){
    int bytes;
    bzero(buffer, sizeof(&buffer));

    //citim lungimea mesajului
    if(read(fd, &bytes, sizeof(bytes)) < 0){
        perror("[client] Error read length from server\n");
        return -1;
    }

    //citim mesajul
    if(bytes > 0){
        if(read(fd, buffer, bytes) < 0){
            perror("[client] Error read() from server\n");
            return -1;
        }
    }
    printf("%s", buffer);

    //printf ("[client]Am citit: %d bytes: %s\n", bytes, buffer);
    return bytes;
}

