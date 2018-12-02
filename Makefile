all: first

first: first.c
	gcc -Wall -Werror -g -fsanitize=address -lm first.c -o first

clean:
	rm -rf first
