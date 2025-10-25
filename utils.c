
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>

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