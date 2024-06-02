
/********************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the single handler code (should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you not need to use any features for this assignment that are supported by C++ but not by C).
*******************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()
#define MAX_SIZE 1024
#define MAX_ARGS 64
// This function terminated the use of Ctrl+C and Ctrl+Z in parent process and uses them in Child process
void sigintHandler(int signum) {
    // Handle Ctrl + C (SIGINT)
    printf("\nCtrl + C (SIGINT) received. Use 'exit' to quit the shell.\n");
}

void sigtstpHandler(int signum) {
    // Handle Ctrl + Z (SIGTSTP)
    printf("\nCtrl + Z (SIGTSTP) received. Use 'exit' to quit the shell.\n");
}

void executeCommand(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            printf("Usage: cd <directoryPath>\n");
        } else {
            if (strcmp(args[1], "..") == 0) {
                chdir("..");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("Error changing directory");
                }
            }
        }
        return;
    }

    pid_t child_pid = fork();

    if (child_pid == 0) {
        if (execvp(args[0], args) == -1) {  // call the system call execv taking argument args[]
            if (strcmp(args[0], "exit") != 0) {
                printf("Shell: Incorrect command\n");
                exit(EXIT_SUCCESS);
            }
            exit(EXIT_FAILURE);
        }
    } else if (child_pid > 0) {
        int status;
        waitpid(child_pid, &status, 0);
    } else {
        perror("Error");
        exit(EXIT_FAILURE);
    }
}
void executeCommandRedirection(char **argument, char *outputFile) {
    int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Error opening output file");
        return;
    }

    pid_t child_pid = fork();

    if (child_pid == 0) {
        // Redirect the output to the specified file.
        dup2(fd, STDOUT_FILENO);
        close(fd);

        // Execute the command.
        if (execvp(argument[0], argument) == -1) {
            perror("Error executing command");
            exit(EXIT_FAILURE);
        }
    } else if (child_pid > 0) {
        int status;
        waitpid(child_pid, &status, 0);
    } else {
        perror("Error in forking");
        exit(EXIT_FAILURE);
    }
}
void executePipedCommands(char **args1, char **args2) {
    int pipefd[2];
    pid_t child_pid1, child_pid2;

    // Create a pipe
    if (pipe(pipefd) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    // Fork the first child process (for the first command)
    child_pid1 = fork();

    if (child_pid1 == -1) {
        perror("Forking child process 1 failed");
        exit(EXIT_FAILURE);
    }

    if (child_pid1 == 0) {
        // Child process 1: Set up stdout to write to the pipe
        close(pipefd[0]); // Close the read end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]); // Close the write end of the pipe

        // Execute the first command
        if (execvp(args1[0], args1) == -1) {
            perror("Error executing command 1");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process

        // Fork the second child process (for the second command)
        child_pid2 = fork();

        if (child_pid2 == -1) {
            perror("Forking child process 2 failed");
            exit(EXIT_FAILURE);
        }

        if (child_pid2 == 0) {
            // Child process 2: Set up stdin to read from the pipe
            close(pipefd[1]); // Close the write end of the pipe
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]); // Close the read end of the pipe

            // Execute the second command
            if (execvp(args2[0], args2) == -1) {
                perror("Error executing command 2");
                exit(EXIT_FAILURE);
            }
        } else {
            // Parent process
            close(pipefd[0]); // Close both ends of the pipe in the parent
            close(pipefd[1]);

            int status1, status2;
            waitpid(child_pid1, &status1, 0);
            waitpid(child_pid2, &status2, 0);
        }
    }
}

void executeParallelCommands(char **args1, char **args2) {
    pid_t child_pid1 = fork();

    if (child_pid1 == 0) {
        executeCommand(args1);
        exit(EXIT_SUCCESS);
    } else if (child_pid1 > 0) {
        pid_t child_pid2 = fork();

        if (child_pid2 == 0) {
            executeCommand(args2);
            exit(EXIT_SUCCESS);
        } else if (child_pid2 > 0) {
            int status1, status2;
            waitpid(child_pid1, &status1, 0);
            waitpid(child_pid2, &status2, 0);
        } else {
            perror("Error process");
            exit(EXIT_FAILURE);
        }
    } else {
        perror("Error process");
        exit(EXIT_FAILURE);
    }
}
void parseInput(char *input) {
    char *args[MAX_ARGS];
    int argCount = 0;

    char *token;
    char *rest = input;
    while ((token = strsep(&rest, " \t\n")) != NULL) {
        if (*token == '\0') continue;
        args[argCount++] = token;

        if (argCount >= MAX_ARGS - 1) {
            printf("Too many arguments.\n");
            break;
        }
    }

    args[argCount] = NULL;

    for (int i = 0; i < argCount; i++) {
        if (strcmp(args[i], ">") == 0) {
            if (i == argCount - 1) {
                printf("Output file not specified.\n");
                return;
            }
            args[i] = NULL;
            char *outputFile = args[i + 1];
            executeCommandRedirection(args, outputFile);
            return;
        } else if (strcmp(args[i], "&&") == 0) {
            // Execute parallel commands when "&&" is encountered
            args[i] = NULL; // Terminate the first command
            char **args1 = args;
            char **args2 = args + i + 1;
            executeParallelCommands(args1, args2);
            return;
        } else if (strcmp(args[i], "|") == 0) {
            // Execute piped commands when "|" is encountered
            args[i] = NULL; // Terminate the first command
            char **args1 = args;
            char **args2 = args + i + 1;
            executePipedCommands(args1, args2);
            return;
        }
    }

    executeCommand(args);
}
void executeSequentialCommands(char *commands) {
    char *command;
    char *saveptr = NULL;

    // Tokenize the input string by '##' to separate commands using strsep.
    while ((command = strsep(&commands, "##")) != NULL) {
        // Remove leading and trailing whitespaces from the command.
        char *temp = command;
        while (*temp == ' ') {
            temp++;
        }

        int commandLength = strlen(temp);

        while (commandLength > 0 && (temp[commandLength - 1] == ' ' )) {
            temp[commandLength - 1] = '\0';
            commandLength--;
        }

        if (commandLength > 0) {
            // Execute the trimmed command.
            parseInput(temp); // You can reuse your existing parseInput() function.
        }
    }
}
// void executeParallelCommands()
// {
// 	// This function will run multiple commands in parallel
// }

int main()
{
	// Initial declarations
 
	while(1)	// This loop will keep your shell running until user exits.
	{
	    signal(SIGINT, sigintHandler); // Handle Ctrl + C (SIGINT)
    signal(SIGTSTP, sigtstpHandler); // Handle Ctrl + Z (SIGTSTP)

         
          char *input = NULL;
    size_t input_size = 0;
         char cwd[MAX_SIZE];
        // Print the prompt in format - currentWorkingDirectory$
           if (getcwd(cwd, sizeof(cwd)) == NULL) 
        {
            perror("Error getting current directory");
            exit(EXIT_FAILURE);
        }
        printf("%s$", cwd);

	 
			// accept input with 'getline()'
		 

        if (getline(&input, &input_size, stdin) == -1) 
        {
            perror("Error reading input");
            exit(EXIT_FAILURE);
        }	
        if(strcmp(input,"\n")==0){
            continue;
        }
         
        // Remove newline character from the input
        // input[strcspn(input, "\n")] = '\0';

		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		// parseInput(); 		
		
		if(strcmp(input, "exit\n") == 0)	// When user uses exit command.
		{
		free(input);
			printf("Exiting shell...\n");
			break;

		}
            if (strstr(input, "##") != NULL) {
            executeSequentialCommands(input);
        } else {
            parseInput(input);
        }
		
	}
	
	return 0;
}