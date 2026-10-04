/* Exercise the real playback loop against a backend which retains submitted
 * buffers. No Vita calls or Android machine code execute on the host. */
#include <assert.h>
#include <stdio.h>
#include "audio.c"

static Audio *active;
static SceKernelThreadEntry entry;
static const void *queued;
static int16_t snapshot[1024*2];
static unsigned writes,mixes,source_frame,releases;
static unsigned sample_mode;

static jbyteArray new_array(JNIEnv *env,jsize n) {(void)env;return (jbyteArray)calloc(1,(size_t)n);}
static jbyte *elements(JNIEnv *env,jbyteArray a,jboolean *copy) {(void)env;(void)copy;return (jbyte *)a;}
static void delete_ref(JNIEnv *env,jobject a) {(void)env;free(a);}
static struct JNINativeInterface table={.NewByteArray=new_array,.GetByteArrayElements=elements,.DeleteLocalRef=delete_ref};
JNIEnv jni=&table;

static int16_t signal_at(unsigned frame) {return (int16_t)(-24000+(frame%1009)*47);}
static void native_mix(JNIEnv *env,jobject obj,jlong handle,jbyteArray array,jint bytes) {
    (void)env;(void)obj;assert(handle==1234);++mixes;
    if(sample_mode==0) {
        assert(bytes%4==0);
        int16_t *p=(void *)array;
        for(int i=0;i<bytes/4;++i,++source_frame) {
            p[2*i]=signal_at(source_frame);p[2*i+1]=(int16_t)-p[2*i];
        }
    } else {
        /* Unsigned 8-bit PCM silence must become signed 16-bit silence. */
        memset(array,128,(size_t)bytes);source_frame+=(unsigned)bytes;
    }
}
uintptr_t alex_symbol(const char *name) {
    assert(!strcmp(name,"Java_com_rovio_ka3d_AudioOutput_nativeMixData"));
    return (uintptr_t)native_mix;
}
int sceAudioOutOpenPort(int type,int frames,int rate,int mode) {
    assert(type==0 && frames==1024 && rate==48000 && mode==1);return 7;
}
int sceAudioOutSetVolume(int port,int flags,int *volume) {
    assert(port==7 && flags==3 && volume[0]==32768 && volume[1]==32768);return 0;
}
int sceAudioOutOutput(int port,const void *data) {
    assert(port==7);
    if(queued && memcmp(queued,snapshot,sizeof(snapshot))) {
        fputs("Queued PCM was overwritten before playback finished.\n",stderr);abort();
    }
    queued=NULL;
    if(!data)return 0;
    assert((uintptr_t)data%64==0);
    const int16_t *samples=data;
    for(unsigned i=0;i<1024;++i) {
        if(sample_mode==0) {
            assert(samples[2*i]==-samples[2*i+1]);
            unsigned output_frame=writes*1024+i;
            if(output_frame%3==0)assert(samples[2*i]==signal_at(output_frame/3));
        } else assert(samples[2*i]==0 && samples[2*i+1]==0);
    }
    queued=data;memcpy(snapshot,data,sizeof(snapshot));
    if(++writes==12)atomic_store(&active->stop,true);
    return 1024;
}
int sceAudioOutReleasePort(int port) {assert(port==7 && !queued);++releases;return 0;}
SceUID sceKernelCreateThread(const char *name,SceKernelThreadEntry fn,int p,SceSize s,unsigned a,int affinity,const void *o) {
    (void)name;(void)p;(void)s;(void)a;(void)affinity;(void)o;entry=fn;return 9;
}
int sceKernelStartThread(SceUID id,SceSize size,void *arg) {
    assert(id==9);active=*(Audio **)arg;return entry(size,arg);
}
int sceKernelWaitThreadEnd(SceUID id,int *status,unsigned *timeout) {(void)status;(void)timeout;assert(id==9);return 0;}
int sceKernelDeleteThread(SceUID id) {assert(id==9);return 0;}

int main(void) {
    for(sample_mode=0;sample_mode<2;++sample_mode) {
        writes=mixes=source_frame=releases=0;
        void *context=alex_audio_create(1234,16000,sample_mode?1:2,sample_mode?8:16,8192);
        assert(context);alex_audio_start(context);alex_audio_destroy(context);
        assert(writes==12 && mixes>=3 && releases==1 && !queued);
    }
    puts("Audio buffer ownership, alignment, stereo continuity across refills, unsigned PCM silence and shutdown passed.");
}
