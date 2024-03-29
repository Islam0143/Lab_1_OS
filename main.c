#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>

#define Max_Size 100

int status;
int flag = 0;   //flag to determine whether it is the first time to open a file or no

void execute_command(char* args[], int foreground) {
    pid_t pid = fork();
    if(pid == 0) { //child process
        execvp(args[0], args);
        printf("Wrong command!\n"); //if execution fails
        exit(0);
    }
    if(pid > 0 && foreground)   //if parent process and foreground process
        waitpid(pid,&status,0);
    else if(pid < 0)
        printf("failed to fork");
}

void execute_shell_bultin(char* args[]) { //case cd, echo, export
    if(strcmp(args[0], "cd") == 0) { // if the command is cd
        if(args[1] == NULL) //case cd
            chdir("/home");
        else if(strcmp(args[1], "~") == 0)  //case cd ~
            chdir("..");
        else
            chdir(args[1]); //remaining cases
    }
    else if(strcmp(args[0], "echo") == 0) { //if the command is echo
        args[1]++;
        char* LastChar = args[1]+strlen(args[1])-1;
        *LastChar = '\0';   //eliminate the double quotes at the start and the end of the string
        printf("%s\n",args[1]);
    }
    else {    //if the command is export
        char* variable = strtok(args[1], "="); //split the string and store the variable name
        char* val = strtok(NULL, "=");  //store the variable value in val
        if(*val == '"') { //if double quotes exists eliminate them
            val++;
            val[strlen(val)-1] = '\0';
        }
        setenv(variable,val,1); //set the name and the value of the environment variable and enable replacing
    }
}

//function to replace the environment variables if exists by their value
void evaluate_expression(char command[]) {
    char var[20] = "";
    char result[100] = "";
    char* val;
    for (int i = 0; i < strlen(command); i++) {
        if (command[i] == '$') {
            // if it is '$' sign then get the variable name that follows it
            int j = 0;
            while (isalnum(command[i+1+j])) { // check if the next character is alphanumeric
                var[j] = command[i+1+j];
                j++;
            }
            var[j] = '\0'; // terminate the string
            char* variablePointer = &var;
            val = getenv(variablePointer);

            // concatenate the variable value to the result string
            strcat(result, val);

            i += j; // skip the remaining characters
        } else {
            // if it's not a '$' sign then copy the character to the result string
            char temp[2];
            temp[0] = command[i];
            temp[1] = '\0'; // add a null terminator to make it a valid string
            strcat(result, temp); // concatenate the character to the result string
        }
    }
    strcpy(command,result);
}

void shell() {
    char command[Max_Size];
    while(1) {
        fgets(command, Max_Size, stdin); //read line
        command[strcspn(command, "\n")] = 0;    //remove newline character at the end
        int foreground = 1; //flag to determine foreground or not
        if(strcmp(command, "exit") == 0) //if exit command
            exit(0);
        if(command[strlen(command) - 1] == '&') //if not foreground command
            foreground = 0;
        evaluate_expression(command);
        char* pointer = strtok(command, " "); //split the command
        char* args[Max_Size];   //will hold the command name and its parameters
        args[0] = pointer;  //command name in index 0
        if(strcmp(args[0],"cd") == 0 || strcmp(args[0],"echo") == 0 || strcmp(args[0],"export") == 0) {
            args[1] = strtok(NULL, ""); //parameter of the command in index 1
            execute_shell_bultin(args);
            continue;
        }
        int i;
        for(i = 1; pointer != NULL; i++) {  //if still there is a command parameter
            pointer = strtok(NULL, " ");
            args[i] = pointer; //command parameter is in index i
        }
            execute_command(args, foreground);
    }
}
//signal handler function
void on_child_exit_handler() {
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) { //if any child process terminated
        printf("process %d terminated with return code: %d\n", pid, status);
        FILE* myFile;
        // log the termination of the child in a file
        if(flag) myFile = fopen("myFile.txt", "a"); //if not first time to open the file
        else {myFile = fopen("myFile.txt", "w"); flag = 1;} //if it is the first time to open the file
        fprintf(myFile, "Child process %d was terminated\n",pid);
        fclose(myFile);
    }
}

int main() {
    signal(SIGCHLD, on_child_exit_handler); //initialize signal handler
    shell(); // initialize the shell
    return 0;
}