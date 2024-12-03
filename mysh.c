#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#define LAST_CMD 2
#define FIRST_CMD 0

void handle_pipe(int type, int fd_1, int fd_2, char ** tokens);

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
    char *** pipe_tokens;
    if ((pipe_tokens = malloc(2 * sizeof(char **))) == NULL) {
        return -1;
    }
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
        if (strcmp(token, "\n") == 0) {
            printf("Mysh: ");
            continue;
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
                if ((child = fork()) == -1) {
                    perror("fork");
                    return -1;
                }
                char * file_name = strip_new_line(token);
                if (child == 0) {
                    if ((initial_fd = open(file_name, open_mode)) < 0) {
                        perror("open");
                        exit(-1);
                    };
                free(file_name);
                dup2(initial_fd, new_fd); // ERROR CHECK ME!
                close(initial_fd); // ERROR CHECK ME!
                if (execvp(tokens[0], tokens) == -1){
                    perror("execvp");
                    return -1;
                }
                exit(0);
                } else {
                    wait(NULL);
                    break;
                } 
            }
            if (op_flag == 4) {
                pipe_tokens[num_pipes] = tokens;
                if ((tokens = malloc(11 * sizeof(char *))) == NULL) {
                    return -1;
                }
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
        pipe_tokens[num_pipes] = tokens;
        if (op_flag == 4) {
            int pipefd[2];
            for (int i = 0; i <= num_pipes; i++) {
                // Process and fork
                char ** tok = pipe_tokens[i];
                if (i % 2 == 0) {
                    if (pipe(pipefd) == -1) {
                        perror("pipe");
                        return(-1);
                    }
                }
                // fixed weird seg fualt thing (I think)
                int save_0 = pipefd[0];
                int save_1 = pipefd[1];
                pid_t child;
                if ((child = fork()) == -1) {
                    perror("fork");
                    return(-1);
                }
                if (child == 0) {
                    int type = 1;
                    if (i == 0) {
                        type = 0;
                    }
                    if (i == num_pipes) {
                        type = 2;
                    }
                    handle_pipe(type, save_1, save_0, tok);
                }
            }
            for (int i = 0; i <= num_pipes; i++) {
                wait(NULL);
            }
        }
        if (op_flag == 0) {
            pid_t new_child;
            if ((new_child = fork()) == -1) {
                perror("fork");
                return(-1);
            }
            if (new_child == 0) {
                if (execvp(tokens[0], tokens) == -1) {
                    perror("execv");
                    exit(-1);
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

void handle_pipe(int type, int write_fd, int read_fd, char ** tokens) {
    /*
    type = 0 -> first cmd:
        - dup2 stdout -> write_fd
    type = 1 -> any other cmd:
        - dup2 stdout -> write_fd
        - dup2 stdin -> read_fd
    type = 2 -> last cmd:
        - dup2 stdin -> read_fd
    */
    if (type != LAST_CMD){
        if (dup2(write_fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(-1);
        }
    }
    if (type != FIRST_CMD) {
       if (dup2(read_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(-1);
        }
    }
    if (execvp(tokens[0], tokens) == -1) {
        perror("execvp");
        exit(-1);
    }
    exit(0);
}