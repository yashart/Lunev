#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <math.h>
#include <signal.h>
#include <linux/errqueue.h>

struct Range
{
    double start;
    double end;
    double step;
};

struct Answer
{
    double start;
    double result;
};


int client_init(const char* ip_addr, const char* port)
{
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }

    int keepalive = 1;
    int keepcnt = 5;
    int keepidle = 10;
    int keepintvl = 7;
    int reuseadrr = 1;
    timeval timeout;
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,\
            &keepalive, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT,\
            &keepcnt, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE,\
            &keepidle, sizeof(int));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL,\
            &keepintvl, sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,\
            &reuseadrr, sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,\
            (char *)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,\
            (char *)&timeout, sizeof(timeout));


    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port)); // или любой другой порт...
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(2);
    }

    return sock;
}

double function_value(double argument)
{
    return sin(argument)/argument;
}

Answer calc_integral(Range data)
{
    double result = 0;
    for(double i = data.start; i < data.end; i += data.step)
    {
        result += (function_value(i) + function_value(i + data.step))\
                     / 2 * data.step;
    }
    Answer answer;
    answer.result   = result;
    answer.start    = data.start;
    return answer;
}

sockaddr_in broadcast_get_ip(int port)
{
    int sock;
    sockaddr_in addr;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }
    char buf[1];
    socklen_t sendsize = sizeof(addr);
    recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&addr,\
             &sendsize);
    return addr;
}


int main(int argc, char** argv)
{
    if(argc != 2)
    {
        perror("use port");
        return 0;
    }
    sockaddr_in broadcastAddr = broadcast_get_ip(atoi(argv[1]));
    char* ip_addr = inet_ntoa(broadcastAddr.sin_addr);
    printf("%s\n", ip_addr);
    int sock = client_init(ip_addr, argv[1]);
    
    Answer answer;
    Range data;
    int bytes_read;
    while(1)
    {
        if(recv(sock, &data, sizeof(data), MSG_ERRQUEUE) <= 0)
        {
            break;
        }
        printf("data start: %lf\n", data.start);
        answer = calc_integral(data);
        send(sock, &answer, sizeof(answer), 0);
    }
    close(sock);

    return 0;
}