#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#define LAST_CMD 1
#define FIRST_CMD 0

struct p_tkn {
    char ** tkns;
    int len;
};

void handle_pipe(int type, int fd_1, int fd_2, char ** tokens);

void strip_new_line (char * input) {
    char * new_line;
    if ((new_line = strchr(input, '\n')) != NULL) {
        *new_line = '\0';
    }
}

int main (int argc, char * argv[]) {
    printf("Mysh: ");
    char cmd_buff[4096];
    while (fgets(cmd_buff, sizeof(cmd_buff), stdin) != NULL) {
        // Commands have a max of 10 args, so total of 11 b/c cmd name counts
        char ** tokens;
        if ((tokens = malloc(11 * sizeof(char *))) == NULL) {
            return -1;
        }
        // An array of tokens (defualt size 2) realloc'd for more later in needed (not done)
        struct p_tkn * pipe_tkns;
        // Allocate 2 p_tkn's' by defualt since thats the min.
        int pipe_tkns_len = 2;
        if ((pipe_tkns = malloc(2 * sizeof(struct p_tkn))) == NULL) {
            return -1;
        }
        char *token;
        int op_flag = 0;
        int counter = 0;
        int pipe_counter = 0;
        // Initial Setup
        if ((token = strtok(cmd_buff, " ")) == NULL) {
            return -1;
        }
        if (strcmp(token, "\n") == 0) {
            printf("Mysh: ");
            continue;
        }
        strip_new_line(token);
        // Handle exit
        if (strcmp(token, "exit") == 0) {
            free(tokens);
            return 0;
        }
        tokens[counter] = token;
        counter += 1;
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
                strip_new_line(token);
                if (child == 0) {
                    if ((initial_fd = open(token, open_mode)) < 0) {
                        perror("open");
                        exit(-1);
                    };
                //free(file_name);
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
            if (strcmp(token,"<") == 0) {
                op_flag = 1; // In redirect
            } else if (strcmp(token,">") == 0) {
                op_flag = 2; // Out redirect
            } else if (strcmp(token,">>") == 0) {
                op_flag = 3; // Out redirect special
            } else if (strcmp(token,"|") == 0) {
                op_flag = 4; // Pipe
                if (pipe_counter >= pipe_tkns_len) {
                    // Allocate two extra p_tkn's
                    pipe_tkns_len = pipe_counter + 2;
                    if ((pipe_tkns = realloc(pipe_tkns, pipe_tkns_len * sizeof(struct p_tkn))) == NULL) {
                        perror("realloc");
                        return -1;
                    };
                }
                //printf("Saving token %s to :%d\n",tokens[0], pipe_counter);
                pipe_tkns[pipe_counter].tkns = tokens;
                pipe_tkns[pipe_counter].len = counter - 1; 
                counter = 0;
                pipe_counter += 1;
                if ((tokens = malloc(11 * sizeof(char *))) == NULL) {
                    return -1;
                }
            } else {
                // Save (stripped) token into tokens then increment
                strip_new_line(token);
                tokens[counter] = token;
                counter += 1;
                if (counter % 10 == 0) {
                    // Resize if there are somehow more than 10 optional args
                    // Technically not needed, but it can't hurt right?
                    tokens = realloc(tokens, (counter + 10) * sizeof(char *));
                }
            }
        }
        
        pipe_tkns[pipe_counter].tkns = tokens;
        pipe_tkns[pipe_counter].len = counter - 1;
        if (op_flag == 4) {
            int pipe_fd[2];
            pipe(pipe_fd);
            for (int i = 0; i <= pipe_counter; i ++) {
                // printf("%s\n",pipe_tkns[i].tkns[0]);
                pid_t new_child;
                
                if ((new_child = fork()) == -1) {
                    perror("fork");

                    return(-1);
                }
                if (new_child == 0) {
                    handle_pipe(i, pipe_fd[1], pipe_fd[0], pipe_tkns[i].tkns);
                    exit(0);
                }
            }
            for (int i = 0; i <= pipe_counter; i++) {
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
}