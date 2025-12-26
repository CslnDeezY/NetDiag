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

//traceroute configuration structure
struct trace_config{
    char dest_ip[16];
    int max_ttl;
    int timeout_ms;
    int interval_ms;
    int probes_per_ttl;
    int cycle;
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

#endif // TRACEROUTE_H