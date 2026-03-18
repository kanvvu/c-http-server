#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <errno.h>

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
	}
}

char*  get_mime(char* file_name){
    FILE *file;
    char path[1024];
    path[0] = '\0';

    char path_string[512];
    snprintf(path_string, sizeof(path_string), "file -i %s", file_name);

    file = popen(path_string, "r");
    if (file == NULL) {
        printf("Fail popen!\n");
        return NULL;
    }

    if (fgets(path, sizeof(path), file) == NULL) {
        printf("get_mime: Fail fgets!\n");
        pclose(file);
        return NULL;
    }

    pclose(file);

    if (strlen(path) == 0) {
        return NULL;
    }

    char* space_pos = strchr(path, ' ');
    if (space_pos == NULL) {
        printf("get_mime: Fail first strchr!\n");
        return NULL;
    }
    
    char* semicolon_pos = strchr(space_pos+1, ';');
    if (semicolon_pos == NULL) {
        printf("get_mime: No semicolon - taking pos of last character!\n");
        semicolon_pos = path + strlen(path);
    }
    semicolon_pos--;

    char *mime_type = strndup(space_pos+1, semicolon_pos-space_pos);

	char *new_line_pos = strchr(mime_type, '\n');
	if (new_line_pos != NULL) {
		printf("get_mime: New line detected - stripping it!\n");
		*new_line_pos = '\0';
	}
	printf("get_mime: mime+type: %s\n", mime_type);
	return mime_type;
}

void create_http_response(char * buf, struct http_response* response) {
	// snprintf(response->content_type, sizeof(response->content_type), "text/html");
	k_string_set(&response->response_type, "text/html");

	int is_file = 0;

	/*
	1. Get first line of request
	*/
	char *first_space = strchr(buf, ' ');
	if (first_space == NULL) {
		make_internal_server_error(response);
		return;
	}
	char *second_space = strchr(first_space+1, ' ');
	if (second_space == NULL) {
		make_internal_server_error(response);
		return;
	}
	/*
	2. Check first argument, if GET proceed
	*/
	if (strncmp(buf, "GET", 3) == 0) {
		char *path = strndup(first_space+1, second_space-first_space);
		path[second_space-first_space-1] = '\0';

		printf("VERI GOOOD!!!! |%s|\n", path);

		char path_buffer[1024];
		if (strcmp(path, "/") == 0) snprintf(path_buffer, sizeof(path_buffer), "."); 
		else {
			snprintf(path_buffer, sizeof(path_buffer), ".%s", path);
		}


		/*
		3. Check if path exists
		*/
		if (access(path_buffer, R_OK) == 0) {
			/*
			4. If exists print contnents
			*/
			printf("path %s exists!\n", path_buffer);
			// snprintf(response->response_code, sizeof response->response_code, "200 OK");
			k_string_set(&response->response_code, "200 OK");
			
			

			DIR *dr;
			struct dirent *en;
			dr = opendir(path_buffer);

			// do stpcpy when size src is less than dest
			// we need size of dest, 
			if (dr) {
				k_string_append(&response->response_body, "<ul>");
				while((en = readdir(dr)) != NULL) {
					if (strcmp(en->d_name, ".") == 0) continue;

					char line[600];
					char new_path[300];
					if (path[strlen(path)-1] == '/') path[strlen(path) - 1] = '\0';
					// if (strcmp(path, "/\0") == 0) snprintf(new_path, sizeof new_path, "/%s", en->d_name);
					// else snprintf(new_path, sizeof new_path, "%s/%s", path, en->d_name);
					snprintf(new_path, sizeof new_path, "%s/%s", path, en->d_name);

					if (en->d_type == DT_REG) {
						snprintf(line, sizeof line, "<li><a href=\"%s\" download>%s</a></li>", new_path, en->d_name);
						k_string_append(&response->response_body, line);
					} else if (en->d_type == DT_DIR) {
						snprintf(line, sizeof line, "<li><a href=\"%s\">-> %s</a></li>", new_path, en->d_name);
						k_string_append(&response->response_body, line);
					}
				}
				k_string_append(&response->response_body, "</ul>");
				closedir(dr);
			}
			else {
				if (errno == ENOTDIR ) {
					is_file = 1; 
					char* mime_type = get_mime(path_buffer);
					if (mime_type == NULL) k_string_set(&response->response_type, "application/octet-stream");
					else k_string_set(&response->response_type, mime_type);

					printf("NOT A DIRECTORY: |%s|\n", path_buffer);
					FILE *fptr = fopen(path_buffer, "r");
					if (fptr == NULL) {
						perror("");
						exit(1);
					}
					char ch[1];
					k_string_set(&response->response_body, "");
					int size = 0;
					while (fread(ch, sizeof(char), 1, fptr) == 1) {
						k_string_appendc(&response->response_body, ch[0]);
						size++;
					}
					printf("\tsize of bin: %i\n", size);

					fclose(fptr);
					
					free(mime_type);

				}
			}
		} else {
			printf("path %s does not exist!\n", path_buffer);
			k_string_set(&response->response_code, "404 Not Found");
		}
		free(path);
		
	} 
	make_response(response);
	
}

