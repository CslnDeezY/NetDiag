#ifndef TRACEROUTE_H
#define TRACEROUTE_H
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdbool.h>

struct trace{
    char ip[64];
    int ttl;
    double rtt;
    bool success;
};

//Checksum function for ICMP packets
//fac un checksum pe 16 biti (antetul ICMP)
unsigned short check_Sum(void *b, int len);

//send probe probe with given TTL
//construiesc pachetul ICMP ECHO REQUEST si il trimit
int send_Probe(int sd, const char *dest_ip, int ttl, int seq);

//ascult/filtrez raspunsurile ICMP primite
int recive_Reply(int sd, int pid, int seq, char *router_ip, long *rtt_ms);

//functia de traceroute
int traceroute(const char *dest_ip, int max_ttl, int timeout_ms, int interval_ms, int probes_per_ttl);

#endif // TRACEROUTE_H