all: main #client

main: src/main.c obj/utils.o obj/k_string.o
	gcc -o bin/main src/main.c obj/utils.o obj/k_string.o

# client: src/client.c obj/utils.o obj/k_string.o
# 	gcc -o bin/client src/client.c obj/utils.o obj/k_string.o

# $<: This is a Makefile automatic variable that represents the first prerequisite (dependency) of the rule.
# $@: Represents the target of the rule.
obj/%.o: src/%.c
	gcc -c $< -o $@

run:
	./bin/main
runc:
	./bin/client

runt: src/temp.c src/utils.c src/k_string.c
	gcc -o bin/temp src/temp.c src/utils.c src/k_string.c && ./bin/temp

clean: 
	rm -f *.o bin/* obj/*
