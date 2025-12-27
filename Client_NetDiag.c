#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

extern int errno;
#define message_len 512
int port;
volatile sig_atomic_t stop = 0;

int recive_Message(int fd, char* buffer);
int send_Message(int sd, char* buffer);
void handler_sigint(int sig);
void reset_terminal_view();
int read_until_quit_ack(int sd, char *msg);

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
    // Instalam handler pentru SIGINT cu sigaction si SA_RESTART
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("[client] Error sigaction()\n");
        close(sd);
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

        if(stop){
            printf("[client] Inchid conexiunea...\n");
            if(send(sd, "quit\n", strlen("quit\n"), 0) <= 0){
                perror("[client]Eroare la send\n");
                errno;
            }
            if(read_until_quit_ack(sd, msg) == 0){
                printf("[client] Conexiunea la server s-a inchis.\n");
                reset_terminal_view();
            } else {
                printf("[client] Raspuns neasteptat de la server: %s mai stau un pic\n", msg);
            }
            break;
        }

        //verific constant daca trimit sau primesc date
        FD_ZERO(&rfds);
        FD_SET(sd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);

        if( sd > STDIN_FILENO )
            maxfd = sd;
        else
            maxfd = STDIN_FILENO;
        
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                // intrerupt de semnal; bucla va verifica 'stop' la inceput
                continue;
            }
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
                if(read_until_quit_ack(sd, msg) == 0){
                    printf("[client] Conexiunea la server s-a inchis.\n");
                    //reset_terminal_view();
                } else {
                    printf("[client] Raspuns neasteptat de la server: %s mai stau un pic\n", msg);
                }
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
    int bytes = 0;
    bzero(buffer, message_len);

    // citim lungimea mesajului
    int n = read(fd, &bytes, sizeof(bytes));
    if(n < 0){
        perror("[client] Error read length from server\n");
        return -1;
    } else if(n == 0){
        // conexiunea s-a inchis
        return 0;
    }

    // citim mesajul
    if(bytes > 0){
        n = read(fd, buffer, bytes);
        if(n < 0){
            perror("[client] Error read() from server\n");
            return -1;
        } else if(n == 0){
            // conexiunea s-a inchis
            return 0;
        }
        if(n < bytes) {
            buffer[n] = '\0';
        } else {
            buffer[bytes] = '\0';
        }
    }

    printf("%s", buffer);
    return bytes;
}

void handler_sigint(int sig){
    stop = 1;
}

void reset_terminal_view(){
    //Revine la ecranul principal, arata cursorul, curata si pozitioneaza la inceput
    printf("\033[?1049l\033[?25h\033[2J\033[H");
    fflush(stdout);
}

//Citeste in bucla mesajele pana gaseste "Quit Accepted" sau conexiunea se inchide
//Ignora output suplimentar de la traceroute Returneaza 0 cand a primit quit ack, altfel -1
int read_until_quit_ack(int sd, char *msg){
    while(true){
        int rc = recive_Message(sd, msg);
        if(rc <= 0){
            //conexiune inchisa sau eroare
            return -1;
        }
        if(strcmp(msg, "Quit Accepted\n") == 0){
            return 0;
        }
        //alt mesaj: continua sa citeasca, poate fi output de traceroute
    }
}
