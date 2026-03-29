#ifndef _GLOBAL_VARS_H 
#define _GLOBAL_VARS_H


struct _GLOBALS {
	int _DOWNLOAD_FLAG;
	char PORT[6]; // stackoverflow says that max port number is 65535 (usigned int) so 5 digits + '\0' 
};

extern struct _GLOBALS GLOBALS;

#endif