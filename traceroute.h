#ifndef TRACEROUTE_H
#define TRACEROUTE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>

#define PID_MASK 0xFFFF
#define PACKET_SIZE (sizeof(struct icmphdr) + sizeof(struct timeval))


//pentru afisare:
#define CLR_RESET  "\033[0m"
#define CLR_STBL     "\033[0;32m" //verde
#define CLR_UNSTBL     "\033[0;33m" //galben
#define CLR_ERR    "\033[0;31m" //rosu

//traceroute configuration structure
struct trace_config{
    char dest_ip[16];
    int max_ttl;
    int timeout_ms;
    int interval_ms;
    int probes_per_ttl;
    int cycle;
};

struct trace_hop_data{
    char router_ip[16];
    long last_rtt;
    long sum_rtt;
    int send;
    int received;
    bool reached;   //in cazul in care s-a ajuns la destinatie o vom seta ca true
};

//Checksum function for ICMP packets
//fac un checksum pe 16 biti (antetul ICMP)
unsigned short check_Sum(void *b, int len);

//send probe probe with given TTL
//construiesc pachetul ICMP ECHO REQUEST si il trimit
int send_Probe(int sd, const char *dest_ip, int ttl, int seq);

//ascult/filtrez raspunsurile ICMP primite
int recive_Reply(int sd, int pid, int seq, char *router_ip, int timeout_ms);

//functia de traceroute
int traceroute(int fd, const char *dest_ip, int max_ttl, int timeout_ms, int interval_ms, int probes_per_ttl);

//crearea si afisarea tabelului cu datele traceului pe ecran
int print_trace_table(int fd, struct trace_hop_data* hops_data, int max_ttl);

#endif // TRACEROUTE_H