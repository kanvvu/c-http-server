#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global_vars.h"


struct _GLOBALS GLOBALS = {0, "1313"};

void process_terminal_arguments(int argc, char **argv) {
	if (argc > 1) {
		for(int i=1; i<argc; i++) {
			// download flag
			if (strcmp("-d", argv[i]) == 0) {
				printf("DOWNLOAD flag provided!\n");
				GLOBALS._DOWNLOAD_FLAG = 1;
			// port flag
			} else if(strcmp("-p", argv[i]) == 0) {
				if (argc <= i + 1) {
					printf("You need to specify port number!\n");
					exit(1);
				}
				if (strlen(argv[i+1]) > 5) {
					printf("INVALID port number!\n");
					exit(1);
				}
                for(int j=0; j<strlen(argv[i+1]); j++) {
                    if (argv[i+1][j] < '0' || argv[i+1][j] > '9') {
                        printf("INVALID port number! %s\n", argv[i+1]);
                        exit(1);
                    }
                    GLOBALS.PORT[j] = argv[i+1][j];
                }
                GLOBALS.PORT[strlen(argv[i+1])] = '\0';
                i++;
			// unknown flag 
			} else {
				printf("UNKOWN argument:'%s'\n", argv[i]);
				exit(1);
			}
		}
	}
}

int main(int argc, char **argv) {

	process_terminal_arguments(argc, argv);
	
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