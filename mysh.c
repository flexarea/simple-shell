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
void free_tokens (char ** tokens, int length) {
    for (int i = 0; i < length; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

int main (int argc, char * argv[]) {
    printf("Mysh: ");
    char cmd_buff[4096];
    // Cursed pointers ahead
    char *** pipe_tokens = malloc(2 * sizeof(char **)); // ERROR CHECK ME!!!!
    int num_pipes = 0;
    while (fgets(cmd_buff, sizeof(cmd_buff), stdin) != NULL) {
        // Commands have a max of 10 args, so total of 11 b/c cmd name counts?
        char ** tokens = malloc(11 * sizeof(char *));
        char *token;
        int op_flag = 0;
        int counter = 0;
        // Initial Setup
        if ((token = strtok(cmd_buff, " ")) == NULL) {
            return -1;
        }
        token = strip_new_line(token);
        // Handle exit
        if (strcmp(token, "exit") == 0) {
            free(tokens);
            return 0;
        }
        tokens[counter++] = token; // The ++ is on purpose!
        while ((token = strtok(NULL, " ")) != NULL) {
            int initial_fd;
            pid_t child;
            if (op_flag != 0 && op_flag != 4) {
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
                //char * file_name = strip_new_line(token);
                if (child == 0) {
                    if ((initial_fd = open(tokens[0], open_mode)) < 0) {
                    perror("open");
                    exit(-1);
                };
                dup2(initial_fd, new_fd); // ERROR CHECK ME!
                close(initial_fd); // ERROR CHECK ME!
                execv(tokens[0], tokens); //ERROR CHECK ME!
                exit(0);
                } else {
                    wait(NULL);
                    break;
                } 
            }
            if (op_flag == 4) {
                // HANDLE PIPE
                pipe_tokens[num_pipes] = tokens;
                tokens = malloc(11 * sizeof(char *)); // ERROR CHECK ME!
                num_pipes += 1;
                counter = 0; // RESET counter
            }

            if (strcmp(token,"<") == 0) {
                op_flag = 1; // In redirect
            } else if (strcmp(token,">") == 0) {
                op_flag = 2; // Out redirect
            } else if (strcmp(token,">>") == 0) {
                op_flag = 3; // Out redirect special
            } else if (strcmp(token,"|") == 0) {
                op_flag = 4; // Pipe
            } else {
                // Save (stripped) token into tokens then increment
                tokens[counter++] = strip_new_line(token);
                
                if (counter % 10 == 0) {
                    // Resize if there are somehow more than 10 optional args
                    // Technically not needed, but it can't hurt right?
                    tokens = realloc(tokens,counter + 10);
                }
            }
        }
        // Loop has ended, we need to handle pipe here for whatever the last argument is.
        // I need a list of toekns. 
        if (op_flag == 4) {
            for (int i = 0; i < num_pipes, i++) {
                //Process and fork
                // Inside child:
                //    handle_pipe(_, _, _)
            }
        }
        if (op_flag == 0) {
            pid_t new_child = fork();
            if (new_child == 0) {
                if (execv(tokens[0], tokens) == -1) {
                    perror("execv");
                }
                exit(0);
            } else {
                 wait(NULL);
            }
        }
        free_tokens(tokens, counter);
        op_flag = 0;
        printf("Mysh: ");
    }
    return 0;
}

void * handle_pipe(int location, int pipefd[2], char ** tokens) {
    /*
    location = 0 -> first cmd:
        - dup2 stdout -> pipefd[0]
    location = 1 -> any other cmd:
        - dup2 stdout -> pipefd[0]
        - dup2 stdin -> pipefd[1]
    location = 2 -> last cmd:
        - dup2 stdin -> pipefd[1]
    ALL:
    execv(tokens[0], tokens);
    */
}