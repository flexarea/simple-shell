#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char * argv[]) {
    printf("Mysh: ");
    char cmd_buff[4096];
    while (fgets(cmd_buff, sizeof(cmd_buff), stdin) != NULL) {
        char *token = strtok(cmd_buff, " ");

        if (strcmp(token, "exit\n") == 0 || strcmp(token, "exit") == 0) {
            // First word is "exit" (if no new line, then exit is not the only word in the cmd)
            return 0;
        }
        /* if (fork() == 0) {
            puts("ive forked");
            //execv(token,args);
        } */
        char * cmd = token;
        while ((token = strtok(NULL, " ")) != NULL) {
            printf("%s\n", token);
            if (strcmp(token,"<") == 0) {
                puts("in redirect");

            }
            if (strcmp(token,">") == 0) {
                puts("out redirect");
            }
            if (strcmp(token,">>") == 0) {
                puts("out redirect append");
            }
            if (strcmp(token,"|") == 0) {
                puts("pipe");
            }
        }
    }
    return 0;
}