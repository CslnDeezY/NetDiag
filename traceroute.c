#include "traceroute.h"

unsigned short check_Sum(void *b, int len){
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for(sum = 0; len > 1; len-=2){
        sum += *buf++;
    }
    if(len == 1){
        sum += *(unsigned char*)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int send_Probe(int sd, const char *dest_ip, int ttl, int seq){
    struct sockaddr_in to_addr;
    bzero(&to_addr, sizeof(to_addr));
    to_addr.sin_family = AF_INET;
    to_addr.sin_addr.s_addr = inet_addr(dest_ip);

    //set ttl
    if(setsockopt(sd, IPPROTO_IP, IP_TTL, sizeof(ttl), &ttl) < 0){
        perror("D: setsockopt() error\n");
        return -1;
    }

    //prepar ICMP packet
    struct icmphdr icmp;
    icmp.type = ICMP_ECHO;
    icmp.code = 0;
    icmp.un.echo.id = getpid() & 0xFFFF;
    icmp.un.echo.sequence = seq;
    icmp.checksum = 0;
    icmp.checksum = check_Sum(&icmp, sizeof(icmp));

    int sent = sendto(sd, &icmp, sizeof(icmp), 0, (struct sockaddr*)&to_addr, sizeof(to_addr));
    if(sent < 0){
        perror("D: sendto() error\n");
        return -1;
    }
    print("D: Sent ICMP probe to %s with TTL=%d, seq=%d\n", dest_ip, ttl, seq);
    return sent;
}

int recive_Reply(int sd, int pid, int seq, char *router_ip, long *rtt_ms, int timeout_ms){

}

int traceroute(const char *dest_ip, int max_ttl, int timeout_ms, int interval_ms, int probes_per_ttl){
    
}