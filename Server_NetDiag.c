#include <arpa/inet.h>
//#include <ctype.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <strings.h>
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

#include "command.c"

extern int errno;
#define PORT 2728



//trimite un mesaj catre client:
int send_Message( int fd, char* message );

//IP to string
char *conv_Addr(struct sockaddr_in address) {
    static char str[25];
    char port[7];

    //IP client
    strcpy (str, inet_ntoa (address.sin_addr));
    //port client
    bzero (port, 7);
    sprintf (port, ":%d", ntohs (address.sin_port));
    strcat (str, port);
    return (str);
}


int main(int argc, char* argv[]){

    struct sockaddr_in server;	    //struct server - client
    struct sockaddr_in from;
    fd_set readfds;                 //multimea descriptorilor de citire
    fd_set actfds;                  //mult fdesc activi
    struct timeval tv;		        // structura de timp pentru select()
    int sd, client;                 //descriptori sock lucru
    int optval=1;                   //pt setsockopt

    int nfds;			            //nr maxim
    int len;			            //lungimea structurii sockaddr_in

    //creez socket
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[server] Error: socket()\n");
        return errno;
    }

    //setam SO_REUSEADDR
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    //prepar structurile:
    bzero (&server, sizeof (server));
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    server.sin_port = htons (PORT);
    server.sin_family = AF_INET;

    //atasez:
    if( bind(sd, (struct sockaddr *) &server, sizeof(server)) == -1){
        perror("[server] Error: bind()\n");
        return errno;
    }
    if (listen (sd, 16) == -1)
    {
        perror ("[server] Error: listen()\n");
        return errno;
    }

    //completam mult de descriptori:
    FD_ZERO(&actfds);         //initializare multime fdesc activi
    FD_SET(sd, &actfds);      //adaug sd
    nfds = sd;

    //structura timeval pt select:
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    printf("[server] Wait at port: %d.........\n", PORT);
    fflush(stdout);

    //abslujivaiem tioti clientii
    while(true){

        //ajust mult desc activi
        bcopy ( (char*) &actfds, (char*) &readfds, sizeof(readfds));

        if(select(nfds+1, &readfds, NULL, NULL, &tv) < 0){
            perror("[server] Error: select()\n");
            return errno;
        }

        //verific daca socket e moral gata:
        if(FD_ISSET(sd, &readfds)){

            //preg structura
            len = sizeof(from);
            bzero(&from, sizeof(from));

            client = accept(sd, (struct sockaddr *) &from, &len);
            if(client < 0){
                perror("[server] Error: accept()\n");
                return errno;
            }

            if(client > nfds){
                nfds = client;
            }

            //adaug la lista de socketi activi:
            FD_SET(client, &actfds);

            printf("[server] S-a conectat clientul cu descriptorul %d, de la adresa %s.\n",client, conv_Addr (from));
            fflush (stdout);
        }

        //////verific daca este gata sa primeasca cineva rs
        for(int fd = 0; fd <= nfds; fd++){
            //exist socket de citire gata
            if( fd != sd && FD_ISSET(fd, &readfds)){
                /*==============================================================IMP=====================================*/
                char buffer[516];
                bzero(buffer, sizeof(buffer));

                int bytesRead = recv(fd, buffer, sizeof(buffer)-1, 0);
                if(bytesRead < 0){
                    perror("[server] Error: recv()\n");
                    return errno;
                }else if(bytesRead == 0){
                    //conexiunea s-a inchis
                    printf("[server] S-a deconectat clientul cu descriptorul %d, de la adresa %s.\n",fd, conv_Addr (from));
                    fflush (stdout);
                    close(fd);
                    FD_CLR(fd, &actfds);
                }else {
                    //am primit date
                    buffer[bytesRead] = '\0';
                    printf("[server] Am primit de la clientul cu descriptorul %d comanda: %s", fd, buffer);
                    //parsam comanda
                    struct Command cmd = parse_Command(buffer);
                    if(cmd.isValid){
                        command_Executor(cmd);
                        if(send_Message(fd,"Command executed successfully") < 0){
                            perror("[server] Error: send_Message()\n");
                            return errno;
                        }
                    }else {
                        printf("[server] Comanda primita este invalida.\n");
                        if(send_Message(fd,"Invalid command") < 0){
                            perror("[server] Error: send_Message()\n");
                            return errno;
                        }
                    }
                    fflush(stdout);

                }
                
            }

        }//end for de parcurgere a descriptorilor

    }   //end while(true)


}   //end manin


int send_Message( int fd, char* message ){
    int bytes;			
    char msg_answ[100]="";        //mesaj de raspuns pentru client

    //preg mesajul 
    bzero(msg_answ,100);

    strcpy(msg_answ, message);
    bytes = strlen (msg_answ);

    //printf("D: [server]Trimitem mesajul inapoi...%s\n",msg_answ);   
    if(bytes <= 0){
        perror ("[server] Error strlen() \n");
        return -1;
    }
    if(write(fd, &bytes, sizeof(bytes)) < 0){
        perror("[server] Error write length to client\n");
        return -1;
    }
    
    if (bytes && write (fd, msg_answ, bytes) < 0)
    {
        perror ("[server] Error write() to client\n");
        return -1;
    }

    return bytes;
}