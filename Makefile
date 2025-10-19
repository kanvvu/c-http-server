all: main client

main: main.c 
	gcc -o main main.c

client: client.c
	gcc -o client client.c

fork_test: fork_test.c
	gcc -o ft fork_test.c

run:
	./main
runc:
	./client

runf:
	./ft

clean: 
	rm -f *.o main client
