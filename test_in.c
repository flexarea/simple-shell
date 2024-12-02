#include <stdio.h>

int main (int argc, char * argv[]) {
    char line[2500];
    fgets(line, sizeof(line), stdin);
    printf("%s\n", line);
}