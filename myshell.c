//
// Created by dario on 10/08/2022.
//
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/wait.h>

#define MAXBUFF 1000

struct opts{
    bool background;
    bool chIn;
    bool chOut;
    bool truncate;
    bool pipe;
    int pipeIndex;
    char **args;
    int arglen;
    char **env;
    char *newIn;
    char *newOut;

};

//batchmode is invoked if argc is 2 and will open the file given for running
int batchMode(struct opts *input, char *filePath);
//we need to reset the opts struct every time the last input is given
void optsReset(struct opts *input);
//readInput saves the line of text
char* readInput(void);
//parseInput checks for the different flags
int parseInput(struct opts *input, char *inputLine);
//execInput runs the certain command that is read
void execInput(struct opts *input);
//myStdIn will change the input from standard input to an input given by the user
//if a file is given when initiating the program then batchmode will trigger
int myStdIn(struct opts *input, char *filePath);
//myStdOut will change the output from standard output to output on to a file given
//by the user
int myStdOut(struct opts *input, char *filePath);
void extCmd(struct opts *input);
int pipeExec(struct opts *input);
int background(struct opts *input);

void cd(struct opts *input);
int dir(char *pathName);
void help(void);

int main(int argc, char *argv[], char * envp[]){
    //set run to true for interactive mode
    bool run = true;
    //user input struct
    struct opts input;
    //string to store the users input
    char *cmdInput;
    cmdInput = malloc(sizeof(char)*MAXBUFF);

    input.args = malloc(10*sizeof(char *));

    input.env = malloc(10*sizeof(char *));
    input.env = envp;

    int stdoutCpy = dup(1);
    int stdinCpy = dup(0);

    if(argc == 2){
        //batch mode
        int c;
        //printf("Batch mode\n");
        c = myStdIn(&input, argv[1]);
        if(c == -1){
            run = true;
        }
        else{
            run = false;
        }
    }
    else if(argc == 1){
        //just pass along
    }
    else{
        printf("Invalid input: too many arguments\n");
        exit(1);
    }
    //beginning of the while loop
    while(run){
        //reset the struct
        optsReset(&input);
        //these two dup2 reset back to stdin and stdout in the case that stdin or stdout was changed
        //in the previous iteration
        dup2(stdoutCpy, 1);
        dup2(stdinCpy, STDIN_FILENO);

        char cwd[PATH_MAX];
        getcwd(cwd, PATH_MAX);
        printf("%s myshell> ", cwd);
        //read the input and save it to cmdInput
        cmdInput = readInput();
        //parse the input
        parseInput(&input, cmdInput);

        if(input.chIn){//if chIn is true then we will change the input with myStdIn
            myStdIn(&input, input.newIn);
        }
        else if(input.chOut) {//if chOut is true then we will change the input with myStdOut
            myStdOut(&input, input.newOut);
        }
        //otherwise exec the input
        if(input.background == true){
            background(&input);
        }
        execInput(&input);
    }
    return 1;
}

int batchMode(struct opts *input, char *filePath){
    FILE *file = fopen(filePath, "r");
    if(file == NULL){
        printf("Could not open batch file\n");
        return -1;
    }
    //now get line, run the program then read next line
    return 1;
}
//we need all the struct properties reset for each iteration of the shell
void optsReset(struct opts *input){
    //reset all args
    input->background = false;
    input->chIn = false;
    input->chOut = false;
    input->truncate = false;
    input->pipe = false;
    input->newIn = "";
    input->newOut = "";

    for(int i=0; i<sizeof(input->args); i++){
        input->args[i] = "";
    }
    input->arglen = 0;
}

char* readInput(void){
    char *line = NULL;
    size_t bufferSize = 0;
    //this checks for an error while reading input
    if(getline(&line, &bufferSize, stdin) == -1){
        printf("Could not get line\n");
        exit(0);
    }
    //this removes the new line from when enter is pressed
    if(line[strlen(line) - 1] == '\n'){
        line[strlen(line) - 1] = ' ';
    }
    return line;
}

