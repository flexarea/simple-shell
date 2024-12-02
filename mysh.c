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
        char *token = strtok(cmd_buff, " ");
        if (strcmp(token, "exit\n") == 0 || strcmp(token, "exit") == 0) {
            // First word is "exit" (if no new line, then exit is not the only word in the cmd)
            return 0;
        }
        int op_flag = 0;
        //int counter = 0;
        char * cmd = token;
        // char * opts[25]; 
        while ((token = strtok(NULL, " ")) != NULL) {
            int initial_fd;
            pid_t child;
            switch (op_flag) {
                // CASES 1,2,3 are the same except for the flag that goes into open. Should just combine them...
                case 1:
                    puts("Handle input redirect");
                    child = fork(); // ERROR CHECK ME?
                    if (child == 0) {
                        if ((initial_fd = open(strip_new_line(token), O_RDONLY)) < 0) {
                        perror("open");
                        exit(-1);
                    };
                    dup2(initial_fd, STDIN_FILENO); // ERROR CHECK ME!
                    close(initial_fd);
                    execl(cmd,cmd,NULL); //ERROR CHECK ME!
                    exit(0);
                    }else{
                        wait(NULL);
                        break;
                    } 
                case 2:
                    // Handle output redirect (trunc)
                    puts("Handle output redirect");
                    child = fork(); // ERROR CHECK ME?
                    if (child == 0){
                        if ((initial_fd = open(strip_new_line(token), O_WRONLY | O_TRUNC)) < 0) {
                            perror("open");
                            exit(-1);
                        }
                        dup2(initial_fd, fileno(stdout)); // ERROR CHECK ME!
                        close(initial_fd);
                        execl(cmd,cmd,NULL); //ERROR CHECK ME!
                        exit(0);
                    }else{
                        wait(NULL);
                        break;
                    }
                case 3:
                    child = fork(); // ERROR CHECK ME?
                    if (child == 0){
                        if ((initial_fd = open(strip_new_line(token), O_WRONLY | O_APPEND)) < 0) {
                            perror("open");
                            exit(-1);
                        }
                        dup2(initial_fd, fileno(stdout)); // ERROR CHECK ME!
                        close(initial_fd);
                        execl(cmd,cmd,NULL); //ERROR CHECK ME!
                        exit(0);
                    }else{
                        wait(NULL);
                        break;
                    }
                    break;
                case 4:
                    // Handle pipe
                    break;
                default:
                    break;
            }
            /* NOTE: ++ returns value before it increments, so technically this does opts[counter] then increments counter */
            //printf("%s\n", token);
            if (strcmp(token,"<") == 0) {
                op_flag = 1; // In redirect
            }
            if (strcmp(token,">") == 0) {
                op_flag = 2; // Out redirect
            }
            if (strcmp(token,">>") == 0) {
                op_flag = 3; // Out redirect special
            }
            if (strcmp(token,"|") == 0) {
                op_flag = 4; // Pipe
            }
        }
        printf("Mysh: ");
    }
    return 0;
}