#ifndef TEST_AUDIOOUT_H
#define TEST_AUDIOOUT_H
#define SCE_AUDIO_OUT_PORT_TYPE_MAIN 0
#define SCE_AUDIO_OUT_MODE_STEREO 1
#define SCE_AUDIO_VOLUME_0DB 32768
#define SCE_AUDIO_VOLUME_FLAG_L_CH 1
#define SCE_AUDIO_VOLUME_FLAG_R_CH 2
int sceAudioOutOpenPort(int type,int frames,int rate,int mode);
int sceAudioOutSetVolume(int port,int flags,int *volume);
int sceAudioOutOutput(int port,const void *data);
int sceAudioOutReleasePort(int port);
#endif
