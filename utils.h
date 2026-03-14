#ifndef MY_UTIL_H
#define MY_UTIL_H

#include <poll.h>
#include <stddef.h>

struct k_string {
    char *str;
    char *end;
    size_t max_size;
};

void k_string_init(struct k_string* str);
void k_string_set(struct k_string* str1, const char * str2);
void k_string_append(struct k_string* str1, const char * str2);
void k_string_appendc(struct k_string* str1, const char c);
void k_string_appendk(struct k_string* str1, struct k_string* str2);
size_t k_string_length(struct k_string* str1);
void k_string_free(struct k_string* str);


// struct http_response {
//     char response[1000];
//     char response_code[128];
//     char response_body[500];
//     char content_type[100];
// };
struct http_response {
    struct k_string response;
    struct k_string response_code;
    struct k_string response_body;
    struct k_string response_type;
};

void http_response_init(struct http_response* response);
void http_response_free(struct http_response* response);

void create_http_response(char * buf, struct http_response* response); 

void make_response(struct http_response* response);
void make_internal_server_error(struct http_response* response);



int sent_greeting(int fd);
int sent_msg(int fd, char * msg);
int recv_msg(int fd);

const char *inet_ntop2(void *addr, char *buf, size_t size); 
int get_listener_socket();
void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count, int *fd_size); 
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count); 

//book example
void handle_new_connection(int listener, int *fd_count, int *fd_size, struct pollfd **pfds); 
void handle_client_data(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i);
void process_connections(int listener, int *fd_count, int *fd_size, struct pollfd **pfds); 

//http
void handle_new_connection2(int listener, int *fd_count, int *fd_size, struct pollfd **pfds); 
void handle_client_data2(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i);
void process_connections2(int listener, int *fd_count, int *fd_size, struct pollfd **pfds); 

#endif /* MY_UTIL_H */