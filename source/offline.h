#ifndef ALEX_OFFLINE_H
#define ALEX_OFFLINE_H
#include <sys/types.h>
int alex_network_unavailable(void);
int alex_getaddrinfo(const char *,const char *,const void *,void **);
void alex_freeaddrinfo(void *);
unsigned alex_alarm(unsigned);
unsigned alex_geteuid(void);
void *alex_getpwuid(unsigned);
#endif
