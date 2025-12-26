#include "command.h"

extern int errno;
#define message_len 512
#define PORT 3686

//tablou de configurari pentru fiecare client
struct trace_config client_configs[FD_SETSIZE];

void set_default_config(int fd_client){
    strcpy(client_configs[fd_client].dest_ip, "127.0.0.1"); //localhost
    client_configs[fd_client].max_ttl = 30;
    client_configs[fd_client].timeout_ms = 3000;      //3 secunde
    client_configs[fd_client].interval_ms = 1000;     //1 secunda
    client_configs[fd_client].probes_per_ttl = 3;
    client_configs[fd_client].cycle = 1;
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

    printf("[server] Wait at port: %d...\n", PORT);
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

            set_default_config(client); //setez config default pt client (la fiecare client nou aceasta se reseteaza la valorile default indiferent de fd)
            if(client > nfds){
                nfds = client;
            }

            //adaug la lista de socketi activi:
            FD_SET(client, &actfds);

            printf("[server] The client with descriptor %d, from address %s, has connected\n",client, conv_Addr (from));
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
                    printf("[server] The client with descriptor %d disconnected from address %s\n",fd, conv_Addr (from));
                    fflush (stdout);
                    close(fd);
                    FD_CLR(fd, &actfds);
                }else {
                    //am primit date
                    buffer[bytesRead] = '\0';
                    printf("[server] Received command from client with descriptor %d: %s", fd, buffer);

                    //parsam comanda
                    struct Command cmd = parse_Command(buffer);
                    if(cmd.isValid){
                        //daca comanda este quit trebuie sa actualizam mult de descriptori actfds
                        bool isQuit = (cmd.type == CMD_QUIT);

                        command_Executor(fd,cmd, &client_configs[fd]);

                        if(isQuit){
                            printf("Debug: [serer] stergem descriptorul %d din multimea de descriptori activi\n", fd);
                            FD_CLR(fd, &actfds);
                            close(fd);
                        }
                    }else {
                        printf("[server] Invalid command\n");
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

