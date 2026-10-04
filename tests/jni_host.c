#include "alex.h"
#include "audio.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static unsigned destroyed,started,stopped;
void fatal_error(const char *fmt,...) {va_list a;va_start(a,fmt);vfprintf(stderr,fmt,a);va_end(a);abort();}
uintptr_t alex_symbol(const char *name) {(void)name;abort();}
void *alex_audio_create(jlong h,int r,int c,int b,int n) {
    assert(h==0x1122334455667788LL && r==44100 && c==2 && b==16 && n==8192);
    return calloc(1,16);
}
void alex_audio_start(void *p) {assert(p);++started;}
void alex_audio_stop(void *p) {assert(p);++stopped;}
void alex_audio_destroy(void *p) {++destroyed;free(p);}
int main(void) {
    jni_init();alex_java_init();
    for(int i=0;i<1000;++i) {
        jstring s=(*jni).NewStringUTF(&jni,"Alex caf\xc3\xa9");
        jobject g=(*jni).NewGlobalRef(&jni,s);
        (*jni).DeleteLocalRef(&jni,s);
        const char *t=(*jni).GetStringUTFChars(&jni,g,NULL);
        assert(!strcmp(t,"Alex caf\xc3\xa9"));
        (*jni).ReleaseStringUTFChars(&jni,g,(char *)t);
        (*jni).DeleteGlobalRef(&jni,g);
        jbyteArray a=(*jni).NewByteArray(&jni,4096);
        jbyte data[4]={1,2,3,4},copy[4]={0};
        (*jni).SetByteArrayRegion(&jni,a,4092,4,data);
        (*jni).GetByteArrayRegion(&jni,a,4092,4,copy);
        assert(!memcmp(data,copy,4));
        (*jni).DeleteLocalRef(&jni,a);
    }
    jclass audio=(*jni).FindClass(&jni,"com/rovio/ka3d/AudioOutput");
    jmethodID init=(*jni).GetMethodID(&jni,audio,"<init>","(JIIII)V");
    jobject local=(*jni).NewObject(&jni,audio,init,(jlong)0x1122334455667788LL,44100,2,16,8192);
    assert(local);
    jobject retained=(*jni).NewGlobalRef(&jni,local);(*jni).DeleteLocalRef(&jni,local);
    (*jni).CallVoidMethod(&jni,retained,(*jni).GetMethodID(&jni,audio,"startOutput","()V"));
    (*jni).CallVoidMethod(&jni,retained,(*jni).GetMethodID(&jni,audio,"stopOutput","()V"));
    assert(started==1 && stopped==1 && destroyed==0);
    (*jni).DeleteGlobalRef(&jni,retained);assert(destroyed==1);
    (*jni).DeleteLocalRef(&jni,audio);
    jclass renderer=(*jni).GetObjectClass(&jni,alex_renderer());
    jmethodID read=(*jni).GetMethodID(&jni,renderer,"readFile","(Ljava/lang/String;)[B");
    jstring name=(*jni).NewStringUTF(&jni,"fixture.bin");
    jbyteArray contents=(*jni).CallObjectMethod(&jni,alex_renderer(),read,name);
    assert(contents && (*jni).GetArrayLength(&jni,contents)==5);
    char copy[6]={0};(*jni).GetByteArrayRegion(&jni,contents,0,5,(jbyte *)copy);
    assert(!strcmp(copy,"hello"));
    (*jni).DeleteLocalRef(&jni,contents);(*jni).DeleteLocalRef(&jni,name);
    name=(*jni).NewStringUTF(&jni,"../fixture.bin");
    assert(!(*jni).CallObjectMethod(&jni,alex_renderer(),read,name));
    (*jni).DeleteLocalRef(&jni,name);
    assert(!(*jni).CallBooleanMethod(&jni,alex_renderer(),(*jni).GetMethodID(&jni,renderer,"canOpenEmail","()Z")));
    (*jni).DeleteLocalRef(&jni,renderer);
    puts("JNI reference lifetime, UTF-8, arrays, constructor arguments, asset reads and offline availability passed.");
}
