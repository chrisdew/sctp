#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#define PORT 11000
#define MSG_SIZE 1000
#define NUMBER_OF_MESSAGES 1000

int sock;
struct sockaddr_in *paddrs[5];
struct sockaddr_in *laddrs[5];

void handle_signal(int signum);

int main(int argc, char **argv)
{
    int i;
    int counter = 0;
    int asconf = 1;
    int ret;
    int addr_count;
    char address[16];
    char buffer[MSG_SIZE];
    sctp_assoc_t id;
    struct sockaddr_in addr;
    struct sctp_status status;
    struct sctp_initmsg initmsg;
    struct sctp_event_subscribe events;
    struct sigaction sig_handler;
    struct sctp_paddrparams heartbeat;
    struct sctp_rtoinfo rtoinfo;

    memset(&buffer,     'j', MSG_SIZE);
    memset(&initmsg,    0,   sizeof(struct sctp_initmsg));
    memset(&addr,       0,   sizeof(struct sockaddr_in));
    memset(&events,     1,   sizeof(struct sctp_event_subscribe));
    memset(&status,     0,   sizeof(struct sctp_status));
    memset(&heartbeat,  0,   sizeof(struct sctp_paddrparams));
    memset(&rtoinfo,    0, sizeof(struct sctp_rtoinfo))

    if(argc != 2 || (inet_addr(argv[1]) == -1))
    {
        puts("Usage: client [IP ADDRESS in form xxx.xxx.xxx.xxx] ");        
        return 0;
    }

    strncpy(address, argv[1], 15);
    address[15] = 0;

    addr.sin_family = AF_INET;
    inet_aton(address, &(addr.sin_addr));
    addr.sin_port = htons(PORT);

    initmsg.sinit_num_ostreams = 2;
    initmsg.sinit_max_instreams = 2;
    initmsg.sinit_max_attempts = 1;

    heartbeat.spp_flags = SPP_HB_ENABLE;
    heartbeat.spp_hbinterval = 5000;
    heartbeat.spp_pathmaxrxt = 1;

    rtoinfo.srto_max = 2000;

    sig_handler.sa_handler = handle_signal;
    sig_handler.sa_flags = 0;

    /*Handle SIGINT in handle_signal Function*/
    if(sigaction(SIGINT, &sig_handler, NULL) == -1)
        perror("sigaction");

    /*Create the Socket*/
    if((ret = (sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP))) < 0)
        perror("socket");

    /*Configure Heartbeats*/
    if((ret = setsockopt(sock, SOL_SCTP, SCTP_PEER_ADDR_PARAMS , &heartbeat, sizeof(heartbeat))) != 0)
        perror("setsockopt");

    /*Set rto_max*/
    if((ret = setsockopt(sock, SOL_SCTP, SCTP_RTOINFO , &rtoinfo, sizeof(rtoinfo))) != 0)
        perror("setsockopt");

    /*Set SCTP Init Message*/
    if((ret = setsockopt(sock, SOL_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg))) != 0)
        perror("setsockopt");

    /*Enable SCTP Events*/
    if((ret = setsockopt(sock, SOL_SCTP, SCTP_EVENTS, (void *)&events, sizeof(events))) != 0)
        perror("setsockopt");

    /*Get And Print Heartbeat Interval*/
    i = (sizeof heartbeat);
    getsockopt(sock, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &heartbeat, (socklen_t*)&i);

    printf("Heartbeat interval %d\n", heartbeat.spp_hbinterval);

    /*Connect to Host*/
    if(((ret = connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr)))) < 0)
    {
        perror("connect");
        close(sock);
        exit(0);
    }

    /*Get Peer Addresses*/
    addr_count = sctp_getpaddrs(sock, 0, (struct sockaddr**)paddrs);
    printf("\nPeer addresses: %d\n", addr_count);

    /*Print Out Addresses*/
    for(i = 0; i < addr_count; i++)
        printf("Address %d: %s:%d\n", i +1, inet_ntoa((*paddrs)[i].sin_addr), (*paddrs)[i].sin_port);

    sctp_freepaddrs((struct sockaddr*)*paddrs);

    /*Start to Send Data*/
    for(i = 0; i < NUMBER_OF_MESSAGES; i++)
    {
        counter++;
        printf("Sending data chunk #%d...", counter);

        if((ret = send(sock, buffer, MSG_SIZE, 0)) == -1)
            perror("write");

        printf("Sent %d bytes to peer\n",ret);

        sleep(1);
    }

    if(close(sock) != 0)
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
