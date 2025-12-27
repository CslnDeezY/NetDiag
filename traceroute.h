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
#include <pthread.h>

#define PID_MASK 0xFFFF //masca pentru PID in pachetul ICMP
#define PACKET_SIZE (sizeof(struct icmphdr) + sizeof(struct timeval))


//pentru afisare:
#define CLR_RESET  "\033[0m"
#define CLR_STBL   "\033[0;32m" //verde
#define CLR_UNSTBL "\033[0;33m" //galben
#define CLR_ERR    "\033[0;31m" //rosu

//traceroute configuration structure
struct trace_config{
    //campuri cu parametri pentru tracereoute
    char dest_ip[64];
    int max_ttl;
    int timeout_ms;
    int interval_ms;
    int probes_per_ttl;
    int cycle;

    //campuri pentru gestionarea corecta a traceroute-ului in firele de executie
    int client_fd;          //socketul clientului unde trimitem rezultatele
    pthread_t thread_id;    //id-ul firului de executie
    pthread_mutex_t mutex;  //mutex pentru sincronizare
    pthread_cond_t cond;    //conditie pentru sincronizare

    bool thread_exists;     //arata daca thread-ul a fost creat
    bool is_running;        //arata daca exista un trace activ pentru client
    bool want_report;       //semnal pentru generarea unui raport
    bool want_stop;         //semnal pentru oprirea trace-ului
    bool want_reset;        //semnal pentru resetarea trace-ului
    bool want_exit;         //semnal pentru terminarea thread-ului

};

struct trace_hop_data{
    char router_ip[64];
    int last_rtt;
    float sum_rtt;
    int send;
    int received;
    bool reached;   //in cazul in care s-a ajuns la destinatie o vom seta ca true
};

//salvam datele pentru report
struct trace_data{

};

struct trace_config default_trace_config(int client);

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

//functia thread-ului de traceroute
void* traceroute_thread(void *arg);

//run trace citind flagurile din structura de configurare
int trace_run(struct trace_config* client_config);

//ruleaza un trace si afiseaza rezultatele la finalul acestuia
int traceroute_raport(struct trace_config* client_config);

//trimite raportul clientului
int send_raport(int fd, struct trace_hop_data* hops_data, int max_ttl);

//crearea si afisarea tabelului cu datele traceului pe ecran
int print_trace(int fd, struct trace_hop_data* hops_data, int max_ttl);

#endif // TRACEROUTE_H