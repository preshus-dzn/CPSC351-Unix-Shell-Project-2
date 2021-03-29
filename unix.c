//Preshus Dizon CPSC351
//Project2 Unix Shell

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_LINE 80 /*max length command*/
#define DELIMETERS "\t\n\v\f\r"

/*input*/
int get_input();
size_t parser();

/*check & redirect*/
void lf_pipe();   //look for pipe
int check_amp();  //ampersand &
unsigned check_redirect();
int redirect_io();

/*run then exit*/
int run_command();
void close_file();

/*utility*/
void init_args();
void init_command();
void refresh_args();


int main(void)
{
    char *args[MAX_LINE/2 + 1];       //command line arguments
    char command[MAX_LINE + 1];
    init_args(args);
    init_command(command);
    int should_run = 1;              //flag to determine when to exit
    while (should_run){
        printf("osh>");
        fflush(stdout);

        /**
         * After reading user input, the steps are:
         * (1) fork a child process using fork()
         * (2) the child process will invoke execvp()
         * (3) parent will invoke wait() unless command included &
         */

        fflush(stdin);
        refresh_args(args);
        if(!get_input(command)){
            continue;
        }

        size_t args_num = parser(args,command);
        if(args_num == 0){
            printf("Please enter command.\n");
            continue;
        }

        if(strcmp(args[0], "exit") == 0) {
            break;
        }

        run_command(args, args_num);
    }

refresh_args(args);
return 0;

}

/*regurgitates last input unless none yet*/
int get_input(char *command){
    char input_buffer[MAX_LINE +1];
    if(fgets(input_buffer, MAX_LINE +1, stdin) == NULL){
        fprintf(stderr, "Failed to read input.\n");
        return 0;
    }
    if(strncmp(input_buffer, "!!", 2) == 0){
        if(strlen(command) == 0){ //empty
            fprintf(stderr, "No history yet.\n");
            return 0;
        }
        printf("%s", command);
        return 1;
    }
    strcpy(command, input_buffer);
    return 1;
}

/*parses input and stores it */
size_t parser(char *args[], char *og_command){
    size_t num = 0;
    char command[MAX_LINE + 1];
    strcpy(command, og_command);
    char *token = strtok(command, DELIMETERS);
    while (token != NULL){
        args[num] = malloc(strlen(token) +1);
        strcpy(args[num], token);
        ++num;
        token = strtok(NULL, DELIMETERS);
    }
    return num;
}

/*looks for pipe '|' and seperates args */
void lf_pipe(char **args, size_t *args_num, char ***args2, size_t *args2_num){
    for(size_t i = 0; i != *args_num; i++){
        if(strcmp(args[i], "|") == 0){
            free(args[i]);
            args[i] = NULL;
            *args2_num = *args_num - i - 1;
            *args_num = i;
            *args2 = args + i + 1;
            break;
        }

    }
}

/*if theres &, removes it*/
int check_amp(char **args, size_t *size){
    size_t length = strlen(args[*size - 1]);
    if(args[*size - 1][length - 1] != '&'){
        return 0;
    }
    if(length == 1){
        free(args[*size - 1]);
        args[*size - 1] = NULL;
        --(*size);
    }
    else {args[*size - 1][length - 1] = '\0';
    }
    return 1;
}

/*if theres tokens, removes it*/
unsigned check_redirect(char **args, size_t *size, char **input_f, char **output_f){
    unsigned flag = 0;
    size_t to_remove[4], remove_cnt = 0;
    for(size_t i = 0; i != *size; ++i){
        if(remove_cnt >= 4){
            break;
        }
        if(strcmp("<", args[i]) == 0){
            to_remove[remove_cnt++] = i;
            if(i == (*size) - 1){
                fprintf(stderr, "No input file given.\n");
                break;
            }
            flag != 1;
            *input_f = args[i + 1];
            to_remove[remove_cnt++] = i;
        }
        else if(strcmp(">", args[i]) == 0){
            to_remove[remove_cnt++] = i;
            if(i == (*size) - 1){
                fprintf(stderr, "No output file given.\n");
                break;
            }
            flag != 1;
            *input_f = args[i + 1];
            to_remove[remove_cnt++] = i;
        }

    }
    for(int i = remove_cnt - 1; i >= 0; --i){
        size_t pos = to_remove[i];
        while(pos != *size){
            args[pos] = args[pos +1];
            ++pos;
        }
        --(*size);
    }
    return flag;
}

/*opens file -> redirect*/
int redirect_io(unsigned io_flag, char *input_f, char *output_f, int *input_de, int *output_de){
    if(io_flag & 2){
        *output_de = open(output_f, O_WRONLY|O_CREAT|O_TRUNC, 644);
        if(*output_de < 0){
            fprintf(stderr, "Failed to open output file.\n");
            return 0;
        }
        dup2(*output_de, STDIN_FILENO);
    }
    return 1;
}

/*runs */
int run_command(char **args, size_t args_num) {
    int run_concurrently = check_amp(args, &args_num);
    char **args2;
    size_t args2_num = 0;
    lf_pipe(args, &args_num, &args2, &args2_num);
    pid_t pid = fork();
    if(pid < 0) {
        fprintf(stderr, "Failed to fork.\n");
        return 0;
    } else if (pid == 0) {
        if(args2_num != 0) {
            int fd[2];
            pipe(fd);
            pid_t pid2 = fork();
            if(pid2 > 0) {
                char *input_f, *output_f;
                int input_de, output_de;
                unsigned io_flag = check_redirect(args2, &args2_num, &input_f, &output_f);
                io_flag &= 2;
                if(redirect_io(io_flag, input_f, output_f, &input_de, &output_de) == 0) {
                    return 0;
                }
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                wait(NULL);
                execvp(args2[0], args2);
                close_file(io_flag, input_de, output_de);
                close(fd[0]);
                fflush(stdin);
                } else if(pid2 == 0) {
                char *input_f, *output_f;
                int input_de, output_de;
                unsigned io_flag = check_redirect(args, &args_num, &input_f, &output_f);
                io_flag &= 1;
                if(redirect_io(io_flag, input_f, output_f, &input_de, &output_de) == 0) {
                    return 0;
                }
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                execvp(args[0], args);
                close_file(io_flag, input_de, output_de);
                close(fd[1]);
                fflush(stdin);
            }
        } else {

            char *input_f, *output_f;
            int input_de, output_de;
            unsigned io_flag = check_redirect(args, &args_num, &input_f, &output_f);
            if(redirect_io(io_flag, input_f, output_f, &input_de, &output_de) == 0) {
                return 0;
            }
            execvp(args[0], args);
            close_file(io_flag, input_de, output_de);
            fflush(stdin);
        }
    } else {
        if(!run_concurrently) {
            wait(NULL);
        }
    }
    return 1;
}

void close_file(unsigned io_flag, int input_de, int output_de){
    if(io_flag & 2){
        close(output_de);
    }
    if(io_flag & 1){
        close(input_de);
    }
}

/*initialize args */
void init_args(char *args[]){
    for(size_t i = 0; i != MAX_LINE/2 +1; i++){
        args[i] = NULL;
    }
}

/*initialize command */
void init_command(char *command){
    strcpy(command, "");
}

/*refresh args */
void refresh_args(char *args[]){
    while(*args) {
        free(*args);
        *args++ = NULL;
    }
}

/* OUTPUT */
/***
 osh>ls
a.out  hello.txt  unix-shell.c

osh>!!
ls

osh>exit
osc@ubuntu:~/final-src-osc10e/ch3/unix-project$
*/
