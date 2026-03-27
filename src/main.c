#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global_vars.h"


struct _GLOBALS GLOBALS = {0, 1313};

int main(int argc, char **argv) {

	if (argc > 1) {
		if (strcmp("-d", argv[1]) == 0) {
			GLOBALS._DOWNLOAD_FLAG = 1;
		}
	}

	
	int listener;
	int fd_size = 5;
	int fd_count = 0;
	struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

	listener = get_listener_socket();

	if (listener == -1) {
		fprintf(stderr, "error getting listening socket\n");
		exit(1);
	}

	pfds[0].fd = listener;
	pfds[0].events = POLLIN;

	fd_count = 1;

	puts("pollsever: waiting for connections... ");

	for(;;) {
		int poll_count = poll(pfds, fd_count, -1);
		printf("polled: %d\n", poll_count);

		if (poll_count == -1) {
			perror("poll");
			exit(1);
		}

		process_connections(listener, &fd_count, &fd_size, &pfds);
	}

	free(pfds);
	return 0;
}