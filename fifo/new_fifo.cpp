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

const char* WRITER_FIFO = "my_fifo";
const char* READER_FIFO = "command_fifo";
const int BUFF_SIZE 	= 1024;
const int CONNECTIONS_TIMING = 0;
const int TIME_TO_CONFIRM_CONNECTION = 10;

struct Writer
{
	pid_t writerPid;
	pid_t readerPid;
	char data[BUFF_SIZE];
	int dataSize;
	int packageNumber;
};

int writer_ctor(Writer* pack);
int writer_dtor(Writer* pack);
void writer_dump(Writer* pack);

struct Reader
{
	pid_t writerPid;
	pid_t readerPid;
	int packageNumber;
};

int reader_ctor(Reader* pack);
int reader_dtor(Reader* pack);
void reader_dump(Reader* pack);

int reader_connection(Reader* pack, pollfd reader_fifo, pollfd writer_fifo);
int writer_connection(Writer* pack, pollfd reader_fifo, pollfd writer_fifo);

int reader_connection_two(Reader* pack, pollfd reader_fifo, pollfd writer_fifo);
int writer_connection_two(Writer* pack, pollfd reader_fifo, pollfd writer_fifo);

int writer_sending(Writer* pack, pollfd reader_fifo, pollfd writer_fifo, FILE* file);
int reader_getting(Reader* pack, pollfd reader_fifo, pollfd writer_fifo);

int reader_main();
int writer_main(const char* filename);

void create_fifo(pollfd* fds, short events, int openFlag, const char* fifoName);

int main(int argc, char* argv[])
{
	switch(argc)
	{
		case 1:
			reader_main();
			break;
		case 2:
			writer_main(argv[1]);
			break;
		default:
			perror("BAD ARG");
			break;
	}
	return 0;
}

int reader_main()
{
	Reader client;
	reader_ctor(&client);
	
	pollfd writer;
	create_fifo(&writer, POLLIN, O_RDONLY, WRITER_FIFO);
	pollfd reader;
	create_fifo(&reader, POLLOUT, O_WRONLY, READER_FIFO);

	do{
		reader_ctor(&client);
		reader_connection(&client, reader, writer);
		reader_dump(&client);
	}while(reader_connection_two(&client, reader, writer));

	reader_getting(&client, reader, writer);

	reader_dtor(&client);
	return 0;
}

int writer_main(const char* filename)
{
	FILE* dataFile;
	if(!(dataFile = fopen(filename, "r")))
	{
		perror("BAD FILE OPEN");
		return -1;
	}

	Writer server;
	writer_ctor(&server);

	pollfd writer;
	create_fifo(&writer, POLLOUT, O_WRONLY, WRITER_FIFO);
	pollfd reader;
	create_fifo(&reader, POLLIN, O_RDONLY, READER_FIFO);

	do{
		writer_ctor(&server);
		writer_connection(&server, reader, writer);
	}while(writer_connection_two(&server, reader, writer));

	writer_sending(&server, reader, writer, dataFile);

	writer_dtor(&server);
	fclose(dataFile);
	return 0;
}

int writer_ctor(Writer* pack)
{
	pack->writerPid = getpid();
	pack->readerPid = 0;
	pack->packageNumber = 0;
	pack->dataSize = 0;
	return 0;
}
int writer_dtor(Writer* pack)
{
	pack->writerPid = -1;
	pack->readerPid = -1;
	pack->packageNumber = -1;
	return 0;
}
void writer_dump(Writer* pack)
{
	printf("Pack Writer: \n");
	printf("writerPid = %d\n:", pack->writerPid);
	printf("readerPid = %d\n:", pack->readerPid);
	printf("packageNumber = %d\n", pack->packageNumber);
}

int reader_ctor(Reader* pack)
{
	pack->writerPid = 0;
	pack->readerPid = getpid();
	pack->packageNumber = 0;
	return 0;
}
int reader_dtor(Reader* pack)
{
	pack->writerPid = -1;
	pack->readerPid = -1;
	pack->packageNumber = -1;
	return 0;
}
void reader_dump(Reader* pack)
{
	printf("Pack Reader: \n");
	printf("writerPid = %d\n:", pack->writerPid);
	printf("readerPid = %d\n:", pack->readerPid);
	printf("packageNumber = %d\n", pack->packageNumber);
}
void create_fifo(pollfd* fds, short events, int openFlag, const char* fifoName)
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

