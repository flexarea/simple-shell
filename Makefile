mysh: mysh.c
	gcc -Wall -pedantic -o $@ $^

.PHONY: clean
clean:
	rm -f mysh