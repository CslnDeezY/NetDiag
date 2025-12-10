#include "command.h"


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