serv: serv.c list.c list.h
	gcc serv.c list.c -lpthread -Wformat -Wall -o server

cli: cli.c
	gcc cli.c -lpthread -Wformat -Wall -o client
