#include <string.h>
#include "token.h"

int tokenise(char line[], char *token[])
{
	char *tk;	// Variable to store token
	int i=0;	// Number of tokens

	// Using strtok function to split input into tokens
	tk = strtok(line, tokenSeparators);
	token[i] = tk;

	while(tk != NULL)
	{
		++i;

		// Break out of loop if number of tokens
		// exceeds the number set for MAX_NUM_TOKENS
		if(i >= MAX_NUM_TOKENS)
		{
			i = -1;
			break;
		}

		// Getting tokens and storing into token array
		tk = strtok(NULL, tokenSeparators);
		token[i] = tk;
	}

	return i;  // Number of tokens
}
