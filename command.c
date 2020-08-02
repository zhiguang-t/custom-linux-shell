#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "command.h"

// Return 1 if the token is a command
// return 0 otherwise
int separator(char *token)
{
	int i=0;
	char *commandSeparators[] = {pipeSep, conSep, seqSep, NULL};

	while(commandSeparators[i] != NULL)
	{
		if(strcmp(commandSeparators[i], token) == 0)
        {
            return 1;
        }
		++i;
	}
	return 0;
}

// Fill one command structure with details
void fillCommandStructure(Command *cp, int first, int last, char *sep)
{
	cp->first = first;
	cp->last = last - 1;
	cp->sep = sep;
    if(strcmp(sep,"&") == 0)
    {
        cp->nArgv = 1;
    }
}

// Process standard in/out redirections in a command
void searchRedirection(char *token[], Command *cp)
{
	int i;
	for(i=cp->first; i<=cp->last; ++i)
	{
		if(strcmp(token[i], "<") == 0)	// Standard input redirection
		{
			cp->stdin_file = token[i+1];
			++i;
		}
		else if(strcmp(token[i], ">") == 0)	// Standard ouput redirection
		{
			cp->stdout_file = token[i+1];
			++i;
		}
	}
}

// Build command line argument vector for execvp function
void buildCommandArgumentArray(char *token[], Command *cp)
{
	int n = (cp->last - cp->first + 1)	// The number of tokens in the command
		+ 1;				// The element in argv must be NULL

	// Re-allocate memory for argument vector
	cp->argv = (char **) realloc(cp->argv, sizeof(char *) * n);
	if(cp->argv == NULL)
	{
		perror("realloc\n");
		exit(1);
	}

	// Build the argument vector
	int i, k=0;
	for (i=cp->first; i<=cp->last; ++i)
	{
		if(strcmp(token[i], ">") == 0 || strcmp(token[i], "<") == 0)
			++i;	// Skip off the std in/out redirection
		else
		{
			cp->argv[k] = token[i];
			++k;
		}
	}
	cp->argv[k] = NULL;
	cp->nArgv = k;
}

int separateCommands(char *token[], Command command[])
{
	int i;
	int nTokens;

	// Find out the number of tokens
	i = 0;
	while(token[i] != NULL) ++i;
	nTokens = i;

	// If empty command line
	if(nTokens == 0)
		return 0;

	// Check the first token
	if(separator(token[0]))
		return -3;

	// Check last token, add ";" if necessary
	if(!separator(token[nTokens-1]))
	{
		token[nTokens] = seqSep;
		++nTokens;
	}

	int first=0;	// Points to the first tokens of a command
	int last;	// Points to the last tokens of a command
	char*sep;	// Command separator at the end of a command
	int c = 0;	// Command index
	for(i=0; i<nTokens; i++)
	{
		last = i;
		if(separator(token[i]))
		{
			sep = token[i];
			if(first == last)	// two consecutive separators
				return -2;
			fillCommandStructure(&(command[c]), first, last, sep);
			++c;
			first = i+1;
		}
	}

	// Check the last token if the last command
	if(strcmp(token[last], pipeSep) == 0)	// last token is pipe separator
		return -4;

	// Calculate the number of commands
	int nCommands = c;

	// Handle standard in/out redirection and build command line argument vector
	for(i=0; i<nCommands; ++i)
	{
		searchRedirection(token, &(command[i]));
		buildCommandArgumentArray(token, &(command[i]));
	}

	return nCommands;
}

