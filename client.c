#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>


#define MYPORT "6969"

int main() {
	int status;
	struct sockaddr_storage server_addr;
	socklen_t addr_size;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	char ipstr[INET6_ADDRSTRLEN];
	int s;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
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


    if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
        perror("connect");
        exit(1);
    }
    printf("\tConnected to server!\n");

	freeaddrinfo(servinfo);

    char buff[64];
    int bytes_recived;
    
    bytes_recived = recv(s, buff, sizeof buff, 0);
    if (bytes_recived == -1) {
        perror("recv");
        exit(1);
    }
    if (bytes_recived == 0) {
        printf("Server is closed!\n");
        exit(1);
    }

    printf("\tBytes recived %d\n", bytes_recived);
    buff[strcspn(buff, "\n")] = '\0';
    printf("MSG: %s\n", buff);

	close(s);

	printf("\tClient closed\n");
	return 0;
}
