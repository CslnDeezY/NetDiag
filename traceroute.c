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
    if(setsockopt(sd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0){
        perror("D: Error: setsockopt()\n");
        return -1;
    }

    //prepar ICMP header
    char packet[PACKET_SIZE];
    struct icmphdr *icmp = (struct icmphdr*)packet;
    bzero(icmp, sizeof(struct icmphdr) + sizeof(struct timeval));

    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = htons(getpid() & PID_MASK);
    icmp->un.echo.sequence = htons(seq);

    gettimeofday((struct timeval *)(packet + sizeof(struct icmphdr)), NULL);

    icmp->checksum = 0;
    icmp->checksum = check_Sum((unsigned short *)icmp, PACKET_SIZE);

    int sent = sendto(sd, &icmp, sizeof(icmp), 0, (struct sockaddr*)&to_addr, sizeof(to_addr));
    if(sent < 0){
        perror("D: sendto() error\n");
        return -1;
    }
    printf("D: Sent ICMP probe to %s with TTL=%d, seq=%d\n", dest_ip, ttl, seq);
    return sent;
}

int recive_Reply(int sd, int pid, int seq, char *router_ip, long *rtt_ms, int timeout_ms){
    fd_set read_fds;
    struct timeval tm_out;
    struct timeval start, end;
    tm_out.tv_sec = timeout_ms / 1000;
    tm_out.tv_usec = (timeout_ms % 1000) * 1000;

    FD_ZERO(&read_fds);
    FD_SET(sd, &read_fds);

    //gettimeofday(&start, NULL);
    //trimitem pachetul ICMP_ADDRESS
    int rt = select(sd + 1, &read_fds, NULL, NULL, &tm_out);
    if(rt < 0){
        perror("D: Erroro: select()\n");
        return -1;
    }else if(rt == 0)  printf("D: select() timeout\n"); return 0;

    //citesc pachet ICMP
    char buff[2048];
    struct sockaddr_in from_addr;
    socklen_t from_sk_len = sizeof(from_addr);

    int bytes = recvfrom(sd, buff, sizeof(buff), 0, (struct sockaddr*)&from_addr, &from_sk_len);
    if(bytes < 0){
        perror("D: Eror recvfrom()\n");
        return -1;
    }

    gettimeofday(&end, NULL);

    //extrag headerul ip exterior
    struct iphdr *ip_hdr = (struct iphdr*)buff;
    int ip_header_len = ip_hdr->ihl * 4;

    //extrag headerul ICMP
    if(bytes < ip_header_len + sizeof(struct icmphdr)){
        printf("D: Packet too short (%d bytes) \n", bytes);
        return -1;
    }
    struct icmphdr *icmp_hdr = (struct icmphdr*)(buff + ip_header_len);

    int expected_id = pid & PID_MASK;
    int received_id = 0;
    int received_seq = 0;

    //procesare:
    if(icmp_hdr->type == ICMP_TIME_EXCEEDED || icmp_hdr->type == ICMP_DEST_UNREACH){
        //raspunsul de la router
        //parsam antetul icmp original (incepe dupa 8 octeti )
        struct iphdr *org_ip_hdr = (struct iphdr*)(buff + ip_header_len + sizeof(struct icmphdr));
        int org_ip_hdr_len = org_ip_hdr->ihl * 4;

        //paket original
        struct icmphdr *org_icmp_hdr = (struct icmphdr*)(buff + ip_header_len + sizeof(struct icmphdr) + org_ip_hdr_len);
        //extragere ID seq timestamp
        received_id = ntohs(org_icmp_hdr->un.echo.id);
        received_seq = ntohs(org_icmp_hdr->un.echo.sequence);
        start = *(struct timeval *)((char*)org_icmp_hdr + sizeof(struct icmphdr));
    }
    else if(icmp_hdr->type == ICMP_ECHOREPLY){
        //raspunsul de la destinatie
        received_id = ntohs(icmp_hdr->un.echo.id);
        received_seq = ntohs(icmp_hdr->un.echo.sequence);
        start = *(struct timeval *)((char*)icmp_hdr + sizeof(struct icmphdr));
    } else return -1;

    //verificare id si seq
    if(received_id == expected_id || received_id == seq){
        printf("D: Am primit ICMP reply de la:%s, type=%d, code=%d, seq=%d\n", inet_ntoa(from_addr.sin_addr), icmp_hdr->type, icmp_hdr->code, received_seq);
        *rtt_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
        strcpy(router_ip, inet_ntoa(from_addr.sin_addr));
        return icmp_hdr->type;
    }
    else{
        printf("D: am primit / m-am shocat ID/seq (id=%d, seq=%d)\n", received_id, received_seq);
    }

    return -1; //pachet invalid


}

int traceroute(const char *dest_ip, int max_ttl, int timeout_ms, int interval_ms, int probes_per_ttl){
    //implementarea urmeaza sa fie facuta 
    return 0;
}