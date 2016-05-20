#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

const int INTEGRAL_PARTS_NUMBER = 30;
const double STEP = 1e-8;

struct Range
{
    double start;
    double end;
    double step;
};

struct AllocationQuery
{
    Range distancesQuery[INTEGRAL_PARTS_NUMBER];
    int iterator; 
};

struct Answer
{
    double start;
    double result;
};

AllocationQuery* SHM_BUFFER = NULL;
double* GLOBAL_RESULT = NULL;
int semId = 0;
int shmId = 0;
int shmResultId = 0;
double localResult = 0;


Range Range_ctor(double start, double end, double step)
{
    Range range;
    range.start = start;
    range.end   = end;
    range.step  = step;
    return range;
}

AllocationQuery AllocationQuery_ctor(double start,\
                     double end, double step)
{
    AllocationQuery query;
    query.iterator = 0;
    double part = (end - start)/INTEGRAL_PARTS_NUMBER;
    for(int i = 0; i < INTEGRAL_PARTS_NUMBER; i++)
    {
        query.distancesQuery[i] = \
            Range_ctor(start + i * part,\
                       start + (i + 1) * part,\
                       step);
    }
    return query;
}

int init_shm()
{
    shmId = shmget(IPC_PRIVATE, sizeof(AllocationQuery),\
                     0666 | IPC_CREAT);
    shmResultId = shmget(IPC_PRIVATE, sizeof(double),\
                     0666 | IPC_CREAT);
    SHM_BUFFER = (AllocationQuery*) shmat(shmId, NULL, 0);
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
    shmdt(SHM_BUFFER);
    shmctl(shmId, IPC_RMID, NULL);
    shmdt(GLOBAL_RESULT);
    shmctl(shmResultId, IPC_RMID, NULL);
    return 0;
};

