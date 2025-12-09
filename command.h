#ifndef COMMAND_H
#define COMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>


#define message_len 512


typedef enum{
    CMD_INVALID = -1,
    CMD_SET_DEST = 0,
    CMD_SET_MAXTTL = 1,
    CMD_SET_INTERVAL = 2,
    CMD_SET_TIMEOUT = 3,
    CMD_SET_PROBES = 4,
    CMD_START = 5,
    CMD_STOP = 6,
    CMD_RESET = 7,
    CMD_REPORT,
    CMD_HELP, 
    CMD_QUIT
} CommandType;

struct Command{
    CommandType type;
    char args[64];
    int  arg_valid;
    bool isValid;
    char errorMsg[128];
};

//parsam comanda primita de la client si o stocam in structura Command
struct Command parse_Command(const char* commandStr);

bool validate_ip(const char *arg);
bool validate_maxttl(const char *arg);
bool validate_interval(const char *arg);
bool validate_timeout(const char *arg);
bool validate_probes(const char *arg);




#endif // COMMAND_H