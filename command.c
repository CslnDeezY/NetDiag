#include "command.h"

//functiile de trimitere si receptare de mesaje
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

//aceasta functie nu este utilizata
/*
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
*/


//parsam comanda primita de la client si o stocam in structura Command
struct Command parse_Command(const char* commandStr){
    struct Command cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_INVALID;
    cmd.isValid = false;

    //analizam inputul 
    char buffer[message_len];  // Now uses compile-time constant
    strncpy(buffer, commandStr, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    //analizamcomanda primita de la client si o stocam in structura P_Command
    char *token = strtok(buffer, " \n");
    if(token == NULL){
        perror("[server] Eroare ParseCommand: comanda goala\n");
        cmd.isValid = false;
        strcpy(cmd.errorMsg, "Command is NULL");
        return cmd;
    }

    if(strcmp(token, "set") == 0){
        token = strtok(NULL, " \n");

        if(token == NULL){
            perror("[server] Eroare ParseCommand: comanda SET incompleta\n");
            cmd.isValid = false;
            strcpy(cmd.errorMsg, "Incomplet set command");
            return cmd;
        }else {
            if(strcmp(token, "dest") == 0){
                //SET DEST <IP> Seteaza destinatia traceroute-ului
                cmd.type = CMD_SET_DEST;
                if((token = strtok(NULL, " \n")) != NULL){
                    strcpy(cmd.args, token);
                    if((token = strtok(NULL, " \n")) != NULL){
                        //prea multi arg
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Too many arguments for 'set dest'");
                        return cmd;
                    }
                    if(!validate_ip(cmd.args)) {
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Invalid IP address format");
                        return cmd;
                    }else {
                        cmd.isValid = true;
                        return cmd;
                    }
                }else {
                    //lipsa arg
                    cmd.isValid = false;
                    strcpy(cmd.errorMsg, "Missing argument for 'set dest'");
                    return cmd;
                }
            }else if(strcmp(token, "maxttl") == 0){
                //Seteaza TTL maxim
                cmd.type = CMD_SET_MAXTTL;
                if((token = strtok(NULL, " \n")) != NULL){
                    strcpy(cmd.args, token);
                    if((token = strtok(NULL, " \n")) != NULL){
                        //prea multi arg
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Too many arguments for 'set maxttl'");
                        return cmd;
                    }
                    if(!validate_maxttl(cmd.args)) {
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Max TTL must be between 1 and 64");
                        return cmd;
                    }else {
                        cmd.isValid = true;
                        return cmd;
                    }
                }else {
                    //lipsa arg
                    cmd.isValid = false;
                    strcpy(cmd.errorMsg, "Missing argument for 'set maxttl'");
                    return cmd;
                }
            }else if(strcmp(token, "interval") == 0){
                //SET INTERVAL <secunde> Intervalul de actualizare
                cmd.type = CMD_SET_INTERVAL;
                if((token = strtok(NULL, " \n")) != NULL){
                    strcpy(cmd.args, token);
                    if((token = strtok(NULL, " \n")) != NULL){
                        //prea multi arg
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Too many arguments for 'set interval'");
                        return cmd;
                    }
                    if(!validate_interval(cmd.args)) {
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Interval must be between 1 and 3600 seconds");
                        return cmd;
                    }else {
                        cmd.isValid = true;
                        return cmd;
                    }
                }else {
                    //lipsa arg
                    cmd.isValid = false;
                    strcpy(cmd.errorMsg, "Missing argument for 'set interval'");
                    return cmd;
                }
            }else if(strcmp(token, "timeout") == 0){
                //SET TIMEOUT <ms> Timeout pe hop
                cmd.type = CMD_SET_TIMEOUT;
                if((token = strtok(NULL, " \n")) != NULL){
                    strcpy(cmd.args, token);
                    if((token = strtok(NULL, " \n")) != NULL){
                        //prea multi arg
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Too many arguments for 'set timeout'");
                        return cmd;
                    }
                    if(!validate_timeout(cmd.args)) {
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Timeout must be between 10 and 5000 ms");
                        return cmd;
                    }else{
                        cmd.isValid = true;
                        return cmd;
                    }
                }else {
                    //lipsa arg
                    cmd.isValid = false;
                    strcpy(cmd.errorMsg, "Missing argument for 'set timeout'");
                    return cmd;
                }
            }else if(strcmp(token, "probes") == 0){
                //SET PROBES <număr> Numărul de pachete trimise la fiecare actualizare
                cmd.type = CMD_SET_PROBES;
                if((token = strtok(NULL, " \n")) != NULL){
                    strcpy(cmd.args, token);
                    if((token = strtok(NULL, " \n")) != NULL){
                        //prea multi arg
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Too many arguments for 'set probes'");
                        return cmd;
                    }
                    if(!validate_probes(cmd.args)) {
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Probes must be between 1 and 10");
                        return cmd;
                    }else{
                        cmd.isValid = true;
                        return cmd;
                    }
                }else {
                    //lipsa arg
                    cmd.isValid = false;
                    strcpy(cmd.errorMsg, "Missing argument for 'set probes'");
                    return cmd;
                }
            }else if(strcmp(token, "cycle") == 0){
                //set cycle <numar> seteaza numarul de cicluri pentru report
                cmd.type = CMD_SET_CYCLE;
                if((token = strtok(NULL, " \n")) != NULL){
                    strcpy(cmd.args, token);
                    if((token = strtok(NULL, " \n")) != NULL){
                        //prea multi arg
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Too many arguments for 'set cycle'");
                        return cmd;
                    }
                    if(!validate_cycle(cmd.args)) {
                        cmd.isValid = false;
                        strcpy(cmd.errorMsg, "Cycle must be between 1 and 64");
                        return cmd;
                    }else{
                        cmd.isValid = true;
                        return cmd;
                    }
                }else {
                    //lipsa arg
                    cmd.isValid = false;
                    strcpy(cmd.errorMsg, "Missing argument for 'set cycle'");
                    return cmd;
                }
            }
        }
    } //end if "SET"

    if(strcmp(token, "start") == 0){
        //
        cmd.type = CMD_START;
        if((token = strtok(NULL, " \n")) != NULL){
            //prea multi arg
            cmd.isValid = false;
            strcpy(cmd.errorMsg, "Too many arguments for 'start'");
            return cmd;
        }else {
            cmd.isValid = true;
            return cmd;
        }
    }else if(strcmp(token, "stop") == 0){
        //
        cmd.type = CMD_STOP;
        if((token = strtok(NULL, " \n")) != NULL){
            //prea multi arg
            cmd.isValid = false;
            strcpy(cmd.errorMsg, "Too many arguments for 'stop'");
            return cmd;
        }else {
            cmd.isValid = true;
            return cmd;
        }
    }else if(strcmp(token, "reset") == 0){
        //
        cmd.type = CMD_RESET;
        if((token = strtok(NULL, " \n")) != NULL){
            //prea multi arg
            cmd.isValid = false;
            strcpy(cmd.errorMsg, "Too many arguments for 'reset'");
            return cmd;
        }else {
            cmd.isValid = true;
            return cmd;
        }
    }else if(strcmp(token, "report") == 0){
        //
        cmd.type = CMD_REPORT;
        if((token = strtok(NULL, " \n")) != NULL){
            //prea multi arg
            cmd.isValid = false;
            strcpy(cmd.errorMsg, "Too many arguments for 'report'");
            return cmd;
        }else {
            cmd.isValid = true;
            return cmd;
        }
    }
    /* aici a fost schedule*/
    
    else if(strcmp(token, "help") == 0){
        //help
        cmd.type = CMD_HELP;
        cmd.isValid = true;
        return cmd;
    }else if(strcmp(token, "quit") == 0){
        //quit
        cmd.type = CMD_QUIT;
        if((token = strtok(NULL, " \n")) != NULL){
            //prea multi arg
            cmd.isValid = false;
            strcpy(cmd.errorMsg, "Too many arguments for 'quit'");
            return cmd;
        }else {
            cmd.isValid = true;
            return cmd;
        }
    }
    cmd.isValid = false;
    strcpy(cmd.errorMsg, "Unknown command");
    return cmd;
} //end functie parseCommand

//functiile de validare a argumentelor pentru fiecare comanda
bool validate_ip(const char *arg){
    struct sockaddr_in sa;
    return inet_pton(AF_INET, arg, &(sa.sin_addr)) == 1;
}
bool validate_maxttl(const char *arg){
    int i;
    for (i = 0; arg[i] != '\0'; i++) {
        if (!isdigit(arg[i])) {
            return false;  
        }
    }
    int ttl = atoi(arg);
    if (ttl < 1 || ttl > 64)
        return false;
    return true;
}
bool validate_interval(const char *arg){
    int i;
    for(i = 0; arg[i] != '\0'; i++){
        if (!isdigit(arg[i])){
            return false;  
        }
    }
    int sec = atoi(arg);
    if (sec < 1 || sec > 360)
        return false;
    return true;
}
bool validate_timeout(const char *arg){
    int i;
    for(i = 0; arg[i] != '\0'; i++){
        if (!isdigit(arg[i])){
            return false;  
        }
    }
    int ms = atoi(arg);
    if(ms < 10 || ms > 5000)
        return false;
    return true;
}
bool validate_probes(const char *arg){
    int i;
    for(i = 0; arg[i] != '\0'; i++){
        if (!isdigit(arg[i])){
            return false;  
        }
    }
    
    int p = atoi(arg);
    if(p < 1 || p > 10)
        return false;
    return true;
}
bool validate_cycle(const char  *arg){
    int i;
    for(i = 0; arg[i] != '\0'; i++){
        if (!isdigit(arg[i])){
            return false;  
        }
    }
    
    int cycle = atoi(arg);
    if(cycle < 1 || cycle > 64)
        return false;
    return true;
}

//preiau/executam comenzile:
void command_Executor (int fd, struct Command cmd){
    switch(cmd.type){
        case CMD_SET_DEST:
            printf("[server] Execute command SET DEST cu arg %s", cmd.args);
            send_Message(fd, "Comanda SET DEST inca nu este implementata\n");
            break;
        case CMD_SET_MAXTTL:
            printf("[server] Execute command SET MAXTTL cu arg %s", cmd.args);
            send_Message(fd, "Commanda SET MAXTTL inca nu este implementata\n");
            break;
        case CMD_SET_INTERVAL:
            printf("[server] Execute command SET INTERVAL cu arg %s", cmd.args);
            send_Message(fd, "Commanda SET INTERVAL inca nu este implementata\n");
            break;
        case CMD_SET_TIMEOUT:
            printf("[server] Execute command SET TIMEOUT cu arg %s", cmd.args);
            send_Message(fd, "Commanda SET TIMEOUT inca nu este implementata\n");
            break;
        case CMD_SET_PROBES:
            printf("[server] Execute command SET PROBES cu arg %s", cmd.args);
            send_Message(fd, "Commanda SET PROBES inca nu este implementata\n");
            break;
        case CMD_SET_CYCLE:
            printf("[server] Execute command SET CYCLE cu arg %s", cmd.args);
            send_Message(fd, "Commanda SET CYCLE inca nu este implementata\n");
            break;
        case CMD_START:
            printf("[server] Execute command START");
            send_Message(fd, "Commanda START inca nu este implementata\n");
            break;
        case CMD_STOP:
            printf("[server] Execute command STOP");
            send_Message(fd, "Commanda STOP inca nu este implementata\n");
            break;
        case CMD_RESET:
            printf("[server] Execute command RESET");
            send_Message(fd, "Commanda RESET inca nu este implementata\n");
            break;
        case CMD_REPORT:
            printf("[server] Execute command REPORT");
            send_Message(fd, "Commanda REPORT inca nu este implementata\n");
            break;
        case CMD_HELP:
            printf("[server] Execute command HELP");
            //send_Message(fd, "Commanda HELP inca nu este implementata\n");
            execute_help(fd);
            break;
        case CMD_QUIT:
            printf("[server] Execute commandQUIT");
            //send_Message(fd, "Commanda QUIT inca nu este implementata\n");
            execute_quit(fd);
            break;
        case CMD_INVALID:
            //aici nu ar trebui sa ajunga
            printf("[server] How to execute an invalid ommand?\n");
            break;
        default:
            //aici nu ar trebui sa ajunga
            printf("[server] What can I execute?\n");
            break;
    }
}

//functii de implementare a comenzilor
void execute_set_dest(int fd, const char *arg){}
void execute_set_maxttl(int fd, const char *arg){}
void execute_set_interval(int fd, const char *arg){}
void execute_set_timeout(int fd, const char *arg){}
void execute_set_probes(int fd, const char *arg){}
void execute_set_cycle(int fd, const char *arg){}
void execute_start(int fd){}
void execute_stop(int fd){}
void execute_reset(int fd){}
void execute_report(int fd){}
void execute_help(int fd){
    FILE *help_file = fopen("help.txt", "r");
    if(help_file == NULL){
        perror("[server] Help execute: Error opening help.txt\n");
        return;
    }
    char line[512] = "";
    //citesc si afisez fiecare linie
    while(fgets(line, sizeof(line), help_file) != NULL){
        printf("[serve] D: Help execute:%s", line);
        send_Message(fd, line);
        bzero(line, sizeof(line));
    }
    fclose(help_file);
}
void execute_quit(int fd){
    printf("[server] Quit execute: The client with descriptor: %d requests disconnection\n", fd);
    send_Message(fd, "Quit Accepted\n");
    //close(fd);
}