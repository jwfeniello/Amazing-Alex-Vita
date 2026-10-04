#include "alex.h"
#include "utils/init.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/dialog.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <so_util/so_util.h>
int _newlib_heap_size_user=256*1024*1024;
int sceLibcHeapSize=4*1024*1024;
/* GameStateUtils::CreateNew alone reserves 0x54dc0 bytes, exceeding the
 * default 256 KiB main stack. vita-elf-create puts this into process params. */
const SceSize sceUserMainThreadStackSize=2*1024*1024;
so_module so_mod;
int main(void) {
    alex_log_init();
    l_info("Amazing Alex HD Vita build 0.4; offline development build");
    SceKernelThreadInfo thread_info={.size=sizeof(thread_info)};
    if(sceKernelGetThreadInfo(sceKernelGetThreadId(),&thread_info)==0)
        l_info("Main thread stack: %p, %u bytes",thread_info.stack,
               (unsigned)thread_info.stackSize);
    soloader_init_clocks(); alex_check_data(); soloader_init_all();
    alex_java_init();
    jint (*onload)(JavaVM *,void *)=(void *)alex_symbol("JNI_OnLoad");
    if(onload(&jvm,NULL)<0) fatal_error("JNI_OnLoad failed.");
    gl_init(); alex_start();
    unsigned frame=0;
    for(;;) {
        uint64_t start=sceKernelGetProcessTimeWide();
        controls_poll(); alex_java_poll();
        if(!alex_render()) break;
        gl_swap();
        if(frame==0 || frame==60 || frame==600) l_info("Rendered frame %u",frame);
        ++frame;
        uint64_t elapsed=sceKernelGetProcessTimeWide()-start;
        if(elapsed<16667) sceKernelDelayThread(16667-elapsed);
    }
    alex_stop(); sceKernelExitProcess(0); return 0;
}
void controls_handler_key(int32_t key,ControlsAction action) { alex_key(key,action); }
void controls_handler_touch(int32_t id,float x,float y,ControlsAction action) { alex_touch(id,x,y,action); }
void controls_handler_analog(ControlsStickId stick,float x,float y,ControlsAction action) {
    (void)stick;(void)x;(void)y;(void)action;
}
