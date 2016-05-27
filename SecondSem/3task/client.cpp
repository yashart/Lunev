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
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

const int PORT = 3080;

double* GLOBAL_RESULT = NULL;
int semId = 0;
int shmResultId = 0;

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

Range Range_ctor(double start, double end, double step)
{
    Range range;
    range.start = start;
    range.end   = end;
    range.step  = step;
    return range;
}

int init_shm()
{
    shmResultId = shmget(IPC_PRIVATE, sizeof(double),\
                     0666 | IPC_CREAT);
    GLOBAL_RESULT = (double*) shmat(shmResultId, NULL, 0);
    *GLOBAL_RESULT = 0;
    return 0;
};

int init_sem()
{
    semId = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    return 0;
};

int free_sem()
{
    semctl(semId, IPC_RMID, 0);
    return 0;
};

int free_shm()
{
    shmdt(GLOBAL_RESULT);
    shmctl(shmResultId, IPC_RMID, NULL);
    return 0;
};


int client_init(const char* ip_addr)
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
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(2);
    }

    return sock;
}

sembuf sem_set(int sem_num, int sem_op, int sem_flg)
{
    sembuf semops;
    semops.sem_num = sem_num;
    semops.sem_op = sem_op;
    semops.sem_flg = sem_flg;

    return semops;
}


double function_value(double argument)
{
    return sin(argument)/argument;
}

double calc_part_integral(Range data)
{
    double result = 0;
    for(double i = data.start; i < data.end; i += data.step)
    {
        result += (function_value(i) + function_value(i + data.step))\
                     / 2 * data.step;
    }
    return result;
}

Answer calc_integral(Range data, int numberProcess)
{
    double forkStep = (data.end - data.start)\
                        /numberProcess;
    Range dataStep;
    for(int i = 0; i < numberProcess; i++)
    {
        int pid = fork();
        if(pid == 0)
        {
            double localResult = 0;
            dataStep = Range_ctor(\
                data.start + i * forkStep,\
                data.start + (i + 1) * forkStep,\
                data.step);
            localResult = calc_part_integral(dataStep);
            sembuf semops[2];
            semops[0] = sem_set(0, 0, 0);
            semops[1] = sem_set(0, 1, 0);
            if(semop(semId, semops, 2))
            {
                perror("semop start");
                exit(0);
            }
            *GLOBAL_RESULT += localResult;
            semops[0] = sem_set(0, -1, 0);
            if(semop(semId, semops, 1))
            {
                perror("semop end");
                exit(0);
            }
            exit(0);
        }
    }
    for(int i = 0; i < numberProcess; i++)
    {
        int status = 0;
        wait(&status);
    }
    
    Answer answer;
    answer.result   = *GLOBAL_RESULT;
    *GLOBAL_RESULT = 0;
    answer.start    = data.start;
    return answer;
}

sockaddr_in broadcast_get_ip()
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
    addr.sin_port = htons(PORT);
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
    close(sock);
    return addr;
}


int main(int argc, char** argv)
{
    if(argc != 2)
    {
        perror("use numberProcess");
        return 0;
    }
    int numberProcess = atoi(argv[1]);
    sockaddr_in broadcastAddr = broadcast_get_ip();
    char* ip_addr = inet_ntoa(broadcastAddr.sin_addr);
    printf("%s %d\n", ip_addr, broadcastAddr.sin_port);
    sleep(3);

    int sock = client_init(ip_addr);
    init_sem();
    init_shm();
    
    Answer answer;
    Range data;
    while(1)
    {
        if(recv(sock, &data, sizeof(data), 0) <= 0)
        {
            break;
        }
        printf("data start: %lf\n", data.start);
        answer = calc_integral(data, numberProcess);
        send(sock, &answer, sizeof(answer), 0);
    }
    free_shm();
    free_sem();
    close(sock);

    return 0;
}