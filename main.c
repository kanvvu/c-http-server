#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include "utils.h"


#define MYPORT "6969"
#define BACKLOG 10

void sigchild_handler(int s) {
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

int main() {
	int status;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	char ipstr[INET6_ADDRSTRLEN];
	int s;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	if ((status = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "gai error: %s \n", gai_strerror(status));
		exit(1);
	}

	struct sockaddr_in *ipv4 = NULL;

	if (servinfo == NULL) {
		printf("ERROR no addrinfo!\n");
		exit(1);
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if (p->ai_family == AF_INET) {
			ipv4 = (struct sockaddr_in *)p->ai_addr;
			break;
		}
	}

	if (ipv4 == NULL) {
		printf("ERROR no ipv4!\n");
		exit(1);
	} 

	s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	if (s == -1) {
		perror("Socket");
		exit(1);
	}

	if (bind(s, p->ai_addr, p->ai_addrlen) == -1) {
		perror("Bind");
		exit(1);
	} 

	//getting rid of Address already in use error
	int yes = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
		perror("setsockopt");
		exit(1);
	}

	freeaddrinfo(servinfo);

	struct sigaction sa;
	sa.sa_handler = sigchild_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}


	if (listen(s, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("\tListening for connection...\n");

	while(1) {
		addr_size = sizeof their_addr;
		int new_fd = accept(s,(struct sockaddr *) &their_addr, &addr_size);
		if (new_fd == -1) {
			perror("accept");
			exit(1);
		}

		if (sent_greeting(new_fd) == -1) exit(1);
		
		if (fork() == 0) {
			printf("\tAccepted new connection!\n");

			while(1) {
				char buff[64];
				int bytes_recived;

				bytes_recived = recv(new_fd, buff, sizeof buff, 0);

				if (bytes_recived == -1) {
					perror("recv");
					exit(1);
				}
				if (bytes_recived == 0) exit(1);

				printf("\tBytes recived: %d, %ld\n", bytes_recived, strlen(buff));
				buff[strcspn(buff, "\n")] = '\0';
				printf("MSG: %s\n", buff);

				sent_msg(new_fd, "Hello World!\n");
			}

			return 0;
		}
	}


	close(s);

	printf("\tServer closed\n");
	return 0;
}
