chttpserver: src/main.c obj/utils.o obj/k_string.o
	gcc -o bin/chttpserver src/main.c obj/utils.o obj/k_string.o

obj/%.o: src/%.c
	gcc -c $< -o $@

run:
	./bin/chttpserver

clean: 
	rm -f *.o bin/* obj/*
