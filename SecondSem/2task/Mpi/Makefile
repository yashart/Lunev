CPPFLAGS = -MD

all: a.out

-include *.d

a.out: main.cpp
	mpic++ -o $@ $^

clean:
	rm -rf *.o *.out *.d

.PHONY: all clean