#include "audio.h"
#include "alex.h"
#include "utils/logger.h"
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdatomic.h>

#define OUT_FRAMES 1024
#define INPUT_FRAMES 2048
#define OUTPUT_BUFFERS 2
typedef struct {
    jlong handle;
    int rate,channels,bits;
    SceUID thread;
    atomic_bool stop;
    jbyteArray samples;
    unsigned cursor;
    void (*mix)(JNIEnv *,jobject,jlong,jbyteArray,jint);
} Audio;
static void next_frame(Audio *a,int32_t out[2]) {
    unsigned frame_bytes=(unsigned)a->channels*a->bits/8;
    if(a->cursor>=INPUT_FRAMES) {
        a->mix(&jni,NULL,a->handle,a->samples,INPUT_FRAMES*frame_bytes);
        a->cursor=0;
    }
    const unsigned char *p=(void *)(*jni).GetByteArrayElements(&jni,a->samples,NULL);
    p+=a->cursor++*frame_bytes;
    if(a->bits==16) {
        out[0]=(int16_t)(p[0]|p[1]<<8);
        out[1]=a->channels==2 ? (int16_t)(p[2]|p[3]<<8) : out[0];
    } else {
        out[0]=((int)p[0]-128)*256;
        out[1]=a->channels==2 ? ((int)p[1]-128)*256 : out[0];
    }
}
static int audio_thread(SceSize size,void *arg) {
    (void)size;Audio *a=*(Audio **)arg;
    /* Keep the submitted PCM alive while filling the next chunk. Vita's
     * blocking output call can return with the current buffer still queued.
     * Use 64-byte alignment, as in SDL's Vita output backend. */
    int16_t (*output)[OUT_FRAMES*2]=memalign(64,OUTPUT_BUFFERS*sizeof(*output));
    if(!output) {l_error("Audio buffer allocation failed");return 0;}
    int port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,OUT_FRAMES,48000,SCE_AUDIO_OUT_MODE_STEREO);
    if(port<0) {l_error("Audio output failed: %08x",port);free(output);return 0;}
    int volume[2]={SCE_AUDIO_VOLUME_0DB,SCE_AUDIO_VOLUME_0DB};
    sceAudioOutSetVolume(port,SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH,volume);
    unsigned slot=0;
    int32_t left[2],right[2];unsigned phase=0;
    a->cursor=INPUT_FRAMES;next_frame(a,left);next_frame(a,right);
    while(!atomic_load(&a->stop)) {
        for(unsigned frame=0;frame<OUT_FRAMES;++frame) {
            for(unsigned ch=0;ch<2;++ch)
                output[slot][frame*2+ch]=(int16_t)(left[ch]+(int64_t)(right[ch]-left[ch])*phase/48000);
            phase+=(unsigned)a->rate;
            while(phase>=48000) {
                phase-=48000;left[0]=right[0];left[1]=right[1];next_frame(a,right);
            }
        }
        int rc=sceAudioOutOutput(port,output[slot]);
        if(rc<0) {l_error("Audio write failed: %08x",rc);break;}
        slot=(slot+1)%OUTPUT_BUFFERS;
    }
    sceAudioOutOutput(port,NULL);sceAudioOutReleasePort(port);
    free(output);
    return 0;
}
void *alex_audio_create(jlong handle,int rate,int channels,int bits,int buffer_bytes) {
    (void)buffer_bytes;
    if(rate<8000 || rate>96000 || channels<1 || channels>2 || (bits!=8 && bits!=16)) {
        l_error("Unsupported native audio format %d Hz / %d ch / %d bits",rate,channels,bits);return NULL;
    }
    Audio *a=calloc(1,sizeof(*a));if(!a)return NULL;
    a->handle=handle;a->rate=rate;a->channels=channels;a->bits=bits;a->thread=-1;
    atomic_init(&a->stop,false);
    a->mix=(void *)alex_symbol("Java_com_rovio_ka3d_AudioOutput_nativeMixData");
    a->samples=(*jni).NewByteArray(&jni,INPUT_FRAMES*channels*bits/8);
    if(!a->samples) {free(a);return NULL;}
    l_info("Native audio: %d Hz, %d channels, %d bits",rate,channels,bits);
    return a;
}
void alex_audio_start(void *context) {
    Audio *a=context;if(!a || a->thread>=0)return;
    atomic_store(&a->stop,false);
    a->thread=sceKernelCreateThread("alex_audio",audio_thread,0x40,128*1024,0,0,NULL);
    if(a->thread<0) {l_error("Audio thread creation failed: %08x",a->thread);return;}
    int rc=sceKernelStartThread(a->thread,sizeof(a),&a);
    if(rc<0) {sceKernelDeleteThread(a->thread);a->thread=-1;l_error("Audio thread start failed: %08x",rc);}
}
void alex_audio_stop(void *context) {
    Audio *a=context;if(!a || a->thread<0)return;
    atomic_store(&a->stop,true);
    sceKernelWaitThreadEnd(a->thread,NULL,NULL);sceKernelDeleteThread(a->thread);a->thread=-1;
}
void alex_audio_destroy(void *context) {
    Audio *a=context;if(!a)return;
    alex_audio_stop(a);(*jni).DeleteLocalRef(&jni,a->samples);free(a);
}
