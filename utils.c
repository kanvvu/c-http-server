#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9034"

int sent_msg(int fd, char * msg) {
	int len, bytes_sent;
	len = strlen(msg);

	bytes_sent = send(fd, msg, len, 0);
	if (bytes_sent == -1) {
		perror("send");
		return -1;
	}

	printf("\tBytes to sent: %d, Bytes sent: %d\n", len, bytes_sent);
	return 0;
}

int recv_msg(int fd) {
	char buff[64];
	int bytes_recived;

	bytes_recived = recv(fd, buff, sizeof buff, 0);
	if (bytes_recived == -1) {
		perror("recv");
		return -1;
	}
	if (bytes_recived == 0) {
		printf("Server is closed!\n");
		return -1;
	}

	printf("\tBytes recived %d\n", bytes_recived);
	buff[strcspn(buff, "\n")] = '\0';
	printf("MSG: %s\n", buff);

	return 0;
}

int sent_greeting(int fd) {
	sent_msg(fd, "Hello! I hope we will have meaningful conversation!");
}

const char *inet_ntop2(void *addr, char *buf, size_t size) {
	struct sockaddr_storage *sas = addr;
	struct sockaddr_in *sa4;
	struct sockaddr_in6 *sa6;
	void *src;

	switch (sas->ss_family) {
		case AF_INET:
			sa4 = addr;
			src = &(sa4->sin_addr); 
			break;
		case AF_INET6:
			sa6 = addr;
			src = &(sa6->sin6_addr); 
			break;
		default:
			return NULL;
	}

	return inet_ntop(sas->ss_family, src, buf, size);
}

int get_listener_socket() {
	int listener;
	int yes=1;
	int rv;

	struct addrinfo hints, *ai, *p;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "pollsever: %s\n", gai_strerror(rv));
		exit(1);
	}

	for(p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			continue;
		}

		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	if (p==NULL) {
		return -1;
	}

	freeaddrinfo(ai);

	if (listen(listener, 10) == -1) {
		return -1;
	}

	return listener;
}

void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count, int *fd_size) {
	if (*fd_count == *fd_size) {
		*fd_size *= 2;
		*pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN;
	(*pfds)[*fd_count].revents = 0;

	(*fd_count)++;
}

void del_from_pfds(struct pollfd pfds[], int i, int *fd_count) {
	pfds[i] = pfds[*fd_count-1];

	(*fd_count)--;
}

void handle_new_connection(int listener, int *fd_count, int *fd_size, struct pollfd **pfds) {
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;
	int newfd;
	char remoteIP[INET6_ADDRSTRLEN];

	addrlen = sizeof remoteaddr;
	newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

	if (newfd == -1) {
		perror("accept");
	} else {
		add_to_pfds(pfds, newfd, fd_count, fd_size);

		printf("pollserver: new connection from %s on socket %d\n", inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP), newfd);


		//lets greet our client!
		char msg[128];
		snprintf(msg, sizeof msg, "\nWelcome traveler! I hope you will have fun!\r\nFrom now you will be known as user%d! What interesting you have to say?\r\n\n", newfd);
		if (send(newfd, msg, strlen(msg), 0) == -1) {
			perror("send");
		}

	}
}


void handle_client_data(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i) {
	//even though fresh in each call still can have junk from other calls
	char buf[256];

	//the one who is recived data is the one who sent it
	int sender_fd = pfds[*pfd_i].fd;

	int nbytes = recv(sender_fd, buf, sizeof buf, 0);
	buf[nbytes] = '\0';


	if (nbytes <=0 ) {
		if (nbytes == 0) {
			printf("pollserver: socket %d hung up\n", sender_fd);
		} else {
			perror("recv");
		}

		close(pfds[*pfd_i].fd);
		del_from_pfds(pfds, *pfd_i, fd_count);
		(*pfd_i)--;
	} else {
		printf("pollsever: recv from fd %d: %.*s", sender_fd, nbytes, buf);

		for(int j=0; j<*fd_count; j++) {
			int dest_fd = pfds[j].fd;

			if (dest_fd != listener && dest_fd != sender_fd) {
				char* nickname = "user";
				char temp[300];

				snprintf(temp, sizeof temp, "%s%d: %s", nickname, sender_fd, buf);

				if (send(dest_fd, temp, strlen(temp), 0) == -1) {
					perror("send");
				}

			}
		}
	}
}

void process_connections(int listener, int *fd_count, int *fd_size, struct pollfd **pfds) {
	for(int i=0; i<*fd_count; i++) {
		if ((*pfds)[i].revents & (POLLIN | POLLHUP)) {

			if ((*pfds)[i].fd == listener) {
				handle_new_connection(listener, fd_count, fd_size, pfds);
			} else {
				handle_client_data(listener, fd_count, *pfds, &i);
			}
		}
	}
}

void handle_new_connection2(int listener, int *fd_count, int *fd_size, struct pollfd **pfds) {
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;
	int newfd;
	char remoteIP[INET6_ADDRSTRLEN];

	addrlen = sizeof remoteaddr;
	newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

	if (newfd == -1) {
		perror("accept");
	} else {
		add_to_pfds(pfds, newfd, fd_count, fd_size);

		printf("pollserver: new connection from %s on socket %d\n", inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP), newfd);
	}
}

void handle_client_data2(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i) {
	//even though fresh in each call still can have junk from other calls
	char buf[1024]; //64, 128, 256, 512, 1024

	//the one who is recived data is the one who sent it
	int sender_fd = pfds[*pfd_i].fd;

	int nbytes = recv(sender_fd, buf, sizeof buf, 0);
	buf[nbytes] = '\0';


	if (nbytes <=0 ) {
		if (nbytes == 0) {
			printf("pollserver: socket %d hung up\n", sender_fd);
		} else {
			perror("recv");
		}

		close(pfds[*pfd_i].fd);
		del_from_pfds(pfds, *pfd_i, fd_count);
		(*pfd_i)--;
	} else {
		printf("pollsever: recv from fd %d: %.*s", sender_fd, nbytes, buf);
// 		char * response = "HTTP/1.1 200 OK\n\
// Server: MyCustomServer/1.0 (Linux)\n\
// Content-Type: text/html; charset=UTF-8\n\
// Content-Length: 350\n\
// Connection: keep-alive\n\
// <!DOCTYPE html>\n\
// <html lang=\"en\">\n\
// <head>\n\
// 	<meta charset=\"UTF-8\">\n\
// 	<title>Welcome Home</title>\n\
// </head>\n\
// <body>\n\
// 	<h1>Success!</h1>\n\
// 	<p>This is the main page served from localhost:6969.</p>\n\
// 	<p>Your browser is running: Chrome/142 on Windows.</p>\n\
// </body>\n\
// </html>";

		char response[1024];
		char *response_body = "<html><body>Hello, world!</body></html>";
		snprintf(response, sizeof response,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %zu\r\n"
		"Connection: close\r\n"
		"\r\n"
		"%s", strlen(response_body), response_body);
		if (send(sender_fd, response, strlen(response), 0) == -1) {
			perror("send");
		}
		printf("response strlen: %ld\n", strlen(response));

	}
}

void process_connections2(int listener, int *fd_count, int *fd_size, struct pollfd **pfds) {
	for(int i=0; i<*fd_count; i++) {
		if ((*pfds)[i].revents & (POLLIN | POLLHUP)) {

			if ((*pfds)[i].fd == listener) {
				handle_new_connection2(listener, fd_count, fd_size, pfds);
			} else {
				handle_client_data2(listener, fd_count, *pfds, &i);
			}
		}
	}
}
