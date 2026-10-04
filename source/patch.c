#include "alex.h"
#include "utils/logger.h"
#include "utils/dialog.h"
#include <so_util/so_util.h>
#include <stdint.h>
#include <stdbool.h>
static void (*inform_http)(void *,bool);
static void (*cleanup_http)(void *,bool);
static int unavailable(void) { return 0; }
static void http_fail(void *operation,bool async) {
    /* HD 1.0.5: ThreadFunc passes this response field to InformListeners.
       Keep the original listener notification and thread cleanup sequence. */
    *(int *)((char *)operation+0x3c)=0;
    inform_http(operation,false);
    cleanup_http(operation,async);
}
static void checked_hook(const char *name,uint32_t first,uintptr_t replacement) {
    uintptr_t p=alex_symbol(name);
    if(*(const uint32_t *)(p&~1u)!=first)
        fatal_error("Unsupported game library at %s",name);
    hook_addr(p,replacement);
    l_info("Offline hook: %s",name);
}
void so_patch(void) {
    inform_http=(void *)alex_symbol("_ZN13HttpOperation15InformListenersEb");
    cleanup_http=(void *)alex_symbol("_ZN13HttpOperation13ThreadCleanupEb");
    checked_hook("_ZN13HttpOperation10ThreadFuncEb",0xe92d40f8,(uintptr_t)http_fail);
    checked_hook("_ZN2pf7WebView18isWebViewSupportedEv",0xe3a00001,(uintptr_t)unavailable);
}
