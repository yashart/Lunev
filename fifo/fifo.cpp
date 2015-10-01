//
// Created by yashart for lunev
//
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

const char* FIFO_NAME 		= "my_fifo";
const char* COMMANDS_FIFO	= "command_fifo";
const int BUFF_SIZE			= 1024;

enum sendT
{
	DATA 	= 0,
	COMMAND = 1,
	DEATH	= 2
};

struct package
{
	pid_t pid;
	sendT type;
	int dataSize;
	char data[BUFF_SIZE];
	int packNum;
};
void packageCtor(package* pack);
void packageDtor(package* pack);
void packageDump(package* pack);

struct command
{
	pid_t pid;
	int packNum;
};
void commandCtor(command* cmd);
void commandDtor(command* cmd);
void commandDump(command* cmd);

void createFifo(pollfd* fds, short events, int openFlag, const char* fifoName);

int startClient(char* filename);
int startServer();

int main(int argc, char* argv[])
{
	switch(argc)
	{
		case 1:
			startServer();
			break;
		case 2:
			startClient(argv[1]);
			break;
		default:
			perror("BAD ARG");
			break;
	}
	exit(0);
	return 0;
}

int startClient(char* filename)
{
	pollfd fds;
	createFifo(&fds, POLLOUT, O_WRONLY, FIFO_NAME);
	
	pollfd commandFifo;
	createFifo(&commandFifo, POLLIN, O_RDONLY, COMMANDS_FIFO); 

	errno = 0;

	FILE* rFile = 0;
	if(!(rFile = fopen(filename, "r")))
	{
		perror("BAD FILE OPEN");
		return -1;
	}
	package pack;
	packageCtor(&pack);
	pid_t serverPid = 0;
	command cmd;
	commandCtor(&cmd);
	pack.packNum = 0;
	for(int packNum = 0; 
		(pack.dataSize = fread(pack.data, sizeof(*pack.data), BUFF_SIZE, rFile)) > 0;
		 packNum ++)
	{
		while(packNum == pack.packNum)
		{
			switch(poll(&fds, 1, 11))
			{
				case 0:
					break;
				case -1:
					perror("BAD WRITE IN FIFO");
					return -1;
				default:
					if(write(fds.fd, &pack, sizeof(pack)) != sizeof(pack))
					{
						perror("BAD WRITE IN FIFO");
						return -1;
					}
					break;
			}
			switch(poll(&commandFifo, 1, 11))
			{
				case 0:
					break;
				case -1:
					perror("BAD READ COMMAND");
					return -1;
				default:
					if(!serverPid)
					{
						read(commandFifo.fd, &cmd, sizeof(cmd));
						serverPid = cmd.pid;
						pack.packNum = cmd.packNum;
						break;
					}
					read(commandFifo.fd, &cmd, sizeof(cmd));
					if(serverPid != cmd.pid)
					{
						break;
					}
					pack.packNum = cmd.packNum;
					break;
			}
		}
	}
	packageDtor(&pack);
	commandDtor(&cmd);
	fclose(rFile);
	close(fds.fd);
	close(commandFifo.fd);
	return 0;
}

int startServer()
{
	pollfd fds;
	createFifo(&fds, POLLIN, O_RDONLY, FIFO_NAME);

	pollfd commandFifo;
	createFifo(&commandFifo, POLLOUT, O_WRONLY, COMMANDS_FIFO);

	errno = 0;

	int readQuant = 0;
	package pack;
	packageCtor(&pack); 
	package truePack;
	packageCtor(&truePack);
	pack.packNum = 0;
	pid_t clientPid = 0;
	command cmd;
	commandCtor(&cmd);
	for(int packNum = 0; 1; packNum ++)
	{
		while(packNum == truePack.packNum)
		{
			switch(poll(&fds, 1, 11))
			{
				case 0:
					break;
				case -1:
					perror("BAD FIFO READ");
					return -1;
				default:
					readQuant = read(fds.fd, &pack, sizeof(pack));
					if(readQuant != sizeof(pack))
					{
						perror("BAD FIFO READ SIZE");
						return -1;
					}
					if(!clientPid)
					{
						clientPid 	= pack.pid;
						cmd.packNum = packNum + 1;
						break;
					}
					if(clientPid != pack.pid)
						break;
					cmd.packNum = packNum + 1;
					truePack = pack;
					break;
			}
			switch(poll(&commandFifo, 1, 11))
			{
				case 0:
					break;
				case -1:
					perror("BAD COMMAND WRITE");
					return -1;
				default:
					write(commandFifo.fd, &cmd, sizeof(cmd));
					break;
			}
		}

		if((truePack.type == DATA)&&(write(STDOUT_FILENO, truePack.data, truePack.dataSize) == -1))
		{
			return -1;
		}
		truePack.type = COMMAND;
	}
	packageDtor(&pack);
	packageDtor(&truePack);
	commandDtor(&cmd);
	close(fds.fd);
	close(commandFifo.fd);
	return 0;
}

void packageCtor(package* pack)
{
	pack->pid 		= getpid();
	pack->type 		= DATA;
	pack->dataSize 	= 0;
	pack->packNum 	= 0;
}

void packageDtor(package* pack)
{
	pack->pid 		= -1;
	pack->type 		= DEATH;
	pack->dataSize 	= -1;
}

void packageDump(package* pack)
{
	printf("pid - %d, packNum - %d\n", pack->pid, pack->packNum);
	printf("Type - %d, dataSize - %d\n", pack->type, pack->dataSize);
	for(int i = 0; i < pack->dataSize; i++)
		printf("%c", pack->data[i]);
}

void createFifo(pollfd* fds, short events, int openFlag, const char* fifoName)
{
	if((mkfifo(fifoName, 0777) == -1)&&(errno != EEXIST))
	{
		perror("BAD FIFO CREATE");
	}
	
	if((fds->fd = open(fifoName, openFlag)) <= 0)
	{
		perror("BAD SERVER OPEN PIPE");
	}
	fds->events = events;
}

void commandCtor(command* cmd)
{
	cmd->pid 	 = getpid();
	cmd->packNum = 0;
}

void commandDtor(command* cmd)
{
	cmd->pid 	 = -1;
	cmd->packNum = -1;
}

void commandDump(command* cmd)
{
	printf("Command: pid = %d, packNum = %d", cmd->pid, cmd->packNum);
}