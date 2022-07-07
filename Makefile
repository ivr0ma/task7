all: client server
	
client:
	gcc client.c -o cli.out
server:
	gcc server.c -o ser.out
cli:
	./cli.out 2501
ser:
	./ser.out 2501
vcli:
	valgrind ./cli.out 2700
vser:
	valgrind ./ser.out 2700
