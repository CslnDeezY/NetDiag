#include "traceroute.h"
#include "command.h"
#include <pthread.h>
#include <stdio.h>
#include <sys/select.h>


//populeaza structura de configurare cu valori implicite
struct trace_config default_trace_config(int client){
    struct trace_config cfg;
    memset(&cfg, 0, sizeof(struct trace_config));

    strncpy(cfg.dest_ip, "8.8.8.8", sizeof(cfg.dest_ip)); //localhost
    cfg.dest_ip[sizeof(cfg.dest_ip)-1] = '\0';
    cfg.max_ttl = 30;
    cfg.timeout_ms = 500;      //0.5 secunde
    cfg.interval_ms = 1000;     //1 secunda
    cfg.probes_per_ttl = 1;
    cfg.cycle = 1;

    cfg.client_fd = client;
    if(pthread_mutex_init(&cfg.mutex, NULL) != 0){
        perror("[server] Default trace config: Error initializing mutex\n");
    }
    if(pthread_cond_init(&cfg.cond, NULL) != 0){
        perror("[server] Default trace config: Error initializing condition variable\n");
    }

    cfg.thread_exists = false;
    cfg.is_running = false;
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
int send_Probe(int sd, const char *dest_ip, int ttl, int seq, int client_id){
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
    //folosim client_id (fd-ul socketului) in loc de getpid()
    icmp->un.echo.id = htons(client_id & 0xFFFF); //identificator unic pentru proces
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
int recive_Reply(int sd, int client_id, int seq, char *router_ip, int timeout_ms){
    fd_set read_fds;
    struct timeval tm_out;
    struct timeval start_time, current_time;//calculam timpul de asteptare ca sa nu ne blocam undeva
    gettimeofday(&start_time, NULL);
    
    while(true){
        gettimeofday(&current_time, NULL);
        long elapsed = (current_time.tv_sec - start_time.tv_sec) * 1000 + (current_time.tv_usec - start_time.tv_usec) / 1000;
        if (elapsed >= timeout_ms) 
            return -2; // Timeout real

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
        int bytes = recvfrom(sd, buff, sizeof(buff), 0, (struct sockaddr*)&from_addr, &from_sk_len);
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

        //printf("Debug: [server][trace]: Received ICMP type=%d, code=%d from %s\n", icmp_hdr->type, icmp_hdr->code, inet_ntoa(from_addr.sin_addr));

        //procesare:
        if (icmp_hdr->type == ICMP_TIME_EXCEEDED || icmp_hdr->type == ICMP_DEST_UNREACH) {
            if (bytes < ip_header_len + (int)sizeof(struct icmphdr) + (int)sizeof(struct iphdr) + 8) 
                continue;

            struct iphdr *org_ip_hdr = (struct iphdr*)(buff + ip_header_len + sizeof(struct icmphdr));
            int org_ip_hdr_len = org_ip_hdr->ihl * 4;

            struct icmphdr *org_icmp_hdr = (struct icmphdr*)(buff + ip_header_len + sizeof(struct icmphdr) + org_ip_hdr_len);

            received_id  = ntohs(org_icmp_hdr->un.echo.id);
            received_seq = ntohs(org_icmp_hdr->un.echo.sequence);
        } else if (icmp_hdr->type == ICMP_ECHOREPLY) {
            if (icmp_hdr->code != 0)
                continue;
            received_id  = ntohs(icmp_hdr->un.echo.id);
            received_seq = ntohs(icmp_hdr->un.echo.sequence);
        } else {
            //pachetul este pentru alt client
            // logam faptul ca am "interceptat" pachetul altcuiva
            log_packet_mismatch(client_id, received_id, received_seq, inet_ntoa(from_addr.sin_addr));
            continue; // ignora alte tipuri
        }

        //verifica daca pachetul e al nostru
        if (received_id == (client_id & 0xFFFF) && received_seq == seq) {
            const char *ip_str = inet_ntoa(from_addr.sin_addr);
            strncpy(router_ip, ip_str, INET_ADDRSTRLEN - 1);
            router_ip[INET_ADDRSTRLEN - 1] = '\0';
            //printf("Debug: [server][trace]: MATCH! Reply de la %s, type=%d\n", router_ip, icmp_hdr->type);
            return icmp_hdr->type;
        } else {
            //printf("Debug: [server][trace]: Mismatch - expected seq=%d, got seq=%d\n", seq, received_seq);
            continue;
        }
    }
    
    return -1;
}

void* traceroute_thread(void *arg){
    //fac cast la structura de configurare
    struct trace_config* client_config = (struct trace_config*) arg;

    while(true){//principal while
        pthread_mutex_lock(&client_config->mutex);
        //starea de idle pana primim un semnal de a porni munca

        while(!client_config->is_running && !client_config->want_exit){
            pthread_cond_wait(&client_config->cond, &client_config->mutex);
        }

        //verificam daca a venit timpul for death
        if(client_config->want_exit){
            pthread_mutex_unlock(&client_config->mutex);
            printf("[server][trace_thread]: Exiting traceroute thread\n");
            break;
        }

        pthread_mutex_unlock(&client_config->mutex);

        //executarea propriuzisa a trace-ului (aici e cel mai mult)
        trace_run(client_config);

        //dupa ce se termina trace-ul, resetam flagul is_running
        pthread_mutex_lock(&client_config->mutex);
        client_config->is_running = false;
        client_config->want_stop = false;
        //client_config->want_reset = false;
        pthread_mutex_unlock(&client_config->mutex);

        send_Message(client_config->client_fd, "Thread work done\n");
        printf("[server][trace_thread]: Thread work done, IDLE....\n");


    }    //end principal while
    return NULL;
}

int trace_run(struct trace_config* client_config){
    
    if(client_config->want_report){
        //generam raportul
        traceroute_raport(client_config);
    }
    else {
        //rulam traceroute continuu
        traceroute(client_config);
        send_Message(client_config->client_fd, "\033[?25h\033[?1049l"); //afisam cursorul si iesim din modul ecran alternativ
    }
    return  0;
}

int traceroute_raport(struct trace_config* client_config){
    send_Message(client_config->client_fd, "GGenerating traceroute report...wait a moment...\n");
    printf("[server][trace_raport] Generating traceroute report for client_fd %d\n", client_config->client_fd);

    //creez socket raw pentru ICMP
    int sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sd < 0){
        perror("[server][trace]: Error: socket()\n");
        return -1;
    }
    struct timeval start, end;

    //cream un tablou cu datele de la  toate routerele
    struct trace_hop_data hops_data[client_config->max_ttl];
    memset(hops_data, 0, sizeof(hops_data));

    int seq = 0;;

    for(int c = 1; c <= client_config->cycle; c++){
        bool reached = false;

        for(int ttl = 1; ttl <= client_config->max_ttl && !reached; ttl++){
            //verificam daca vrem sa oprim
            if(client_config->want_stop){
                send_Message(client_config->client_fd, "Report stoped!\n");
                return 0;
            }
            //verificam daca vrem sa resetam
            if(client_config->want_reset){
                c = 1;
                ttl = 1;
                reached = false;
                memset(hops_data, 0, sizeof(hops_data));
                //este aceasta o sectiune critica
                pthread_mutex_lock(&client_config->mutex);
                client_config->want_reset = false;
                pthread_mutex_unlock(&client_config->mutex);

                send_Message(client_config->client_fd, "Report reseted!\n");
            }
            struct trace_hop_data *hop = &hops_data[ttl - 1];   //datele unuii hop

            for(int probe = 0; probe < client_config->probes_per_ttl; probe++){
                long rtt_ms = -1;   //round-trip time in milisecunde
                char router_ip[64];
                strncpy(router_ip, "*", sizeof(router_ip));
                router_ip[sizeof(router_ip) - 1] = '\0';
                seq++;
                hop->send++;

                if(send_Probe(sd, client_config->dest_ip, ttl, seq,client_config->client_fd) < 0){
                    //send_Message(client_config->client_fd, "[traceroute] Error: send probe\n");
                    printf("[server][trace] Error send_Probe\n");
                    continue;
                }

                gettimeofday(&start, NULL); //timpul la care am trimis pchetul

                int reply = recive_Reply(sd, client_config->client_fd, seq, router_ip, client_config->timeout_ms);

                gettimeofday(&end, NULL); //timpul la care am primit raspunsul

                if(reply == -2){
                    //timeout
                    printf("[server][trace]: Timeout waiting for reply\n");
                }else if(reply < 0){
                    //eroare
                    printf("[server][trace] Error receiving reply\n");
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

                usleep(client_config->interval_ms * 1000); //interval intre probe 10 ms
            }//probe loop
        }//ttl loop
    }//cycle loop

    //inchid socketul raw
    close(sd);
    //trimitem raportul clientului
    if(send_raport(client_config->client_fd, hops_data, client_config->max_ttl) < 0) {
        printf("[server][trace_raport]: Error sending report to client\n");
    }
    return 0;
}

int send_raport(int fd, struct trace_hop_data* hops_data, int max_ttl){
    char buffer[message_len];
    memset(buffer, 0, sizeof(buffer));

    //initializam antetul tabelului
    strcat(buffer, "==================== FINAL TRACEROUTE REPORT ===================\n");
    strcat(buffer, " nr. |      Host IP      | Last(ms) | Sent | Avg(ms) |  Status  \n");
    strcat(buffer, "-----|-------------------|----------|------|---------|----------\n");
    send_Message(fd, buffer);
    memset(buffer, 0, sizeof(buffer));
    for(int i = 0; i < max_ttl; i++){
        struct trace_hop_data hop = hops_data[i];

        if(hop.send == 0){
            //nu am trimis probe la acest hop inca
            continue;
        }

        float avg_rtt = 0;   //daca nu am primit niciun raspuns este 0
        if(hop.received > 0){
            avg_rtt = hop.sum_rtt / hop.received;
        }

        //determinam statusul si ii atribuim cate o culoare:
        char *statut_str;
        char *statut_color;
        if(hop.received == 0){
            statut_str = "Lost";
            statut_color = CLR_ERR; //rosu
        }else if(hop.received < hop.send){
            statut_str = "Unstable";
            statut_color = CLR_UNSTBL; //galben
        }else {
            statut_str = "Good";
            statut_color = CLR_STBL; //verde
        }
        snprintf(buffer, sizeof(buffer), " %-3d | %-17s | %8d | %4d | %7.2f | %s%-8s %s\n", 
                    i + 1, hop.router_ip, hop.last_rtt, hop.send, avg_rtt, statut_color,statut_str, CLR_RESET);
        send_Message(fd, buffer);
        memset(buffer, 0, sizeof(buffer));
        
    }
    strcat(buffer, "\033[J"); //sterg tot ce e sub tabel ca sa fiu sigur
    return send_Message(fd, buffer);
}


//functia principala de traceroute
//trimite probe ICMP cu TTL crescator si asteapta raspunsuri
//pana la atingerea destinatiei sau max_ttl
//returneaza 0 la succes, -1 la eroare
//fd este socketul client-ului pentru a trimite rezultatele
int traceroute(struct trace_config* client_config){
    int sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); //creez socket raw pentru ICMP
    if(sd < 0){
        perror("[server][trace]: Error: socket()\n");
        return -1;
    }
    struct timeval start, end;

    //creez structura de date pentru fiecare hop
    struct trace_hop_data hops_data[client_config->max_ttl]; //max hop-uri
    memset(hops_data, 0, sizeof(hops_data));
    

    //creez un ecran alternativ pentru a afisa tabelul, sterg tot, setez cursorul la inceput, ascund cursorul
    send_Message(client_config->client_fd, "\033[?1049h\033[2J\033[H\033[?25l"); 

    int seq = 0;
    while(true){
        bool reached = false;
        char buffer[message_len];
        memset(buffer, 0, sizeof(buffer));

        //initializam antetul tabelului
        strcat(buffer, " nr. |      Host IP      | Last(ms) | Sent | Avg(ms) |  Status  \n");
        strcat(buffer, "-----|-------------------|----------|------|---------|----------\n");
        send_Message(client_config->client_fd, buffer);
        memset(buffer, 0, sizeof(buffer));

        //pornim trimiterea mesajelor catre routere
        for(int ttl = 1; ttl <= client_config->max_ttl && !reached; ttl++){
            struct trace_hop_data *hop = &hops_data[ttl - 1];

            //verificam daca vrem sa oprim
            if(client_config->want_stop){
                send_Message(client_config->client_fd, "Trace stoped!\n");
                return 0;
            }
            //verificam daca vrem sa resetam
            if(client_config->want_reset){
                //sterg tot, setez cursorul la inceput, ascund cursorul
                send_Message(client_config->client_fd, "\033[2J\033[H");
                send_Message(client_config->client_fd, "Trace reseted!\n");

                //reafisam antetul
                strcat(buffer, " nr. |      Host IP      | Last(ms) | Sent | Avg(ms) |  Status  \n");
                strcat(buffer, "-----|-------------------|----------|------|---------|----------\n");
                send_Message(client_config->client_fd, buffer);
                memset(buffer, 0, sizeof(buffer));

                ttl = 1;    //pornim iarasi de la primul router
                reached = false;
                memset(hops_data, 0, sizeof(hops_data));
                //este aceasta o sectiune critica
                pthread_mutex_lock(&client_config->mutex);
                client_config->want_reset = false;
                pthread_mutex_unlock(&client_config->mutex);
            }

            for(int probe = 1; probe <= client_config->probes_per_ttl; probe++){
                long rtt_ms = -1;   //round-trip time in milisecunde
                char router_ip[64];
                strncpy(router_ip, "*", sizeof(router_ip));
                router_ip[sizeof(router_ip) - 1] = '\0';
                seq++;
                hop->send++;

                if(send_Probe(sd, client_config->dest_ip, ttl, seq, client_config->client_fd) < 0){
                    //send_Message(client_config->client_fd, "[traceroute] Error: send probe\n");
                    printf("[server][trace] Error send_Probe\n");
                    continue;
                }

                gettimeofday(&start, NULL); //timpul la care am trimis pchetul

                int reply = recive_Reply(sd, client_config->client_fd, seq, router_ip, client_config->timeout_ms);

                gettimeofday(&end, NULL); //timpul la care am primit raspunsul

                if(reply == -2){
                    //timeout
                    printf("[server][trace]: Timeout waiting for reply\n");
                }else if(reply < 0){
                    //eroare
                    printf("[server][trace] Error receiving reply\n");
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

                usleep(client_config->interval_ms * 1000); //interval intre probe 10 ms

                //acutalizam datele pe ecran pentru fiecare pachet trimis

                if(hop->send == 0){
                    //nu am trimis probe la acest hop inca
                    continue;
                }

                float avg_rtt = 0;   //daca nu am primit niciun raspuns este 0
                if(hop->received > 0){
                    avg_rtt = hop->sum_rtt / hop->received;
                }

                //determinam statutul si ii atribuim cate o culoare:
                char *statut_str;
                char *statut_color;
                if(hop->received == 0){
                    statut_str = "Lost";
                    statut_color = CLR_ERR; //rosu

                }else if(hop->received < hop->send){
                    statut_str = "Unstable";
                    statut_color = CLR_UNSTBL; //galben
                }else {
                    statut_str = "Good";
                    statut_color = CLR_STBL; //verde
                }
                snprintf(buffer, sizeof(buffer), "\r %-3d | %-17s | %8d | %4d | %7.2f | %s%-8s %s \033[K", 
                            ttl, hop->router_ip, hop->last_rtt, hop->send, avg_rtt, statut_color,statut_str, CLR_RESET);
                send_Message(client_config->client_fd, buffer);
                memset(buffer, 0, sizeof(buffer));
            }//probe loop
            send_Message(client_config->client_fd, "\n");
        }//end ttl loop

        //intoarcem cursorul sus
        send_Message(client_config->client_fd, "\033[H");
    }//end while

    close(sd);
    //send_Message(fd, "Traceroute completed.\n");
    return 0;
}

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER; //pentru sincronizarea scrierii in fisierul de log
void log_packet_mismatch(int expected_id, int received_id, int seq, char* from_ip){
    pthread_mutex_lock(&log_mutex);
    
    FILE *f = fopen("traceroute_debug.log", "a");
    if (f) {
        time_t now = time(NULL);
        char *timestamp = ctime(&now);
        timestamp[strlen(timestamp) - 1] = '\0'; // Scoatem \n-ul de la final

        fprintf(f, "[%s] THREAD_FD(%d): Ignored packet from %s (Expected ID: %d, Got ID: %d, Seq: %d)\n",
                timestamp, expected_id, from_ip, expected_id, received_id, seq);
        fclose(f);
    }
    
    pthread_mutex_unlock(&log_mutex);
}