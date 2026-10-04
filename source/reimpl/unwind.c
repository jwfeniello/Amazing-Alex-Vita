#include "reimpl/unwind.h"
#include <so_util/so_util.h>

extern so_module so_mod;

/* Bionic returns the loaded module's .ARM.exidx address and the number of
 * eight-byte entries, not the section's byte size. The game's bundled libgcc
 * uses this to reach its own C++ catch handlers (including missing saves).
 * Contract: platform/bionic, libc/arch-arm/bionic/exidx_dynamic.c.
 */
uintptr_t alex_dl_unwind_find_exidx(uintptr_t pc, int *count) {
    *count = 0;
    if (pc < so_mod.text_base || pc - so_mod.text_base >= so_mod.text_size)
        return 0;
    if (!so_mod.exidx_base || !so_mod.exidx_size || so_mod.exidx_size % 8)
        return 0;
    *count = (int)(so_mod.exidx_size / 8);
    return so_mod.exidx_base;
}
