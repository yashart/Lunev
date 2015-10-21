#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSGSZ     128

int check_number(long int* number, char* num);

struct MsgMag 
{
	long type;
	int procNum;
};
int msgMagCtor(MsgMag* msg, long number, int pid);

int main(int argc, char* argv[])
{
	if(argc != 2)
    {
        printf("It isn't 1 number");
        return 0;
    }
    long int number = 0;
    if(check_number(&number, argv[1]))
        return 0;


    pid_t pid = 0;
    key_t fileKey = ftok("./query", 'S');
    int query = 0;
    if((query = msgget(fileKey, 0666 | IPC_CREAT)) < 0)
    {
        perror("msgget");
        exit(1);
    }

    MsgMag queryMessage;

    int i = 0;
    for(i = 1; i <= number; i++)
    {
    	pid = fork();
        if(pid == 0)
        {
            printf("Send pack № %d, pid %d\n", i, getpid());
            break;
        }
    }
    if(pid != 0)
    {
        msgMagCtor(&queryMessage, 1, getpid());
        msgsnd(query, &queryMessage, sizeof(queryMessage.procNum), 0);
        msgrcv(query, &queryMessage, sizeof(queryMessage) - sizeof(queryMessage.type), number + 1, 0);
        msgctl(query, IPC_RMID, NULL);
    	return 0;
    }

    int bytes = 0;
    if((bytes = msgrcv(query, &queryMessage, sizeof(queryMessage) - sizeof(queryMessage.type), i, 0)) != -1)
    {
        printf("%d bytes %d pid %d message pid %d\n", i, bytes, getpid(), queryMessage.procNum);
        msgMagCtor(&queryMessage, i+1, getpid());
        msgsnd(query, &queryMessage, sizeof(queryMessage.procNum), 0);
        return 0;
    }
    return 0;
}

int check_number(long int* number, char* num)
{
    errno = 0;
    char* endptr = NULL;
    *number = strtol(num, &endptr, 0);
    if(errno == ERANGE)
    {
        printf("bad range");
        return -1;
    }
    if(errno == EINVAL)
    {
        printf("it isn't number");
        return -1;
    }
    if((endptr != NULL) && (*endptr != '\0'))
    {
        printf("it isn't number");
        return -1;
    }
    return 0;
}

int msgMagCtor(MsgMag* msg, long type, int pid)
{
	msg->type = type;
	msg->procNum = pid;
	return 0;
}