#include "offline.h"
#include <errno.h>
#include <stddef.h>
int alex_network_unavailable(void) { errno=ENETUNREACH; return -1; }
int alex_getaddrinfo(const char *host,const char *service,const void *hints,void **result) {
    (void)host;(void)service;(void)hints;
    if(result) *result=NULL;
    errno=ENETUNREACH;
    return 2; /* Android/bionic EAI_AGAIN. No DNS request is attempted. */
}
void alex_freeaddrinfo(void *p) { (void)p; }
unsigned alex_alarm(unsigned seconds) { (void)seconds; return 0; }
unsigned alex_geteuid(void) { return 0; }
void *alex_getpwuid(unsigned uid) { (void)uid; errno=ENOENT; return NULL; }
