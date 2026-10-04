#ifndef ALEX_JAVA_REFS_H
#define ALEX_JAVA_REFS_H
#include <stdbool.h>
#include <falso_jni/FalsoJNI.h>
enum AlexRefKind { ALEX_REF_CLASS, ALEX_REF_STRING, ALEX_REF_ARRAY, ALEX_REF_OBJECT };
void alex_refs_install(void);
jobject alex_ref_track(void *object,enum AlexRefKind kind,const char *class_name,void (*destroy)(void *));
const char *alex_ref_class(jobject object);
void alex_ref_keep(jobject object);
#endif
