#ifndef COMMAND_H
#define COMMAND_H

#include "traceroute.h"

#define message_len 512


typedef enum{
    CMD_INVALID = -1,
    CMD_SET_DEST = 0,
    CMD_SET_MAXTTL = 1,
    CMD_SET_INTERVAL = 2,
    CMD_SET_TIMEOUT = 3,
    CMD_SET_PROBES = 4,
    CMD_SET_CYCLE,
    CMD_START,
    CMD_STOP,
    CMD_RESET,
    CMD_REPORT,
    CMD_HELP, 
    CMD_QUIT
} CommandType;

struct Command{
    CommandType type;
    char arg[64];
    bool isValid;
    char errorMsg[128];
};

//IP to string
char *conv_Addr(struct sockaddr_in address);

//trimite un mesaj catre client:
int send_Message( int fd, char* message );
int recive_Message(int fd, char* buffer);

//parsam comanda primita de la client si o stocam in structura Command
struct Command parse_Command(const char* commandStr);

//functiile de validare a argumentelor pentru fiecare comanda
bool validate_ip        (const char *arg);
bool validate_maxttl    (const char *arg);
bool validate_interval  (const char *arg);
bool validate_timeout   (const char *arg);
bool validate_probes    (const char *arg);
bool validate_cycle     (const char  *arg);
//preiau/executam comenzile:
void command_Executor(int fd, struct Command cmd, struct trace_config* client_config);

//functii de implementare a comenzilor
void execute_set_dest       (int fd, const char *arg, struct trace_config* client_config);
void execute_set_maxttl     (int fd, const char *arg, struct trace_config* client_config);
void execute_set_interval   (int fd, const char *arg, struct trace_config* client_config);
void execute_set_timeout    (int fd, const char *arg, struct trace_config* client_config);
void execute_set_probes     (int fd, const char *arg, struct trace_config* client_config);
void execute_set_cycle      (int fd, const char *arg, struct trace_config* client_config);
void execute_start          (int fd, struct trace_config* client_config);
void execute_stop           (int fd);
void execute_reset          (int fd);
void execute_report         (int fd, struct trace_config* client_config);
void execute_help           (int fd);
void execute_quit           (int fd);


void afisare_date_structura_config(int fd, struct trace_config* client_config);

#endif // COMMAND_H