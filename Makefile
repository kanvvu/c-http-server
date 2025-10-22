all: main client

main: main.c utils.o
	gcc -o main main.c utils.o

client: client.c utils.o
	gcc -o client client.c utils.o

fork_test: fork_test.c
	gcc -o ft fork_test.c

%.o: %.c
	gcc -c $< -o $@

run:
	./main
runc:
	./client

runf:
	./ft

clean: 
	rm -f *.o main client