int init_socket(const char* ip_addr)
{
    int listener;
    struct sockaddr_in addr;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    int keepalive = 1;
    int keepcnt = 5;
    int keepidle = 10;
    int keepintvl = 7;
    int reuseadrr = 1;

    setsockopt(listener, SOL_SOCKET, SO_KEEPALIVE,\
            &keepalive, sizeof(int));
    setsockopt(listener, IPPROTO_TCP, TCP_KEEPCNT,\
            &keepcnt, sizeof(int));
    setsockopt(listener, IPPROTO_TCP, TCP_KEEPIDLE,\
            &keepidle, sizeof(int));
    setsockopt(listener, IPPROTO_TCP, TCP_KEEPINTVL,\
            &keepintvl, sizeof(int));
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,\
            &reuseadrr, sizeof(int));

    if(listener < 0)
    {
        perror("socket");
        exit(1);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    if(bind(listener, (struct sockaddr *)&addr,\
             sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }

    listen(listener, 1);
    return listener;
}

sembuf sem_set(int sem_num, int sem_op, int sem_flg)
{
    sembuf semops;
    semops.sem_num = sem_num;
    semops.sem_op = sem_op;
    semops.sem_flg = sem_flg;

    return semops;
}

int check_distanses_step_positive()
{
    for(int i = 0; i < INTEGRAL_PARTS_NUMBER; i++)
        if(SHM_BUFFER->distancesQuery[i].step > 0)
            return 1;
    return 0;
}

Range search_near_distance()
{
    if(check_distanses_step_positive() == 0)
        return Range_ctor(0, 0, 0);

    int i = SHM_BUFFER->iterator;
    for(;SHM_BUFFER->distancesQuery[i].step < 0;\
        i = (i + 1) % INTEGRAL_PARTS_NUMBER);
    SHM_BUFFER->iterator = (i + 1) % INTEGRAL_PARTS_NUMBER;
    return SHM_BUFFER->distancesQuery[i];
}

int delete_one_part_from_shm(Answer answer)
{
    for(int i = 0 ; i < INTEGRAL_PARTS_NUMBER; i++)
    {
        if(SHM_BUFFER->distancesQuery[i].start == answer.start)
        {
            SHM_BUFFER->distancesQuery[i].step = -1;
            return 0;
        }
    }
    return 0;
};

int check_message_in_shm(double start)
{
    for(int i = 0; i < INTEGRAL_PARTS_NUMBER; i++)
    {
        if(SHM_BUFFER->distancesQuery[i].start == start)
        {
            if(SHM_BUFFER->distancesQuery[i].step > 0)
            {
                return 1;
            }else
            {
                return 0;
            }
        }
    }
    return 0;
}

int make_connection(char* ip_addr)
{
    int listener = init_socket(ip_addr);
    int sock = accept(listener, NULL, NULL);
    if(sock < 0)
    {
        perror("accept");
        exit(3);
    }
    int keepalive = 1;
    int keepcnt = 5;
    int keepidle = 10;
    int keepintvl = 7;
    int reuseadrr = 1;

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

    int exitFlag = 0;
    Range message;
    Answer answer;
    sembuf semops[2];
    while(1)
    {
        semops[0] = sem_set(0, 0, 0);
        semops[1] = sem_set(0, 1, 0);

        if(semop(semId, semops, 2))
        {
            perror("semop start");
            exit(0);
        }
        if(check_distanses_step_positive() == 1)
        {
            message = search_near_distance();
            send(sock, &message, sizeof(message), 0);
        }else
        {
            exitFlag = 1;
        }

        semops[0] = sem_set(0, -1, 0);
        if(semop(semId, semops, 1))
        {
            perror("semop end");
            exit(0);
        }
        if(exitFlag == 1)
        {
            break;
        }
        int bytes_read = recv(sock, &answer, sizeof(answer),\
                             MSG_ERRQUEUE);
        if(bytes_read <= 0) break;
        semops[0] = sem_set(0, 0, 0);
        semops[1] = sem_set(0, 1, 0);
        if(semop(semId, semops, 2))
        {
            perror("semop start");
            exit(0);
        }
        if(check_message_in_shm(answer.start))
        {
            localResult += answer.result;
            delete_one_part_from_shm(answer);
            printf("Get %lf\n", message.start);
        }
        semops[0] = sem_set(0, -1, 0);
        if(semop(semId, semops, 1))
        {
            perror("semop end");
            exit(0);
        }
    }
    printf("localResult: %lf\n", localResult);
    semops[0] = sem_set(0, 0, 0);
    semops[1] = sem_set(0, 1, 0);
    if(semop(semId, semops, 2))
    {
        perror("semop start");
        exit(0);
    }
    *GLOBAL_RESULT = *GLOBAL_RESULT + localResult;
    semops[0] = sem_set(0, -1, 0);
    if(semop(semId, semops, 1))
    {
        perror("semop end");
        exit(0);
    }
    close(sock);
    close(listener);
    return 0;
}

int create_n_process(int numberProcess, char* ip_addresses[])
{
    pid_t pid; 
    for(int i = 0; i < numberProcess; i++)
    {
        if((pid = fork()) == 0)
        {
            make_connection(ip_addresses[i]);
            exit(0);
        }
    }
    return 0;
}

int main(int argv, char** argc)
{
    int numberProcess = atoi(argc[1]);
    if(argv != 2 + numberProcess + 2)
    {
        perror("Use numberProcess ip1 ip2 ... start end");
        return 0;
    }
    double start = atof(argc[2 + numberProcess]);
    double end = atof(argc[2 + numberProcess + 1]);

    init_sem();
    init_shm();
    //char* ip_addresses[2];
    //ip_addresses[0] = "127.0.0.1";
    //ip_addresses[1] = "192.168.0.107";

    *SHM_BUFFER = AllocationQuery_ctor(start, end, STEP);

    create_n_process(numberProcess, &argc[2]);
    int status;
    for(int i = 0; i < numberProcess; i++)
        wait(&status);

    printf("RESULT: %lf", *GLOBAL_RESULT);
    free_sem();
    free_shm();

    return 0;
}