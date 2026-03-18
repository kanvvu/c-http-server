#include "k_string.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void k_string_init(struct k_string* str) {
	str->max_size = 64; 
	str->str = malloc(str->max_size*sizeof(char));
	if (str->str == NULL) {
			printf("k_string_init: malloc error!\n");
			exit(1);
		}
	str->str[0] = '\0';
	str->end = str->str;
}

size_t k_string_length(struct k_string* str1) {
	return str1->end - str1->str;
}

void k_string_temp(struct k_string* str1, int length1, int length2) {
	if (str1->max_size > length1 + length2)
		return;

	while(str1->max_size <= length1 + length2) str1->max_size *= 2;
	
	// printf("k_string_append: new max_size %i\n", str1->max_size);
	char * temp = realloc(str1->str, str1->max_size);
	if (temp == NULL) {
		k_string_free(str1);
		printf("k_string_append: realloc error!\n");
		exit(1);
	}
	str1->str = temp;
	str1->end = str1->str + length1;
	
}

void k_string_set(struct k_string* str1, const char * str2) {
	int length1 = str1->end - str1->str;
	int length2 = strlen(str2);

	k_string_temp(str1, length1, length2);

	str1->end = stpcpy(str1->str, str2);
	str1->str[length2] = '\0';
}

void k_string_append(struct k_string* str1, const char * str2) {
	int length1 = str1->end - str1->str;
	int length2 = strlen(str2);
	
	k_string_temp(str1, length1, length2);

	str1->end = stpcpy(str1->end, str2);
	str1->str[length1 + length2] = '\0';
}

void k_string_appendc(struct k_string* str1, const char c) {
	int length1 = str1->end - str1->str;
	int length2 = 1;
	
	k_string_temp(str1, length1, length2);


	str1->str[length1] = c;
	str1->str[length1+1] = '\0';
	str1->end = str1->str + (length1+1);
}

void k_string_appendk(struct k_string* str1, struct k_string* str2) {
	int length1 = str1->end - str1->str;
	int length2 = str2->end - str2->str; 
	
	k_string_temp(str1, length1, length2);

	// str1->end = stpcpy(str1->end, str2);
	mempcpy(str1->end, str2->str, length2);
	str1->str[length1 + length2] = '\0';
	str1->end = str1->str + (length1+length2);
}

void k_string_free(struct k_string* str) {
	free(str->str);
	str->end = NULL;
}