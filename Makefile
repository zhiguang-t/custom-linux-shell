# makefile for test_command

# file to test command.h
myshell: myshell.o token.o command.o
	gcc myshell.o token.o command.o -o myshell

myshell.o: myshell.c
	gcc -c myshell.c

# object file for command module
command.o: command.h command.c
	gcc -c command.c

# object file for token module
token.o: token.c token.h
	gcc -c token.c

clean:
	rm -rf *.o
