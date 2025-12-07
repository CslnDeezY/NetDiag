#include "traceroute.h"

unsigned short check_Sum(void *b, int len){
    
}

int send_Probe(int sd, struct sockaddr_in *addr, int ttl, int seq){

}

int recive_Reply(int sd, int pid, int seq, char *router_ip, long *rtt_ms){

}

int traceroute(const char *dest_ip, int max_ttl, int timeout_ms, int interval_ms, int probes_per_ttl){
    
}