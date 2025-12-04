#ifndef COMMAND_H
#define COMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
    CMD_SCHEDULE_SHOW, 
    CMD_UNSCHEDULE_SHOW,
    CMD_HELP, 
    CMD_QUIT
} CommandType;

struct Command{
    CommandType type;
    char args[64];
    bool isValid;
    char errorMsg[64];
};

//parsam comanda primita de la client si o stocam in structura Command
struct Command parse_Command(const char* commandStr);

//preiau/executam comenzile:
void command_Executor (struct Command cmd);



#endif // COMMAND_H