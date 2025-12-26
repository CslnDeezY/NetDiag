#include "traceroute.h"
#include "command.h"
#include <stdio.h>


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

        //printf("Debug: [server][trace]: Received ICMP type=%d, code=%d from %s\n", 
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
            //printf("Debug: [server][trace]: MATCH! Reply de la %s, type=%d\n",
            //       router_ip, icmp_hdr->type);
            return icmp_hdr->type;
        } else {
            //printf("Debug: [server][trace]: Mismatch - expected seq=%d, got seq=%d\n",
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

int traceroute(int fd, const char *dest_ip, int max_ttl, int timeout_ms, int interval_ms, int probes_per_ttl){
    int sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); //creez socket raw pentru ICMP
    if(sd < 0){
        perror("[server][trace]: Error: socket()\n");
        return -1;
    }

    struct timeval start, end;

    //creez structura de date pentru fiecare hop
    struct trace_hop_data hops_data[max_ttl]; //max hop-uri
    memset(hops_data, 0, sizeof(hops_data));
    
    //char msg[512];
    //snprintf(msg, sizeof(msg), "Starting traceroute to %s\n", dest_ip);
    //send_Message(fd, msg);

    send_Message(fd, "\033[2J\033[H");

    int pid = getpid() & PID_MASK;
    int seq = 0;
    bool reached = false;

    for(int ttl = 1; ttl <= max_ttl && !reached; ttl++){
        
        struct trace_hop_data *hop = &hops_data[ttl - 1];

        for(int probe = 0; probe < probes_per_ttl; probe++){

            long rtt_ms = -1;   //round-trip time in milisecunde

            char router_ip[16];
            strcpy(router_ip, "*");
            seq++;

            hop->send++;

            if(send_Probe(sd, dest_ip, ttl, seq) < 0){
                send_Message(fd, "[traceroute] Error: send probe\n");
                continue;
            }

            gettimeofday(&start, NULL);

            int reply = recive_Reply(sd, pid, seq, router_ip, timeout_ms);

            gettimeofday(&end, NULL);

            if(reply == -2){
                //timeout
                perror("[server][trace]: Timeout waiting for reply\n");
             }else if(reply < 0){
                //eroare
                perror("[server][trace] Error receiving reply\n");
             }else{
                //am primit raspuns
                //memset(msg, 0, sizeof(msg));
                hop->received++;
                
                //salvez ip-ul router-ului
                if(strcmp(router_ip, "*") != 0){
                    strcpy(hop->router_ip, router_ip);
                }

                //calculez RTT
                rtt_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec / 1000 - start.tv_usec / 1000);

                //salvez statistica
                hop->last_rtt = rtt_ms;
                hop->sum_rtt += rtt_ms;

                if(reply == ICMP_ECHOREPLY){
                    reached = true; //am ajuns la destinatie
                    hop->reached = true;
                }
            }
            //afisez tabelul de trace actualizat
            if( print_trace_table(fd, hops_data, max_ttl) < 0 ){
                printf("[server][trace]: Error client deconectat\n");
                close(sd);
                return -1;
            }

            usleep(interval_ms * 1000); //interval intre probes
        }
    }

    close(sd);
    //send_Message(fd, "Traceroute completed.\n");
    return 0;
}

int print_trace_table(int fd, struct trace_hop_data* hops_data, int max_ttl){
    //cream un buffer mare pentru intreg tabelul si il triitem la fiecare modificare ca sa arate totul frumos
    char big_buffer[16384]; //16KB pentru tabel
    memset(big_buffer, 0, sizeof(big_buffer));

    //ne intoarcem in stanga sus cu cursorul
    strcat(big_buffer, "\033[H");
    
    char mini_buffer[message_len];
    memset(mini_buffer, 0, sizeof(mini_buffer));
    //initializam antetul tabelului
    strcat(big_buffer, " nr. |      Host IP      | Last(ms) | Sent | Avg(ms) |  Status  \n");
    strcat(big_buffer, "-----|-------------------|----------|------|---------|----------\n");

    
    for(int i = 0; i < max_ttl; i++){
        struct trace_hop_data hop = hops_data[i];

        if(hop.send == 0){
            //nu am trimis probe la acest hop inca
            continue;
        }

        long avg_rtt = 0;   //daca nu am primit niciun raspuns este 0
        if(hop.reached > 0){
            avg_rtt = hop.sum_rtt / hop.received;
        }

        //determinam statusul si ii atribuim cate o culoare:
        char *status_str;
        char *status_color;
        if(hop.received == 0){
            status_str = "Lost";
            status_color = CLR_ERR; //rosu

        }else if(hop.received < hop.send){
            status_str = "Unstable";
            status_color = CLR_UNSTBL; //galben
        }else {
            status_str = "Good";
            status_color = CLR_STBL; //verde
        }



        snprintf(mini_buffer, sizeof(mini_buffer), "%s %3d | %-17s | %8ld | %4d | %7ld | %-8s %s\n", 
                    status_color, i + 1, hop.router_ip, hop.last_rtt, hop.send, avg_rtt, status_str, CLR_RESET);
        strcat(big_buffer, mini_buffer);
        memset(mini_buffer, 0, sizeof(mini_buffer));
        
    }
    //strcat(big_buffer, "\033[J"); //ster tot ce e sub tabel ca sa fiu sigur
    return send_Message(fd, big_buffer);
}
