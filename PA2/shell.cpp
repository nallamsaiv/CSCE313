#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <termios.h>
#include <vector>
#include <string>
#include <ctime>
#include <pwd.h>
#include <vector>
#include <string>
#include <signal.h>
#include "Tokenizer.h"

//colors for the shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"
#define PATH_MAX 500


using namespace std;

//function used to print the prompt for the user
void print_prompt(){
    //obtaining the current system time
    time_t rawtime;
    time(&rawtime);

    //parsing the time to display on the prompt
    struct tm * timeinfo;
    char time_str[16];
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%b %d %H:%M:%S", timeinfo);

    //obtaining current working directory
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    //output the prompt
    std::cout << YELLOW << "shell$ " << GREEN << time_str << " " << BLUE << getenv("USER") << ":" << WHITE << cwd << NC << " ";
}

//function used to handle zombie processes
void sigchld_handler(int){
    waitpid(-1, 0, 0);
}

//function used to process the commands entered by the user
void process_commands(Tokenizer& tknr) {
    //declaring 2 file descriptors, one forwards and one backwards
    int backwards_fds[2];
    int forwards_fds[2];

    //looping through all the commands entered by the user
    for(size_t i = 0; i < tknr.commands.size(); i++){
        auto command = tknr.commands[i];

        //logic to deal with the cd command
        char* current_pwd = getenv("PWD");
        if(command->args[0] == "cd"){
            //first case: when the arg is "-" then we need to go back to the old directory
            if(command->args[1] == "-"){
                //used getenv and setenv to get the current directory and the old directory and swap the path variables of the two
                char* oldpwd = getenv("OLDPWD");
                if(chdir(oldpwd) == 0){
                    setenv("OLDPWD", current_pwd, 1);
                    setenv("PWD", oldpwd, 1);
                    cout << "Going back to " << BLUE << oldpwd << endl;
                }else{
                    perror("Can't go back to old directory");
                    exit(EXIT_FAILURE);
                }
            //second case: when the user provides the directory
            }else{
                if(chdir(command->args[1].c_str()) == 0){
                    //used getenv and setenv to get the current directory and make it the old directory, and to set the new current directory to the path provided by the user
                    setenv("OLDPWD", current_pwd, 1);
                    char new_pwd[PATH_MAX];
                    getcwd(new_pwd, sizeof(new_pwd));
                    setenv("PWD", new_pwd, 1);
                    cout << "Switching to " << BLUE << new_pwd << endl;
                }else{
                    perror("Can't move to new directory");
                    exit(EXIT_FAILURE);
                }
            }
            continue;
        }
        
        //initializing the backwards pipe to forwards pipe when we are not executing the first command
        if(i != 0){
            backwards_fds[0] = forwards_fds[0];
            backwards_fds[1] = forwards_fds[1];
        }

        //setting up the forwards pipe when we are not executing the last command
        if(i != tknr.commands.size() - 1){
            if(pipe(forwards_fds) == -1){
                perror("Error in creating forward pipe");
                exit(EXIT_FAILURE);
            }
        }

        //creating a new child process
        pid_t pid = fork();
        if(pid < 0){
            perror("fork error");
            exit(EXIT_FAILURE);
        }
        //child process logic for redirection of pipes
        if(pid == 0){
            //redirecting STDOUT to forwards pipe when we are not executing the last command
            if(i != tknr.commands.size() - 1){
                close(forwards_fds[0]);
                dup2(forwards_fds[1], STDOUT_FILENO);
                close(forwards_fds[1]);
            }

            //redirecting STDIN to backwards pipe when we are not executing the first command
            if(i != 0){
                close(backwards_fds[1]);
                dup2(backwards_fds[0], STDIN_FILENO);
                close(backwards_fds[0]);
            }

            //checking if the current command has input
            if(command->hasInput()){
                //redirecting STDIN to input file
                int input_fd = open(command->in_file.c_str(), O_RDONLY);
                if (input_fd < 0) {
                    perror("Unable to redirect input");
                    exit(EXIT_FAILURE);
                }
                dup2(input_fd, STDIN_FILENO);
            }
            
            //checking if the current command has ouput
            if(command->hasOutput()){
                //redirecting STDOUT to output file
                int output_fd = open(command->out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_fd < 0) {
                    perror("Unable to redirect output");
                    exit(EXIT_FAILURE);
                }
                dup2(output_fd, STDOUT_FILENO);
            }

            //executing the command after all the redirection is done
            //storing all the argument of the command in a vector
            vector<char *> args;
            for(size_t j = 0; j < command->args.size(); j++){
                args.push_back(const_cast<char *> (command->args[j].c_str()));
            }
            args.push_back(nullptr);

            //executing the command using execvp
            if(execvp(args[0], args.data()) < 0){
                perror("execvp");
                exit(2);
            }
        }else{    
            //closing the pipe ends so that the pipes do not wait for parent input/output
            if(i != 0){
                close(backwards_fds[0]);
                close(backwards_fds[1]);
            }

            //using sigchild if the command is declared as background process to take care of zombie processes
            if (command->isBackground()) {
                signal(SIGCHLD, sigchld_handler);
            }
            else {
                //waiting for process of they are not declared as background processes in order to avoid zombie processes
                int status = 0;
                waitpid(pid, &status, 0);     
                if (status > 1) {
                    perror("Child process exited with errors");    
                    exit(status);
                }
            }
        }
    }
}


int main () {
    //initializing and setting the old directory
    char* oldpwd = getenv("PWD");;
    setenv("OLDPWD", oldpwd, 1);

    
    //loop that runs infinitely to display the prompt and get the user commands through input
    for (;;) {
        //calling print prompt function, explanation on what the function does is within the function declaration and definition
        print_prompt();
                
        //getting user input
        string input;
        getline(cin, input);
        
        //break condition to leave the program when user enters "exit"
        if(input == "exit"){
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        //if empty input then contnue to the next prompt
        if(input == ""){
            continue;
        }

        //parsing the user input using tknr and if the input has a tokenizing error then move onto the next prompt
        Tokenizer tknr(input);
        if(tknr.hasError()){
            continue;
        }
       
        //function used to process the commands, explanation on what the function does is within the function declaration and definition 
        process_commands(tknr);
    }
}
