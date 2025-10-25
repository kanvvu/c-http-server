#ifndef MY_UTIL_H
#define MY_UTIL_H

int sent_greeting(int fd);
int sent_msg(int fd, char * msg);
int recv_msg(int fd);

#endif /* MY_UTIL_H */