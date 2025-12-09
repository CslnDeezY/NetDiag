#include "command.h"
#include "traceroute.h"

extern int errno;
#define message_len 512
#define PORT 2728




//trimite un mesaj catre client:
int send_Message( int fd, char* message );
int recive_Message(int fd, char* buffer);
//preiau/executam comenzile:
void command_Executor(int fd, struct Command cmd);

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

    signal(SIGPIPE, SIG_IGN);

    //creez socket
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[server] Error: socket()\n");
        return errno;
    }

    //setam SO_REUSEADDR
    if( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1){
        perror("[server] Error: socketsopt()\n");
        return errno;
    }

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

            client = accept(sd, (struct sockaddr *) &from, (socklen_t *) &len);
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
                /*===============IMP==========================*/
                char buffer[message_len];
                bzero(buffer, sizeof(buffer));

                int bytesRead = read(fd, buffer, message_len);
                //int bytesRead = recive_Message(fd, buffer);
                if(bytesRead < 0){
                    perror("[server] Error: read()\n");
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
                        command_Executor(fd,cmd);
                    }else {
                        printf("[server] Comanda primita este invalida.\n");
                        if(send_Message(fd,cmd.errorMsg) < 0){
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
    char msg_answ[message_len];        //mesaj de raspuns pentru client

    //preg mesajul 
    memset(msg_answ, 0, message_len);
    strncpy(msg_answ, message, message_len - 1);
    msg_answ[message_len - 1] = '\0';
    bytes = strlen(msg_answ);

    //printf("D: [server]Trimitem mesajul inapoi...%s\n",msg_answ);   
    if(bytes <= 0){
        perror ("[server] Error strlen() \n");
        return -1;
    }
    if(write(fd, &bytes, sizeof(bytes)) < 0){
        perror("[server] Error write length to client\n");
        return -1;
    }
    
    if (bytes && write(fd, msg_answ, bytes) < 0)
    {
        perror ("[server] Error write() to client\n");
        return -1;
    }

    return bytes;
}
int recive_Message(int fd, char* buffer){
    int bytes = 0;
    bzero(buffer, message_len);

    //citim lungimea mesajului
    int n = 0;
    n = read(fd, &bytes, sizeof(bytes));

    if(n < 0){
        perror ("[server] Error read length from client\n");
        return -1;
    }else if(n == 0){
        //conexiunea s-a inchis
        return 0;
    }
    if(bytes <= 0 || bytes > message_len){
        perror ("[server] Error: invalid message length\n");
        return -1;
    }else {
        //citim mesajul
        n = read(fd, buffer, bytes);
        if(n < 0){
            perror ("[server] Error read() from client\n");
            return -1;
        }
        else if(n == 0){
            //conexiunea s-a inchis
            return 0;
        }

    }

    buffer[bytes] = '\0';
    printf ("[server]Am citit: %d bytes: %s\n", bytes, buffer);

    return bytes;
}

//preiau/executam comenzile:
void command_Executor (int fd, struct Command cmd){
    switch(cmd.type){
        case CMD_SET_DEST:
            printf("[server] Execut comanda SET DEST cu arg %s", cmd.args);
            send_Message(fd, "Comanda SET DEST inca nu este implementata\n");
            break;
        case CMD_SET_MAXTTL:
            printf("[server] Execut comanda SET MAXTTL cu arg %s", cmd.args);
            send_Message(fd, "Commanda SET MAXTTL inca nu este implementata\n");
            break;
        case CMD_SET_INTERVAL:
            printf("[server] Execut comanda SET INTERVAL cu arg %s", cmd.args);
            send_Message(fd, "Commanda SET INTERVAL inca nu este implementata\n");
            break;
        case CMD_SET_TIMEOUT:
            printf("[server] Execut comanda SET TIMEOUT cu arg %s", cmd.args);
            send_Message(fd, "Commanda SET TIMEOUT inca nu este implementata\n");
            break;
        case CMD_SET_PROBES:
            printf("[server] Execut comanda SET PROBES cu arg %s", cmd.args);
            send_Message(fd, "Commanda SET PROBES inca nu este implementata\n");
            break;
        case CMD_START:
            printf("[server] Execut comanda START");
            send_Message(fd, "Commanda START inca nu este implementata\n");
            break;
        case CMD_STOP:
            printf("[server] Execut comanda STOP");
            send_Message(fd, "Commanda STOP inca nu este implementata\n");
            break;
        case CMD_RESET:
            printf("[server] Execut comanda RESET");
            send_Message(fd, "Commanda RESET inca nu este implementata\n");
            break;
        case CMD_REPORT:
            printf("[server] Execut comanda REPORT");
            send_Message(fd, "Commanda REPORT inca nu este implementata\n");
            break;
        case CMD_HELP:
            printf("[server] Execut comanda HELP");
            send_Message(fd, "Commanda HELP inca nu este implementata\n");
            break;
        case CMD_QUIT:
            printf("[server] Execut comanda QUIT");
            send_Message(fd, "Commanda QUIT inca nu este implementata\n");
            break;
        case CMD_INVALID:
            printf("[server] Comanda invalida");
            break;
        default:
            printf("[server] Comanda invalida");
            break;
    }
}