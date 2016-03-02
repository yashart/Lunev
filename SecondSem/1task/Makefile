CPPFLAGS = -MD

all: a.out

-include *.d

a.out: test.o binary_tree.o
	g++ -o $@ $^

clean:
	rm -rf *.o *.out *.d

.PHONY: all clean