all:
	gcc -g *.c -Wall -Wextra -o pizzeria `mysql_config --cflags --include --libs`
clean:
	-rm client