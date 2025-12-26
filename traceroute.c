#include "traceroute.h"

//populeaza structura de configurare cu valori implicite
struct trace_config default_trace_config(){
    struct trace_config cfg;
    strcpy(cfg.dest_ip, "127.0.0.1"); //localhost
    cfg.max_ttl = 30;
    cfg.timeout_ms = 3000;      //3 secunde
    cfg.interval_ms = 1000;     //1 secunda
    cfg.probes_per_ttl = 3;
    cfg.cycle = 1;
    return cfg;
}

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
//functia triite un pachet ICMP echo request cu TTL
int send_Probe(int sd, const char *dest_ip, int ttl, int seq){
    struct sockaddr_in to_addr;
    memset(&to_addr, 0, sizeof(to_addr));
    //set destination address
    to_addr.sin_family = AF_INET;
    to_addr.sin_addr.s_addr = inet_addr(dest_ip);

    //set ttl
    //setez ttl-ul in hederul IP al socket-ului
    //(fortez expirarea acestuia la un anumit hop)
    if(setsockopt(sd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0){
        perror("[server][trace]: Error: setsockopt()\n");
        return -1;
    }

    //prepar ICMP header
    //alocarea memoriei pentru pachet (Header ICMP + Date Payload)
    char packet[PACKET_SIZE];
    memset(packet, 0, PACKET_SIZE);
    struct icmphdr *icmp = (struct icmphdr*)packet;
    //bzero(icmp, sizeof(struct icmphdr) + sizeof(struct timeval));

    //populez campurile 
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = htons(getpid() & PID_MASK); //identificator unic pentru proces
    icmp->un.echo.sequence = htons(seq);            //nr pachetului in secventa

    //calculez checksum-ul
    int packet_len = sizeof(struct icmphdr);

    icmp->checksum = 0;
    icmp->checksum = check_Sum(packet, packet_len);

    //in sfarsit trimit pachetul trimit pachetul  (sper ca merge)
    int sent = sendto(sd, packet, packet_len, 0, (struct sockaddr*)&to_addr, sizeof(to_addr));
    if(sent < 0){
        perror("[server][trace]: sendto() error\n");
        return -1;
    }
    //printf("D: [server][trace]: Sent ICMP probe to %s with TTL=%d, seq=%d\n", dest_ip, ttl, seq);
    return sent;
}


//functia asteapta si proceseaza raspunsurile ICMP
int recive_Reply(int sd, int pid, int seq, char *router_ip, int timeout_ms){
    fd_set read_fds;
    struct timeval tm_out;
    
    while(true){
        //configurez timeout pentru fiecare incercare
        tm_out.tv_sec  = timeout_ms / 1000;
        tm_out.tv_usec = (timeout_ms % 1000) * 1000;

        FD_ZERO(&read_fds);
        FD_SET(sd, &read_fds);

        //asteptam raspuns cu select
        int rt = select(sd + 1, &read_fds, NULL, NULL, &tm_out);
        if(rt < 0){
            perror("[server][trace]: Error: select()\n");
            return -1;
        } else if(rt == 0){
            printf("D: [server][trace]: select() timeout (seq=%d)\n", seq);
            return -2; // timeout, no data ready (avoid clashing with ICMP_ECHOREPLY==0)
            
        }

        char buff[2048];
        struct sockaddr_in from_addr;
        socklen_t from_sk_len = sizeof(from_addr);

        //primesc pachetul in stare cruda
        int bytes = recvfrom(sd, buff, sizeof(buff), 0,
                             (struct sockaddr*)&from_addr, &from_sk_len);
        if(bytes < 0){
            perror("[server][trace]: Error: recvfrom()\n");
            return -1;
        }

        //extrag headerul IP exterior
        struct iphdr *ip_hdr = (struct iphdr*)buff;
        int ip_header_len = ip_hdr->ihl * 4;

        if(bytes < ip_header_len + (int)sizeof(struct icmphdr)) 
            continue;

        struct icmphdr *icmp_hdr = (struct icmphdr*)(buff + ip_header_len);

        int received_id = 0;
        int received_seq = 0;

        //printf("D: [server][trace]: Received ICMP type=%d, code=%d from %s\n", 
        //       icmp_hdr->type, icmp_hdr->code, inet_ntoa(from_addr.sin_addr));

        //procesare:
        if (icmp_hdr->type == ICMP_TIME_EXCEEDED || icmp_hdr->type == ICMP_DEST_UNREACH) {
            if (bytes < ip_header_len + (int)sizeof(struct icmphdr) +
                        (int)sizeof(struct iphdr) + 8) 
                continue;

            struct iphdr *org_ip_hdr =
                (struct iphdr*)(buff + ip_header_len + sizeof(struct icmphdr));
            int org_ip_hdr_len = org_ip_hdr->ihl * 4;

            struct icmphdr *org_icmp_hdr =
                (struct icmphdr*)(buff + ip_header_len +
                                  sizeof(struct icmphdr) + org_ip_hdr_len);

            received_id  = ntohs(org_icmp_hdr->un.echo.id);
            received_seq = ntohs(org_icmp_hdr->un.echo.sequence);
        } else if (icmp_hdr->type == ICMP_ECHOREPLY) {
            if (icmp_hdr->code != 0)
                continue;
            received_id  = ntohs(icmp_hdr->un.echo.id);
            received_seq = ntohs(icmp_hdr->un.echo.sequence);
        } else {
            continue; // ignora alte tipuri
        }

        //verifica daca pachetul e al nostru
        if (received_id == (pid & PID_MASK) && received_seq == seq) {
            const char *ip_str = inet_ntoa(from_addr.sin_addr);
            strncpy(router_ip, ip_str, INET_ADDRSTRLEN - 1);
            router_ip[INET_ADDRSTRLEN - 1] = '\0';
            //printf("D: [server][trace]: MATCH! Reply de la %s, type=%d\n",
            //       router_ip, icmp_hdr->type);
            return icmp_hdr->type;
        } else {
            //printf("D: [server][trace]: Mismatch - expected seq=%d, got seq=%d\n",
            //       seq, received_seq);
            continue;
        }
    }
    
    return -1;
}

//functia principala de traceroute
//trimite probe ICMP cu TTL crescator si asteapta raspunsuri
//pana la atingerea destinatiei sau max_ttl
//returneaza 0 la succes, -1 la eroare
//fd este socketul client-ului pentru a trimite rezultatele
extern int send_Message(int fd, char* message);

int traceroute(int fd, const char *dest_ip, int max_ttl, int timeout_ms,
               int interval_ms, int probes_per_ttl){
    int sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); //creez socket raw pentru ICMP
    if(sd < 0){
        perror("[server][trace]: Error: socket()\n");
        return -1;
    }
    
    char msg[512];
    snprintf(msg, sizeof(msg), "Starting traceroute to %s\n", dest_ip);
    send_Message(fd, msg);

    int pid = getpid() & PID_MASK;
    int seq = 0;
    int reached = 0;

    for(int ttl = 1; ttl <= max_ttl && !reached; ttl++){
        snprintf(msg, sizeof(msg), "%2d  ", ttl);
        send_Message(fd, msg);

        for(int p = 0; p < probes_per_ttl; p++){
            char router_ip[INET_ADDRSTRLEN];
            strcpy(router_ip, "*");
            seq++;

            if(send_Probe(sd, dest_ip, ttl, seq) < 0){
                send_Message(fd, "error ");
                continue;
            }

            int rc = recive_Reply(sd, pid, seq, router_ip, timeout_ms);
            if(rc == -2){
                 send_Message(fd, "* ");
             } else if(rc < 0){
                 send_Message(fd, "err ");
             } else {
                 memset(msg, 0, sizeof(msg));
                 snprintf(msg, sizeof(msg), "%s ", router_ip);
                 send_Message(fd, msg);
                 if(rc == ICMP_ECHOREPLY)
                     reached = 1;
             }

            usleep(interval_ms * 1000);
        }
        send_Message(fd, "\n");
    }

    close(sd);
    send_Message(fd, "Traceroute completed.\n");
    return 0;
}
