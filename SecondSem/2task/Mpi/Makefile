CPPFLAGS = -MD

all: a.out

-include *.d

a.out: main.cpp
	mpicxx -o $@ $^

clean:
	rm -rf *.o *.out *.d

.PHONY: all clean