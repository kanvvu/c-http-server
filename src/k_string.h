#ifndef K_STRING_H
#define K_STRING_H

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

#endif /* MY_UTIL_H */