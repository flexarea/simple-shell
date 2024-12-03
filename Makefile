mysh: mysh.c
	gcc -Wall -pedantic -o $@ $^

test_in: test_in.c
	gcc -Wall -pedantic -o $@ $^

test_out: test_out.c
	gcc -Wall -pedantic -o $@ $^

.PHONY: strace
strace:
	strace -f -o strace.out ./mysh

.PHONY: all
all:
	gcc -Wall -pedantic -o test_in test_in.c
	gcc -Wall -pedantic -o test_out test_out.c
	gcc -Wall -pedantic -o mysh mysh.c

.PHONY: clean
clean:
	rm -f mysh test_in test_out