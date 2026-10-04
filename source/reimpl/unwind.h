#ifndef ALEX_UNWIND_H
#define ALEX_UNWIND_H

#include <stdint.h>

uintptr_t alex_dl_unwind_find_exidx(uintptr_t pc, int *count);

#endif