int parseInput(struct opts *input, char *inputLine){
    //in order to search for redirection commands we will parse the code using strtok
    //we are looking for <, >, >>, | and & then updating the opts struct if the token gets a match
    char *token;
    token = strtok(inputLine, " ");
    //if a strcmp is successful the next string will get tokenized. If no string is found for all
    // but &, and error will be given.
    while(token != NULL){
        if(strcmp(token, "<") == 0){
            token = strtok(NULL, " ");
            if(token != NULL){
                input->chIn = true;
                input->newIn = token;
                token = strtok(NULL, " ");
            }
            else{
                printf("No input file name given\n");
                return -1;
            }
        }
        else if(strcmp(token, ">") == 0){
            token = strtok(NULL, " ");
            if(token != NULL){
                input->chOut = true;
                input->truncate = true;
                input->newOut = token;
                token = strtok(NULL, " ");
            }
            else{
                printf("No output file given\n");
                return -1;
            }
        }
        else if(strcmp(token, ">>") == 0){
            token = strtok(NULL, " ");
            if(token != NULL){
                input->chOut = true;
                input->newOut = token;
                token = strtok(NULL, " ");
            }
            else{
                printf("No output file given\n");
                return -1;
            }
        }
        else if(strcmp(token, "|") == 0){
            token = strtok(NULL, " ");
            if(token != NULL){
                printf("arg length is now %d\n", input->arglen);
                input->pipeIndex = input->arglen;
                input->pipe = true;
            }
            else{
                printf("No string found after |\n");
                return -1;
            }
        }
        else if(strcmp(token, "&") == 0){
            input->background = true;
            token = strtok(NULL, " ");
        }
        else{
            //if the token isn't <, > or >> then save it to the args string list.
            input->args[input->arglen] = token;
            token = strtok(NULL, " ");
            input->arglen++;
        }
    }
    return 0;
}

void execInput(struct opts *input){
    //the variable command holds the command that was passed
    char *command = input->args[0];
    //simply uses exit(1) to quit the program
    if(strcmp(command, "quit") == 0){
        printf("Leaving myshell >:)\n");
        exit(1);
    }
    //clear the screen
    else if(strcmp(command, "clr") == 0){
        printf("\e[1;1H\e[2J");
    }
    //change the directory
    else if(strcmp(command, "cd") == 0){

        if(input->arglen == 2){
            cd(input);
        }
        else {
            dir(".");
        }
    }
    //list contents of the cwd
    else if(strcmp(command, "dir") == 0){
        if(input->arglen == 2){
            dir(input->args[1]);
        }
        else{
            dir(".");
        }
    }
    //list the environment variables
    else if(strcmp(command, "environ") == 0){
        int i;
        for(i=0; input->env[i] != NULL; i++){
            printf("\n%s", input->env[i]);
        }
        printf("\n");
    }
    //use input->args[1] and print it
    else if(strcmp(command, "echo") == 0){
        for(int i=1; i<sizeof(input->args); i++){
            printf("%s ", input->args[i]);
        }
        printf("\n");
    }

    //call the help() function
    else if(strcmp(command, "help") == 0){
        help();

    }
    //simple while loop until enter is pressed
    else if(strcmp(command, "pause") == 0){
        printf("Hit enter to resume, see you soon:)\n");
        while(getchar() != '\n');
    }
    //if a pipe was given then call the pipe
    else if(input->pipe == true){
        pipeExec(input);
    }
    //if the command is not built in then run extCmd()
    else{
        //printf("Command not found\n");
        extCmd(input);
    }
}
void extCmd(struct opts *input){
    int status;
    //getenv is called with the variable SHELL in order to get the path of bash
    char *path = getenv("SHELL");
    //after the path is assigned a value we pass it through setenv to set the environment
    int c = setenv("parent", path, 1);
    if(c == -1){
        printf("Failed to set environment\n");
        exit(0);
    }
    //fork the program
    pid_t pid = fork();
    if(pid == -1){
        printf("Failed to fork\n");
        exit(1);
    }
    if(pid == 0){//child
        //must set the last value in args as NULL otherwise execvp will NOT WORK
        input->args[(input->arglen) - 1] = NULL;
        //execvp takes the arg and the arg list and executes the external command
        if(execvp(input->args[0], input->args) < 0){
            printf("%s: Command not found:(\n", input->args[0]);
            exit(0);
        }
        else{
            printf("DONE\n");
        }
    }
    //wait for the child to be done
    else{
        while(wait(&status) != pid);
    }
}
//help() uses fopen to open the file help.txt which contains the entire
//manual for this program
void help(void){
    FILE *fptr;
    char c;
    fptr = fopen("help.txt", "r");

    if(fptr == NULL){
        printf("Could not open the help file\n");
        exit(1);
    }

    c = fgetc(fptr);
    while(c != EOF){
        printf("%c", c);
        c = fgetc(fptr);
    }
    printf("\n");
    fclose(fptr);
}

void cd(struct opts *input){
    //holds current working directory
    char cwd[PATH_MAX];
    //holds desired directory to change to
    char *newDir = input->args[1];
    //get the current working directory
    //getcwd(cwd, PATH_MAX);
    //chdir() to change the directory
    if(chdir(newDir) == 0){
        getcwd(cwd, PATH_MAX);
        //setenv() to change to the desired directory
        setenv("PWD", cwd, 1);
    }
    else{
        printf("%s: No such file or directory\n", input->args[1]);
    }
}
//dir for changing the directory given
int dir(char *pathName){
    DIR *d;
    struct dirent **dir;
    int numberOfDir;
    char *dname = pathName;
    d =opendir(dname);
    //open the directory and if it is null print error
    if(d == NULL){
        printf("Cannot open directory: %s\n", dname);
        return -1;
    }
    //now we will scan the directory, and use a while loop to print the contents of the
    //desired directory
    numberOfDir = scandir(dname, &dir, NULL, alphasort);
    if(numberOfDir == -1){
        printf("Cannot open directory: %s\n", dname);
        return -1;
    }

    printf("**********************************************************\n");
    printf("[[Current Directory]]\n");
    while(numberOfDir--){
        printf("%s\n", dir[numberOfDir]->d_name);
    }
    return 1;
}

