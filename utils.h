#ifndef MY_UTIL_H
#define MY_UTIL_H

#include <poll.h>
#include <stddef.h>

int sent_greeting(int fd);
int sent_msg(int fd, char * msg);
int recv_msg(int fd);

const char *inet_ntop2(void *addr, char *buf, size_t size); 
int get_listener_socket();
void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count, int *fd_size); 
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count); 
void handle_new_connection(int listener, int *fd_count, int *fd_size, struct pollfd **pfds); 
void handle_client_data(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i);
void process_connections(int listener, int *fd_count, int *fd_size, struct pollfd **pfds); 

#endif /* MY_UTIL_H */