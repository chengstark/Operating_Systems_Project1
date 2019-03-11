all: 
	gcc -g simulation.c process.c -o main.o -lm -Wall -Werror
debug:
	gcc -g simulation.c process.c -o main.o -lm -Wall -Werror -D debug
