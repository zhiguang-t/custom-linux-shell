#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include "token.h"
#include "command.h"

#define CMD_LENGTH 256

// Declare functions
int foo(char const * epath , int eerrno);
int wildcard(Command command);
int create_sub_process(Command command, int input_no);

// Main function for shell
int main()
{
	char *token[MAX_NUM_TOKENS]; 		// Array of tokens
	int nCommands;				// Number of commands
	Command command[MAX_NUM_COMMANDS];	// Structure of command
	char line[CMD_LENGTH];			// Line input from user
	char prompt[CMD_LENGTH] = "$";		// System prompt for input
	char currDir[CMD_LENGTH];		// Current directory
	pid_t pid;				// Pid of child process
	pid_t zPid;				// Pid of zombie process
	int zStatus;				// Termination status of the zombie
	int fd = dup(0);			// Variable to store stdin
	int cStatus;				// Status of child process

	// Signal handling
	struct sigaction act;
	act.sa_flags = 0;
	act.sa_handler = SIG_IGN;

	// Ignore SIGINT, SIGQUIT and SIGSTP
	sigaction(SIGINT,&act,NULL);
	sigaction(SIGQUIT,&act,NULL);
	sigaction(SIGTSTP,&act,NULL);

	// Infinite loop for program to keep getting commands
	while(1)
	{
		int again = 1;		// Flag for signal interruption
		char *lineCheck;	// Pointer to buffer
		int more = 1;		// More zombies to claim

		// Prompt for input
		printf("%s ", prompt);

		// Prevent slow system call
		while(again)
		{
			again = 0;
			lineCheck = fgets(line, CMD_LENGTH, stdin);
			if(lineCheck == NULL)
			{
				if(errno == EINTR)
				{
					again = 1;	// Signal interruption, read again
				}
			}
		}

		// Remove '\n' from input
		if(line[strlen(line)-1] == '\n')
		{
			line[strlen(line)-1] = '\0';
		}

		// Initialise command structure
		command->first = 0;
		command->last = 0;
		command->sep = NULL;
		command->argv = NULL;
		command->nArgv = 0;
		command->stdin_file = NULL;
		command->stdout_file = NULL;

		// Split command into tokens
		tokenise(line, token);

		// Get start and end index of every command
		nCommands = separateCommands(token, command);

		if(nCommands >= 0)
		{
			// Pipe status. More than 1 means previous command is a pipe
			int status = 0;

			// Execute command
			for(int i=0; i<nCommands; i++)
			{
				// Exit program
				if(strcmp(command[i].argv[0], "exit") == 0)
				{
					printf("End of program\n");
					exit(0);
				}
				// Print current working directory
				else if(strcmp(command[i].argv[0], "pwd") == 0)
				{
					getcwd(currDir, CMD_LENGTH);
					printf("%s\n", currDir);
				}
				// Change directory
				else if(strcmp(command[i].argv[0],"cd") == 0)
				{
					// Go to home if no argument provided
					if(command[i].argv[1] == NULL)
					{
						chdir("/home");
					}
					else if(chdir(command[i].argv[1]) != 0)
					{
						printf("Invalid directory name\n");
					}
				}
				// Change prompt for shell
				else if(strcmp(command[i].argv[0],"prompt") == 0)
				{
					// Reset prompt if no argument provided
					if(command[i].argv[1] == NULL)
					{
						prompt[0] = '$';
						prompt[1] = '\0';
					}
					else
					{
						strcpy(prompt, command[i].argv[1]);
					}
				}
				// Pipe command
				else if(strcmp(command[i].sep,"|") == 0)
				{
					fd = create_sub_process(command[i], fd);

					if(fd < 0)
					{
						perror("pipe calls");
						exit(1);
					}

					status++;
				}
				else
				{

					// Create child process to execute commands
					if((pid = fork()) < 0)
					{
						perror("fork call");
					}

					if(pid == 0)
					{
						// Last command of pipe
						if(status > 0 && (strcmp(command[i].sep,"|") != 0))
						{
							close(0);
							dup(fd);
							close(fd);
							status = 0;
						}

						// Input file
						if(command[i].stdin_file != NULL)
						{
							// Open file to read input
							freopen(command[i].stdin_file, "r", stdin);
						}

						// Output file
						if(command[i].stdout_file != NULL)
						{
							// Open file to write output
							freopen(command[i].stdout_file, "w", stdout);
						}

						// Wildcard handling
						if(wildcard(command[i]) == 1)
						{
							exit(0);	// Exit process to prevent execution of wildcard command
						}

						// Execute command
						execvp(command[i].argv[0], command[i].argv);

						// If command fails to execute
						perror("execvp call");
						exit(1);
					}

					// Only wait for child process if not background process
					if(strcmp(command[i].sep, "&") != 0)
					{
						waitpid(pid, &cStatus, 0);
					}
				}
			}
		}
		// Error messages for invalid commands
		else if(nCommands == -1)
		{
			printf("Array command is too small\n");
		}
		else if(nCommands == -2)
		{
			printf("Successive command separated by more than 1 separator\n");
		}
		else if(nCommands == -3)
		{
			printf("First command is a command separator\n");
		}
		else if(nCommands == -4)
		{
			printf("Last command is followed by a command separator\n");
		}
		else
		{
			printf("Unknown error\n");
		}

		// Claim zombie processes
		while(more)
		{
			zPid = waitpid(-1, &zStatus, WNOHANG);
			if(zPid <= 0)
			{
				more = 0;
			}
		}
	}

	return 0;
}

