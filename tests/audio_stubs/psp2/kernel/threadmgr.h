#ifndef TEST_THREADMGR_H
#define TEST_THREADMGR_H
#include <stddef.h>
typedef int SceUID;
typedef unsigned SceSize;
typedef int (*SceKernelThreadEntry)(SceSize,void *);
SceUID sceKernelCreateThread(const char *,SceKernelThreadEntry,int,SceSize,unsigned,int,const void *);
int sceKernelStartThread(SceUID,SceSize,void *);
int sceKernelDeleteThread(SceUID);
int sceKernelWaitThreadEnd(SceUID,int *,unsigned *);
#endif
