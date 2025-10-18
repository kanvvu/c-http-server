main: main.c client
	gcc -o main main.c

client: client.c
	gcc -o client client.c

runs:
	./main
runc:
	./client

clean: 
	rm -f *.o main client