int foo(char const * epath , int eerrno) {return 0;}

/* Function for wildcard handling
 *
 * return: 1 if wildcard exist; 0 otherwise
 */
int wildcard(Command command)
{
	/*Wildcard execution*/
	int flag=0;		// If wildcard found add the argument number into it
	pid_t gpid;		// Pid of child process that executes wildcard command
	int cStatus;		// Status of child process
	glob_t globbuf = {0}; 	// Glob for wildcard

	for(int j=1; j < command.nArgv; j++)
	{
        	// Get length of argv
		int len = strlen(command.argv[j]);
		char *temp = command.argv[j];

		// Loop through characters for wildcards
		for(int a = 0 ; a < len ; a++)
		{
			if(temp[a] == '*' || temp[a] == '?')
			{
				flag = j;    // argv index of wildcard found
			}
		}

		// If wildcard exists
		if(flag >=1)
		{
			// Use glob function to resolve wildcard
			glob(command.argv[j], GLOB_DOOFFS, foo, &globbuf);

			// Create child process to execute command for every wildcard
			for(size_t z = 0; z!= globbuf.gl_pathc; ++z)
			{
				if((gpid= fork())<0)
				{
					perror("fork call");
					exit(1);
				}

				if(gpid == 0)
				{
					// child execute each file with command
					command.argv[j] = globbuf.gl_pathv[z];
					execvp(command.argv[0],command.argv);

					perror("execvp call");
					exit(1);
				}

				//parent wait
				waitpid(gpid, &cStatus, 0);
			}

			globfree(&globbuf);    // Free glob 
		}
	}

	if(flag > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/* Function to execute pipe commands
 *
 */
int create_sub_process(Command command, int input_no)
{
	int pipfd[2];	// Pipe
	pid_t spid;	// Pid of child process
	int status;	// Status of child

	// Create pipe
	if(pipe(pipfd) < 0)
	{
		perror("pipe call");
		close(input_no);
		return -1;
	}

	// Create child
	if((spid = fork()) < 0)
	{
		perror("fork call");
		close(pipfd[0]);
		close(pipfd[1]);
		close(input_no);
		return -1;
	}

	if(spid == 0)
	{
		close(pipfd[0]);

		close(0);
		dup(input_no);
		close(input_no);

		close(1);
		dup(pipfd[1]);
		close(pipfd[1]);

		// Check for wildcard
		if(wildcard(command) == 1)
		{
			exit(0);
		}

		// Execute command
		execvp(command.argv[0], command.argv);

		perror("execvp call");
		exit(1);
	}

	close(input_no);
	close(pipfd[1]);

	// Wait for child
	waitpid(spid, &status, 0);

	return pipfd[0];
}

