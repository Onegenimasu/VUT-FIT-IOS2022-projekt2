CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -lpthread
FILES = proj2.c proj2.h Makefile

proj2: proj2.c
	$(CC) proj2.c $(CFLAGS) -o proj2

zip:
	zip proj2.zip $(FILES)

clean:
	rm -rf proj2 proj2.out proj2.zip debug.txt
