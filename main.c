#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>


int main() {
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	char ipstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	if ((status = getaddrinfo(NULL, "3490", &hints, &servinfo)) != 0) {
		fprintf(stderr, "gai error: %s \n", gai_strerror(status));
		exit(1);
	}

	struct sockaddr_in *ipv4 = NULL;

	if (servinfo == NULL) {
		printf("ERROR no addrinfo!\n");
		exit(1);
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if (p->ai_family == AF_INET) ipv4 = (struct sockaddr_in *)p->ai_addr;
	}

	if (ipv4 == NULL) {
		printf("ERROR no ipv4!\n");
		exit(1);
	} 

	freeaddrinfo(servinfo);
	return 0;
}