int reader_connection(Reader* pack, pollfd reader_fifo, pollfd writer_fifo)
{
	Writer message;
	while(true)
	{
		switch(poll(&writer_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD WRITE IN FIFO");
				return -1;
			default:
				read(writer_fifo.fd, &message, sizeof(message));
				if(message.readerPid == 0)
				{
					pack->writerPid = message.writerPid;
					break;
				}
				if((message.readerPid == pack->readerPid)&&(message.packageNumber == 0))
				{
					return 0;
				}
				break;
		}
		switch(poll(&reader_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD READ IN FIFO");
				return -1;
			default:
				if(pack->writerPid != 0)
					write(reader_fifo.fd, pack, sizeof(*pack));
		}
	}
	return 0;
}

int reader_connection_two(Reader* pack, pollfd reader_fifo, pollfd writer_fifo)
{
	Writer message;
	for(int i = 0; i < TIME_TO_CONFIRM_CONNECTION; i++)
	{
		switch(poll(&writer_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD WRITE IN FIFO");
				return -1;
			default:
				read(writer_fifo.fd, &message, sizeof(message));
				if((message.readerPid != pack->readerPid)
					&&(message.writerPid == pack->writerPid)
					&&(message.readerPid == 0))
				{
					return -1;
				}
				if((message.readerPid == pack->readerPid)
					&&(message.writerPid == pack->writerPid)
					&&(message.packageNumber == 0))
				{
					return 0;
				}
				break;
		}
		switch(poll(&reader_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD READ IN FIFO");
				return -1;
			default:
				if(pack->writerPid != 0)
					write(reader_fifo.fd, pack, sizeof(*pack));
		}
	}
	return 1;
}

 
int writer_connection(Writer* pack, pollfd reader_fifo, pollfd writer_fifo)
{
	Reader message;
	while(true)
	{
		switch(poll(&writer_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD WRITE IN FIFO");
				return -1;
			default:
				write(writer_fifo.fd, pack, sizeof(*pack));
		}
		switch(poll(&reader_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD READ IN FIFO");
				return -1;
			default:
				read(reader_fifo.fd, &message, sizeof(message));
				if((message.writerPid == pack->writerPid)&&(message.packageNumber == 0))
				{
					pack->readerPid = message.readerPid;
					return 0;
				}
		}
	}
	return 0;
}

int writer_connection_two(Writer* pack, pollfd reader_fifo, pollfd writer_fifo)
{
	Reader message;
	for(int i = 0; i < TIME_TO_CONFIRM_CONNECTION; i++)
	{
		switch(poll(&writer_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD WRITE IN FIFO");
				return -1;
			default:
				write(writer_fifo.fd, pack, sizeof(*pack));
		}
		switch(poll(&reader_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD READ IN FIFO");
				return -1;
			default:
				read(reader_fifo.fd, &message, sizeof(message));
				if((message.writerPid == pack->writerPid)
					&&(message.readerPid != pack->readerPid)
					&&(message.readerPid != 0))
				{
					return -1;
				}
				if((message.writerPid == pack->writerPid)
					&&(message.readerPid == pack->readerPid)
					&&(message.packageNumber == 0))
				{
					return 0;
				}
		}
	}
	return 1;
}

int writer_sending(Writer* pack, pollfd reader_fifo, pollfd writer_fifo, FILE* file)
{
	Reader message;
	while(true)
	{
		switch(poll(&writer_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD WRITE IN FIFO");
				return -1;
			default:
				write(writer_fifo.fd, pack, sizeof(*pack));
				break;
		}
		switch(poll(&reader_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD READ IN FIFO");
				return -1;
			default:
				read(reader_fifo.fd, &message, sizeof(message));
				if((message.readerPid == pack->readerPid)
					&&(message.writerPid == pack->writerPid)
					&&(message.packageNumber == pack->packageNumber))
				{
					pack->dataSize = fread(pack->data, sizeof(*pack->data), BUFF_SIZE, file);
					if(pack->dataSize == 0)
					{
						perror("WRITER END");
						return 0;
					}
					pack->packageNumber ++;
					break;
				}
		}
	}
}

int reader_getting(Reader* pack, pollfd reader_fifo, pollfd writer_fifo)
{
	perror("READER START");
	Writer message;
	while(true)
	{
		switch(poll(&writer_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD WRITE IN FIFO");
				return -1;
			default:
				read(writer_fifo.fd, &message, sizeof(message));
				if((message.readerPid == pack->readerPid)
					&&(message.writerPid == pack->writerPid)
					&&(message.packageNumber == pack->packageNumber + 1))
				{
					if((message.packageNumber > 0)&&(message.dataSize == 0))
					{
						perror("READER END");
						return 0;
					}
					pack->packageNumber++;
					write(STDOUT_FILENO, message.data, message.dataSize);
					break;
				}
				if((pack->writerPid != message.writerPid)&&(pack->readerPid == message.writerPid))
				{
					return 0;
				}
		}
		switch(poll(&reader_fifo, 1, CONNECTIONS_TIMING))
		{
			case 0:
				break;
			case -1:
				perror("BAD READ IN FIFO");
				return -1;
			default:
				write(reader_fifo.fd, pack, sizeof(*pack));
				break;
		}
	}
	return 0;
}