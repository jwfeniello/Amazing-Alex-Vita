#ifndef ALEX_H
#define ALEX_H
#include <stdbool.h>
#include <stdint.h>
#include <falso_jni/FalsoJNI.h>
#include "reimpl/controls.h"
uintptr_t alex_symbol(const char *name);
void alex_log_init(void);
void alex_java_init(void);
jobject alex_renderer(void);
void alex_check_data(void);
void alex_start(void);
bool alex_render(void);
void alex_stop(void);
void alex_touch(int id, float x, float y, ControlsAction action);
void alex_key(int key, ControlsAction action);
void alex_java_poll(void);
#endif
