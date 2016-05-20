all: client.out server.out

client.out: client.o
	g++ -o $@ $^

server.out: server.o
	g++ -o $@ $^

clean:
	rm -rf *.o *.out *.d

.PHONY: all clean