int myStdIn(struct opts *input, char *filePath){
    //initialize line and bufferSize needed for getline()
    char *line = NULL;
    size_t bufferSize = MAXBUFF;
    //status is needed to call waitpid()
    int status;
    //open the desired file for reading only
    int newIn = open(filePath, O_RDONLY);
    if(newIn == -1){
        printf("Could not open file: %s\n", filePath);
        return -1;
    }
    //fork the program
    pid_t pid = fork();
    if(pid == -1){
        printf("Failed to fork\n");
        exit(1);
    }
    if(pid == 0){//child
        //dup2 to copy the file descriptor
        int c = dup2(newIn, STDIN_FILENO);
        if(c == -1){
            printf("dup2 failed\n");
            exit(1);
        }
        while(getline(&line, &bufferSize, stdin) != -1){

            for(int i = 0; i <= (strlen(line)); i++){
                if(strcmp(&line[i], "\n") == 0){
                    printf("I just found a new line\n");
                    //strcpy(&line[i], "\0");
                }
                printf("%c\n", line[i]);
            }

            printf("The line is: %s", line);
            parseInput(input, line);
            if(input->background == true){
                background(input);
            }
            else{
                execInput(input);
            }
            optsReset(input);
        }
        //close the new file handle
        close(newIn);
        //must call exit(0) to avoid parent waiting indefinitely
        exit(0);
    }
    //parent waits for the child to finish
    else{
        while(wait(&status) != pid);
    }
    return 0;

}

int myStdOut(struct opts *input, char *filePath){
    //what happens here depends on if > or >> was given for the input. If truncate was indicated
    //then we will open the file with the flag O_TRUNC. If truncate is false, we will open the file
    //with the flag O_APPEND. We then use
    printf("Change standard output called\n");
    if(input->truncate == true){
        int newStdOut = open(filePath, O_WRONLY | O_TRUNC | O_CREAT);
        if(newStdOut == -1){
            printf("Could not open file: %s\n", filePath);
            return -1;
        }
        else{
            int c = dup2(newStdOut, 1);
            if(c == -1){
                printf("dup2 failed\n");
                return -1;
            }
        }
    }
    else{
        int newStdout = open(filePath, O_WRONLY | O_APPEND | O_CREAT);
        if(newStdout == -1){
            printf("Could not open file: %s\n", filePath);
            return -1;
        }
        else{
            int c = dup2(newStdout, 1);
            if(c == -1){
                printf("dup2 failed\n");
                return -1;
            }
        }
    }
    return 0;
}

int pipeExec(struct opts *input){
    int fds[2];
    int status;
    //allocate space for both sides of the pipe
    char **args1 = malloc(10 * sizeof(char *));
    char **args2 = malloc(10 * sizeof(char *));
    //before the pipe
    /*
    if(strcpy(input->args[0], "|") == 0){
        printf("Pipe must be between two separate commands\n");
        return -1;
    }*/
    for(int i = 0; i < input->pipeIndex; i++){
        args1[i] = input->args[i];
    }
    //after the pipe
    int k = 0;
    for(int j = input->pipeIndex; j < input->arglen; j++){
        args2[k] = input->args[j];
        k++;
    }
    //call pipe
    int c = pipe(fds);
    if(c == 0){
        //fork the program
        int pid = fork();
        if(pid == 0){//child 1
            //close stdin
            close(1);
            //dup2 to copy the file descriptor
            dup2(fds[1], 1);
            //close my out
            close(fds[0]);
            //execute and then close
            execvp(args1[0], args1);
            close(fds[1]);
            exit(0);
        }
        else{
            //child 2
            int pid2 = fork();
            if(pid2 == 0){
                //close stdout
                close(0);
                //dup2 to copy the file descriptor
                dup2(fds[0], 0);
                //close my in
                close(fds[1]);
                //execute and then close
                execvp(args2[0], args2);
                close(fds[0]);
                exit(0);
            }
            //parent
            waitpid(pid,&status,0);
        }
    }
    else{
       printf("Error: pipe failed\n");
    }
    return 0;
}

int background(struct opts *input){
    //read from beginning
    pid_t pid = fork();
    if(pid == -1){
        printf("Failed to fork\n");
        return -1;
    }
    if(pid == 0){//child
        extCmd(input);
        exit(0);
    }
    //parent doesn't wait and goes back to the function
    return 0;
}