#include <stdio.h>

int main (int argc, char * argv[]) {
    char line[2500];
    fgets(line, sizeof(line), stdin);
    puts("this is the string I received from stdin: ");
    printf("%s", line);
}