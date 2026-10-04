#ifndef ALEX_AUDIO_H
#define ALEX_AUDIO_H
#include <falso_jni/FalsoJNI.h>
void *alex_audio_create(jlong handle,int rate,int channels,int bits,int buffer_bytes);
void alex_audio_start(void *context);
void alex_audio_stop(void *context);
void alex_audio_destroy(void *context);
#endif
