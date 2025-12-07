#include "command.h"


//parsam comanda primita de la client si o stocam in structura Command
struct Command parse_Command(const char* commandStr){
    struct Command cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_INVALID;
    cmd.isValid = false;

    //analizam inputul 
    char buffer[MESSAGE_LEN];  // Now uses compile-time constant
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
                    cmd.isValid = true;
                    return cmd;
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
                    cmd.isValid = true;
                    return cmd;
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
                    cmd.isValid = true;
                    return cmd;
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
                    cmd.isValid = true;
                    return cmd;
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
                    cmd.isValid = true;
                    return cmd;
                }else {
                    //lipsa arg
                    cmd.isValid = false;
                    strcpy(cmd.errorMsg, "Missing argument for 'set probes'");
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
    }else if(strcmp(token, "schedule") == 0){
        //
        token = strtok(NULL, " \n");
        if(strcmp(token, "show") == 0){
            cmd.type = CMD_SCHEDULE_SHOW;
            if((token = strtok(NULL, " \n")) != NULL){
                cmd.isValid = false;
                strcpy(cmd.errorMsg, "Too many arguments for 'schedule show'");
                return cmd;
            }else {
                cmd.isValid = true;
                return cmd;
            }
        }
    }else if(strcmp(token, "unschedule") == 0){
        //
        token = strtok(NULL, " \n");
        if(strcmp(token, "show") == 0){
            cmd.type = CMD_UNSCHEDULE_SHOW;
            if((token = strtok(NULL, " \n")) != NULL){
                cmd.isValid = false;
                strcpy(cmd.errorMsg, "Too many arguments for 'unschedule show'");
                return cmd;
            }else {
                cmd.isValid = true;
                return cmd;
            }
        }
        return cmd;
    }else if(strcmp(token, "help") == 0){
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


//preiau/executam comenzile:
void command_Executor (struct Command cmd){
    switch(cmd.type){
        case CMD_SET_DEST:
            printf("[server] Execut comanda SET DEST cu arg %s", cmd.args);
            break;
        case CMD_SET_MAXTTL:
            printf("[server] Execut comanda SET MAXTTL cu arg %s", cmd.args);
            break;
        case CMD_SET_INTERVAL:
            printf("[server] Execut comanda SET INTERVAL cu arg %s", cmd.args);
            break;
        case CMD_SET_TIMEOUT:
            printf("[server] Execut comanda SET TIMEOUT cu arg %s", cmd.args);
            break;
        case CMD_SET_PROBES:
            printf("[server] Execut comanda SET PROBES cu arg %s", cmd.args);
            break;
        case CMD_START:
            printf("[server] Execut comanda START");
            break;
        case CMD_STOP:
            printf("[server] Execut comanda STOP");
            break;
        case CMD_RESET:
            printf("[server] Execut comanda RESET");
            break;
        case CMD_REPORT:
            printf("[server] Execut comanda REPORT");
            break;
        case CMD_SCHEDULE_SHOW:
            printf("[server] Execut comanda SCHEDULE SHOW");
            break;
        case CMD_UNSCHEDULE_SHOW:
            printf("[server] Execut comanda UNSCHEDULE SHOW");
            break;
        case CMD_HELP:
            printf("[server] Execut comanda HELP");
            break;
        case CMD_QUIT:
            printf("[server] Execut comanda QUIT");
            break;
        case CMD_INVALID:
            printf("[server] Comanda invalida");
            break;
        default:
            printf("[server] Comanda invalida");
            break;
    }
}