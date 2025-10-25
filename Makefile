all: main client

main: main.c utils.o
	gcc -o bin/main main.c utils.o

client: client.c utils.o
	gcc -o bin/client client.c utils.o

fork_test: fork_test.c
	gcc -o ft fork_test.c

%.o: %.c
	gcc -c $< -o $@

run:
	./bin/main
runc:
	./bin/client

runf:
	./ft

clean: 
	rm -f *.o main client ft bin/*