void make_response(struct http_response* response) {
	k_string_set(&response->response, "HTTP/1.1 ");
	k_string_append(&response->response, response->response_code.str);
	k_string_append(&response->response, "\r\n");

	k_string_append(&response->response, "Content-Type: ");
	k_string_append(&response->response, response->response_type.str);
	k_string_append(&response->response, "\r\n");

	k_string_append(&response->response, "Content-Length: ");
	char content_length[64];
	snprintf(content_length, sizeof (content_length), "%zu", k_string_length(&response->response_body));
	k_string_append(&response->response, content_length);
	k_string_append(&response->response, "\r\n");

	k_string_append(&response->response, "Connection: close\r\n");
	k_string_append(&response->response, "\r\n");

	k_string_appendk(&response->response, &response->response_body);
}

void make_internal_server_error(struct http_response* response) {
	k_string_set(&response->response, "HTTP/1.1 500 Internal Server Error");
	k_string_append(&response->response, "\r\n");

	k_string_append(&response->response, "Content-Type: ");
	k_string_append(&response->response, "text/html");
	k_string_append(&response->response, "\r\n");

	k_string_append(&response->response, "Content-Length: 0");
	k_string_append(&response->response, "\r\n");

	k_string_append(&response->response, "Connection: close\r\n");
	k_string_append(&response->response, "\r\n");
} 

void handle_client_data(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i) {
	//even though fresh in each call still can have junk from other calls
	char buf[1024]; //64, 128, 256, 512, 1024

	//the one who is recived data is the one who sent it
	int sender_fd = pfds[*pfd_i].fd;

	int nbytes = recv(sender_fd, buf, sizeof buf, 0);
	buf[nbytes] = '\0';


	if (nbytes <=0 ) {
		if (nbytes == 0) 
			printf("pollserver: socket %d hung up\n", sender_fd);
		else 
			perror("recv");
		

		close(pfds[*pfd_i].fd);
		del_from_pfds(pfds, *pfd_i, fd_count);
		(*pfd_i)--;
	} else {
		printf("pollsever: recv from fd %d: %.*s", sender_fd, nbytes, buf);
		
		struct http_response response;
		http_response_init(&response);
		create_http_response(buf, &response);
		if (send(sender_fd, response.response.str, k_string_length(&response.response), 0) == -1) {
			perror("send");
		}

		printf("response strlen: %ld\n", k_string_length(&response.response));
		http_response_free(&response);
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

void http_response_init(struct http_response* response) {
	k_string_init(&response->response);
	k_string_init(&response->response_body);
	k_string_init(&response->response_code);
	k_string_init(&response->response_type);
}
void http_response_free(struct http_response* response) {
	k_string_free(&response->response);
	k_string_free(&response->response_body);
	k_string_free(&response->response_code);
	k_string_free(&response->response_type);
}
