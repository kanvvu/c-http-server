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
#include <stdbool.h>
#include "global_vars.h"

// #define PORT "1313"

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
	//printf("MSG: %s\n", buff);

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

	if ((rv = getaddrinfo(NULL, GLOBALS.PORT, &hints, &ai)) != 0) {
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

void  get_mime_file(char* file_name, char *buffer, size_t buffer_size){
    FILE *file;
    char path[1024];
    path[0] = '\0';
	buffer[0] = '\0';

    char path_string[512];
    snprintf(path_string, sizeof(path_string), "file -i %s", file_name);

    file = popen(path_string, "r");
    if (file == NULL) {
        printf("Fail popen!\n");
        return;
    }

    if (fgets(path, sizeof(path), file) == NULL) {
        printf("get_mime: Fail fgets!\n");
        pclose(file);
        return;
    }

    pclose(file);

    if (strlen(path) == 0) {
        return;
    }

    char* space_pos = strchr(path, ' ');
    if (space_pos == NULL) {
        printf("get_mime: Fail first strchr!\n");
        return;
    }
    
    char* semicolon_pos = strchr(space_pos+1, ';');
    if (semicolon_pos == NULL) {
        printf("get_mime: No semicolon - taking pos of last character!\n");
        semicolon_pos = path + strlen(path);
    }
    semicolon_pos--;

	if (semicolon_pos-space_pos >= buffer_size) {
		printf("get_mime: Error excited buffer size!\n");
		return;
	}

	memcpy(buffer, space_pos+1, semicolon_pos-space_pos);
	buffer[semicolon_pos-space_pos] = '\0';

	char *new_line_pos = strchr(buffer, '\n');
	if (new_line_pos != NULL) {
		printf("get_mime: New line detected - stripping it!\n");
		*new_line_pos = '\0';
	}
	printf("get_mime: mime+type: %s\n", buffer);
}

void  get_mime_by_ext(char* file_name, char *buffer, size_t buffer_size){
	int file_name_size = strlen(file_name);

	// v1
	// int j=0;
	// bool is_dot_in_path = false;
	// for(int i=file_name_size - 1; i>=0; i--, j++) {
	// 	if (file_name[i] == '.') {
	// 		is_dot_in_path = true;
	// 		break;
	// 	} 
	// 	if (file_name[i] == '/') break;

	// 	buffer[j] = file_name[i];
	// }
	// buffer[j] = '\0';
	// int buffer_length = strlen(buffer);
	// for(int i=0; i<buffer_length/2; i++) {
	// 	char temp = buffer[buffer_length - i - 1];
	// 	buffer[buffer_length - i - 1] = buffer[i];
	// 	buffer[i] = temp;
	// }


	// v2
	bool is_dot_in_path = false;
	int i=0;
	for(i=file_name_size - 1; i>=0; i--) {
		if (file_name[i] == '.') is_dot_in_path = true; 
		if (file_name[i] == '.' || file_name[i] == '/') break; 
	}

	buffer[0] = '\0';
	if (!is_dot_in_path) return;

	char file_extension[64];
	int j=0;
	for(; j+i+1 < file_name_size; j++) 
		file_extension[j] = file_name[j+i+1];
	file_extension[j] = '\0';

	if (strcmp(file_extension, "css") == 0) strcpy(buffer, "text/css");
	else get_mime_file(file_name, buffer, buffer_size); 

	//printf("mime2: |%s|\n", buffer);

}

void write_file_to_response_body(char* path_buffer, struct http_response* response) {
	char mime_type[64];
	get_mime_by_ext(path_buffer, mime_type, 64);
	if (strlen(mime_type) == 0) k_string_set(&response->response_type, "application/octet-stream");
	else k_string_set(&response->response_type, mime_type);

	//printf("NOT A DIRECTORY: |%s|\n", path_buffer);
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
	//printf("\tsize of bin: %i\n", size);
	fclose(fptr);

}

void add_style(struct http_response* response) {
	k_string_append(&response->response_body, 
	"<head>\n"
	"   <meta charset=\"UTF-8\">\n" 
	"   <style>\n"
	"       body {\n"
	"           background-color: #f4f4f9;\n"
	"           font-family: sans-serif;\n" 
	"       }\n"
	"       .center {\n"
	"           display: flex;\n"
	"           justify-content: center;\n"
	"           padding-top: 5px;\n"
	"       }\n" 
	"       ul {\n"
	"           list-style-type: none;\n"
	"           padding: 0;\n"
	"           width: 80%;\n" 
	"           max-width: 800px;\n"
	"           border: 1px solid #ccc;\n" 
	"           border-radius: 8px;\n" 
	"           overflow: hidden;\n" 
	"           box-shadow: 0 4px 6px rgba(0,0,0,0.1);\n" 
	"       }\n"
	"       li {\n"
	"           padding: 12px 15px;\n"
	"           border-bottom: 1px solid #ddd;\n" 
	"       }\n"
	"       li:last-child {\n"
	"           border-bottom: none;\n" 
	"       }\n"
	"       li:nth-child(odd) {\n"
	"           background-color: #ffffff;\n" 
	"       }\n"
	"       li:nth-child(even) {\n"
	"           background-color: #f9f9f9;\n" 
	"       }\n"
	"       li:hover {\n"
	"           background-color: #e2e8f0;\n"
	"       }\n"
	"       a {\n"
	"           text-decoration: none;\n"
	"           color: #333;\n"
	"           display: block;\n" 
	"       }\n"
	"   </style>\n"
	"</head>\n");
}

void create_http_response(char * buf, struct http_response* response) {
	// snprintf(response->content_type, sizeof(response->content_type), "text/html");
	k_string_set(&response->response_type, "text/html");


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

		//printf("VERI GOOOD!!!! |%s|\n", path);

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

				add_style(response);

				bool is_index_html_in_dir = false;
				k_string_append(&response->response_body, "<div class=\"center\"><ul>\n");
				while((en = readdir(dr)) != NULL) {
					if (strcmp(en->d_name, ".") == 0) continue;

					if (strcmp(en->d_name, "index.html") == 0 && GLOBALS._DOWNLOAD_FLAG == 0) {
						is_index_html_in_dir = true;
						break;
					}

					char line[600];
					char new_path[300];
					if (path[strlen(path)-1] == '/') path[strlen(path) - 1] = '\0';
					snprintf(new_path, sizeof new_path, "%s/%s", path, en->d_name);

					if (en->d_type == DT_REG) {
						if (GLOBALS._DOWNLOAD_FLAG)
							snprintf(line, sizeof line, "\t<li><a href=\"%s\" download>📄 %s</a></li>\n", new_path, en->d_name);
						else
							snprintf(line, sizeof line, "\t<li><a href=\"%s\">📄 %s</a></li>\n", new_path, en->d_name);

						k_string_append(&response->response_body, line);
					} else if (en->d_type == DT_DIR) {
						snprintf(line, sizeof line, "\t<li><a href=\"%s\">📁 %s</a></li>\n", new_path, en->d_name);
						k_string_append(&response->response_body, line);
					}
				}
				k_string_append(&response->response_body, "</ul></div>");
				
				if (is_index_html_in_dir) {
					printf("There IS index.html in directory!\n");
					char buffer[1300];
					snprintf(buffer, sizeof buffer, "%s/index.html", path_buffer);
					write_file_to_response_body(buffer, response);
				} else if (GLOBALS._DOWNLOAD_FLAG) {
					printf("There is NO index.html in directory!\n");
				}

				closedir(dr);
			}
			else {
				if (errno == ENOTDIR ) {
					write_file_to_response_body(path_buffer, response);
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

	k_string_append(&response->response, "Content-Type: text/html");
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
