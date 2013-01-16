#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE (1 << 16)
#define PORT 10000   

int sock, ret, flags;
int i, reuse = 1;
int addr_count = 0;
char buffer[BUFFER_SIZE];
socklen_t from_len;

struct sockaddr_in addr;
struct sockaddr_in *laddr[10];
struct sockaddr_in *paddrs[10];
struct sctp_sndrcvinfo sinfo;
struct sctp_event_subscribe event;  
struct sctp_prim prim_addr; 
struct sctp_paddrparams heartbeat;
struct sigaction sig_handler;
struct sctp_rtoinfo rtoinfo;

void handle_signal(int signum);

int main(void)
{
    if((sock = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP)) < 0)
        perror("socket");

    memset(&addr,       0, sizeof(struct sockaddr_in));
    memset(&event,      1, sizeof(struct sctp_event_subscribe));
    memset(&heartbeat,  0, sizeof(struct sctp_paddrparams));
    memset(&rtoinfo,    0, sizeof(struct sctp_rtoinfo));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    from_len = (socklen_t)sizeof(struct sockaddr_in);

    sig_handler.sa_handler = handle_signal;
    sig_handler.sa_flags = 0;

    heartbeat.spp_flags = SPP_HB_ENABLE;
    heartbeat.spp_hbinterval = 5000;
    heartbeat.spp_pathmaxrxt = 1;

    rtoinfo.srto_max = 2000;

    /*Set Heartbeats*/
    if(setsockopt(sock, SOL_SCTP, SCTP_PEER_ADDR_PARAMS , &heartbeat, sizeof(heartbeat)) != 0)
        perror("setsockopt");

    /*Set rto_max*/
    if(setsockopt(sock, SOL_SCTP, SCTP_RTOINFO , &rtoinfo, sizeof(rtoinfo)) != 0)
        perror("setsockopt");

    /*Set Signal Handler*/
    if(sigaction(SIGINT, &sig_handler, NULL) == -1)
        perror("sigaction");

    /*Set Events */
    if(setsockopt(sock, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(struct sctp_event_subscribe)) < 0)
        perror("setsockopt");

    /*Set the Reuse of Address*/
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))< 0)
        perror("setsockopt");

    /*Bind the Addresses*/
    if(bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0)
        perror("bind");

    if(listen(sock, 2) < 0)
        perror("listen");

    /*Get Heartbeat Value*/
    i = (sizeof heartbeat);
    getsockopt(sock, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &heartbeat, (socklen_t*)&i);
    printf("Heartbeat interval %d\n", heartbeat.spp_hbinterval);

    /*Print Locally Binded Addresses*/
    addr_count = sctp_getladdrs(sock, 0, (struct sockaddr**)laddr);
    printf("Addresses binded: %d\n", addr_count);
    for(i = 0; i < addr_count; i++)
         printf("Address %d: %s:%d\n", i +1, inet_ntoa((*laddr)[i].sin_addr), (*laddr)[i].sin_port);
    sctp_freeladdrs((struct sockaddr*)*laddr);

    while(1)
    {
        flags = 0;

        ret = sctp_recvmsg(sock, buffer, BUFFER_SIZE, NULL, 0, NULL, &flags);

        if(flags & MSG_NOTIFICATION)
        printf("Notification received from %s:%u\n",  inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        printf("%d bytes received from %s:%u\n", ret, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));      
    }

    if(close(sock) < 0)
        perror("close");
}   

void handle_signal(int signum)
{
    switch(signum)
    {
        case SIGINT:
            if(close(sock) != 0)
                perror("close");
            exit(0);
            break;  
        default: exit(0);
            break;
    }
}
