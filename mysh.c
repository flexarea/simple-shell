#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

char * strip_new_line (char * input) {
    char c = *input;
    char *out = malloc(100);
    int counter = 0;
    // Iterate until we find null char or new line
    while (c != '\n' && c != '\0') {
        if (counter % 100 == 0) {
            out = (char *) realloc(out, counter + 100);
        }
        out[counter++] = c;
        c = input[counter];
    }
    // Tack on null char
    out[counter] = '\0';
    out = (char *) realloc(out, counter);
    return out;
}

int main (int argc, char * argv[]) {
    printf("Mysh: ");
    char cmd_buff[4096];

    while (fgets(cmd_buff, sizeof(cmd_buff), stdin) != NULL) {
        char ** tokens = malloc(10*sizeof(char *));
        char *token = strtok(cmd_buff, " ");
        char * first_token = strip_new_line(token);
        int op_flag = 0;
        int counter = 0;

        if (strcmp(first_token, "exit") == 0) {
            free(tokens);
            free(first_token);
            return 0;
        }
        tokens[counter++] = token; // The ++ is on purpose!
        free(first_token);
        while ((token = strtok(NULL, " ")) != NULL) {
            int initial_fd;
            pid_t child;

            if (op_flag == 4) {
                // HANDLE PIPE
                puts("Pipe failed. Too bad thats not implemented yet.");
                return -1;
            }
            if (op_flag != 0) {
                // HANDLE ALL  OTHER OPS
                int open_mode;
                int new_fd = STDOUT_FILENO; // Defualt to dup STDOUT
                switch (op_flag) {
                    case 1: 
                        open_mode = O_RDONLY;
                        new_fd = STDIN_FILENO; // Special case, dup STDIN
                        break;
                    case 2:
                        open_mode = O_WRONLY | O_TRUNC;
                        break;
                    default:
                        open_mode = O_WRONLY | O_APPEND;
                        break;
                }
                child = fork(); // ERROR CHECK ME?
                char * file_name = strip_new_line(token);
                if (child == 0) {
                    if ((initial_fd = open(file_name, open_mode)) < 0) {
                    perror("open");
                    exit(-1);
                };
                dup2(initial_fd, new_fd); // ERROR CHECK ME!
                close(initial_fd); // ERROR CHECK ME!
                execv(tokens[0], tokens); //ERROR CHECK ME!
                free(file_name);
                exit(0);
                }else{
                    wait(NULL);
                    break;
                } 
            }
            if (strcmp(token,"<") == 0) {
                op_flag = 1; // In redirect
            } else if (strcmp(token,">") == 0) {
                op_flag = 2; // Out redirect
            } else if (strcmp(token,">>") == 0) {
                op_flag = 3; // Out redirect special
            } else if (strcmp(token,"|") == 0) {
                op_flag = 4; // Pipe
            }else {
                // Save token into tokens then increment
                tokens[counter++] = token;
                if (counter % 10 == 0) {
                    // Resize if there are somehow more than 10 optional args
                    tokens = realloc(tokens,counter + 10);
                }
            }
        }
        free(tokens);
        printf("Mysh: ");
    }
    return 0;
}