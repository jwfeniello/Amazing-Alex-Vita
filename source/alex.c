#include "alex.h"
#include "utils/dialog.h"
#include "utils/logger.h"
#include <so_util/so_util.h>
#include <psp2/io/stat.h>
#include <stdio.h>

extern so_module so_mod;
static jboolean (*update_fn)(JNIEnv *, jobject);
static void (*touch_fn)(JNIEnv *, jobject, jint, jfloat, jfloat, jint);
static void (*key_fn)(JNIEnv *, jobject, jint, jint, jint);

uintptr_t alex_symbol(const char *name) {
    uintptr_t p = so_symbol(&so_mod, name);
    if (!p) fatal_error("Missing game function: %s", name);
    return p;
}
void alex_check_data(void) {
    SceIoStat st;
    if (sceIoGetstat(SO_PATH, &st) < 0 || st.st_size != 2568272)
        fatal_error("Copy the prepared Amazing Alex HD 1.0.5 data to " DATA_PATH);
    if (sceIoGetstat(DATA_PATH "assets/Data/Common", &st) < 0)
        fatal_error("Missing Amazing Alex assets in " DATA_PATH "assets/");
    sceIoMkdir(DATA_PATH "saves", 0777);
    static const char *dirs[] = {"Solutions", "Sandbox", "Featured", "Downloaded"};
    for (unsigned i=0;i<sizeof(dirs)/sizeof(dirs[0]);++i) {
        char path[256]; snprintf(path,sizeof(path),DATA_PATH "saves/%s",dirs[i]);
        sceIoMkdir(path,0777);
    }
}
void alex_start(void) {
    jboolean (*init)(JNIEnv *,jobject,jint,jint,jstring) = (void *)alex_symbol("Java_com_rovio_ka3d_MyRenderer_nativeInit");
    jboolean (*resize)(JNIEnv *,jobject,jint,jint) = (void *)alex_symbol("Java_com_rovio_ka3d_MyRenderer_nativeResize");
    void (*resume)(JNIEnv *,jobject) = (void *)alex_symbol("Java_com_rovio_ka3d_MyRenderer_nativeResume");
    update_fn=(void *)alex_symbol("Java_com_rovio_ka3d_MyRenderer_nativeUpdate");
    touch_fn=(void *)alex_symbol("Java_com_rovio_ka3d_MyRenderer_nativeInput");
    key_fn=(void *)alex_symbol("Java_com_rovio_ka3d_MyRenderer_nativeKeyInput");
    jstring save=(*jni).NewStringUTF(&jni,DATA_PATH "saves");
    l_info("Calling nativeInit(960,544)");
    jboolean ok=init(&jni,alex_renderer(),960,544,save);
    (*jni).DeleteLocalRef(&jni,save);
    if (!ok) fatal_error("Amazing Alex initialization failed. See loader.log.");
    l_info("Native initialization returned; resizing and resuming");
    resize(&jni,alex_renderer(),960,544);
    resume(&jni,alex_renderer());
}
bool alex_render(void) { return update_fn(&jni,alex_renderer()) != JNI_FALSE; }
void alex_touch(int id,float x,float y,ControlsAction action) {
    int event=action==CONTROLS_ACTION_DOWN ? 0 : action==CONTROLS_ACTION_UP ? 1 : 2;
    touch_fn(&jni,alex_renderer(),event,x,y,id);
}
void alex_key(int key,ControlsAction action) {
    if(key==AKEYCODE_BUTTON_START || key==AKEYCODE_BUTTON_B) key=AKEYCODE_BACK;
    key_fn(&jni,alex_renderer(),key,action==CONTROLS_ACTION_DOWN ? 1 : 0,0);
}
void alex_stop(void) {
    void (*pause)(JNIEnv *,jobject)=(void *)alex_symbol("Java_com_rovio_ka3d_MyRenderer_nativePause");
    void (*deinit)(JNIEnv *,jobject)=(void *)alex_symbol("Java_com_rovio_ka3d_MyRenderer_nativeDeinit");
    pause(&jni,alex_renderer()); deinit(&jni,alex_renderer());
}
