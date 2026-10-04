#include "alex.h"
#include "java_refs.h"
#include "audio.h"
#include "utils/logger.h"
#include "utils/dialog.h"
#include <falso_jni/FalsoJNI_Impl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { M_READ=1,M_SILENT,M_AUDIO_INIT,M_AUDIO_START,M_AUDIO_STOP,
       M_APP,M_LOCALE,M_STRING,M_ID,M_ID_HASH,M_URL,M_CAN_OPEN,M_EMAIL,
       M_CAN_EMAIL,M_TEXT,M_ANALYTICS,M_MAP_INIT,M_MAP_PUT,
       M_WEB_INIT,M_WEB_LOAD,M_WEB_RELOAD,M_WEB_DESTROY,M_WEB_SHOW,M_WEB_HIDE,
       M_WEB_SCRIPT,M_WEB_SCRIPT_ASYNC,M_ALERT };
typedef struct { unsigned id;const char *name,*sig; } Method;
static const Method methods[]={
 {M_READ,"readFile","(Ljava/lang/String;)[B"},
 {M_SILENT,"isSilentProfile","()Z"},
 {M_AUDIO_START,"startOutput","()V"},{M_AUDIO_STOP,"stopOutput","()V"},
 {M_APP,"getInstance","()Lcom/rovio/ka3d/App;"},
 {M_LOCALE,"getDefault","()Ljava/util/Locale;"},{M_STRING,"toString","()Ljava/lang/String;"},
 {M_ID,"getUniqueId","()Ljava/lang/String;"},{M_ID_HASH,"getUniqueIdHash","()Ljava/lang/String;"},
 {M_URL,"openURL","(Ljava/lang/String;)V"},{M_CAN_OPEN,"canOpenProgram","(Ljava/lang/String;)Z"},
 {M_EMAIL,"openEmail","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"},
 {M_CAN_EMAIL,"canOpenEmail","()Z"},{M_TEXT,"enableTextInput","(Z)V"},
 {M_ANALYTICS,"onStartSession","(Landroid/content/Context;Ljava/lang/String;)V"},
 {M_ANALYTICS,"onEndSession","(Landroid/content/Context;)V"},
 {M_ANALYTICS,"onEvent","(Ljava/lang/String;Ljava/util/Map;)V"},
 {M_MAP_PUT,"put","(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"},
 {M_WEB_LOAD,"loadUrl","(Ljava/lang/String;)V"},{M_WEB_RELOAD,"reload","()V"},
 {M_WEB_DESTROY,"destroy","()V"},{M_WEB_SHOW,"show","()V"},{M_WEB_HIDE,"hide","()V"},
 {M_WEB_SCRIPT,"executeJavaScript","(Ljava/lang/String;)Ljava/lang/String;"},
 {M_WEB_SCRIPT_ASYNC,"asyncExecuteJavaScript","(Ljava/lang/String;)V"}
};
typedef struct { jlong handle; } WebObject;
static jobject renderer_object,app_object,locale_object;
static bool web_failed;
static jlong failed_web_handle;

static jmethodID method_id(JNIEnv *env,jclass cls,const char *name,const char *sig) {
    (void)env;
    if(!strcmp(name,"<init>")) {
        const char *c=(const char *)cls;
        if(!strcmp(c,"com/rovio/ka3d/AudioOutput") && !strcmp(sig,"(JIIII)V")) return (jmethodID)M_AUDIO_INIT;
        if(!strcmp(c,"java/util/HashMap") && !strcmp(sig,"()V")) return (jmethodID)M_MAP_INIT;
        if(!strcmp(c,"com/rovio/ka3d/WebViewWrapper") && !strcmp(sig,"(IIIIJ)V")) return (jmethodID)M_WEB_INIT;
    }
    for(unsigned i=0;i<sizeof(methods)/sizeof(methods[0]);++i)
        if(!strcmp(name,methods[i].name) && !strcmp(sig,methods[i].sig)) return (jmethodID)methods[i].id;
    if(!strcmp(name,"showAlert")) return (jmethodID)M_ALERT;
    l_error("Unimplemented JNI method %s %s",name,sig);return NULL;
}
static jobject plain_object(const char *cls) {return alex_ref_track(calloc(1,8),ALEX_REF_OBJECT,cls,NULL);}
static jobject read_asset(jstring filename) {
    const char *name=(*jni).GetStringUTFChars(&jni,filename,NULL);
    if(!name) return NULL;
    char path[768];jbyteArray array=NULL;
    if(name[0]=='/' || strstr(name,"..") || strchr(name,':') || strchr(name,'\\')) goto done;
    int n=snprintf(path,sizeof(path),DATA_PATH "assets/%s",name);
    if(n<0 || n>=(int)sizeof(path)) goto done;
    size_t len=strlen(path);
    if(len>4 && !strcmp(path+len-4,".zip")) path[len-4]=0;
    FILE *f=fopen(path,"rb");
    if(!f) {l_warn("Asset not found: %s",name);goto done;}
    if(fseek(f,0,SEEK_END)) {fclose(f);goto done;}
    long bytes=ftell(f);
    if(bytes<0 || bytes>32*1024*1024 || fseek(f,0,SEEK_SET)) {fclose(f);goto done;}
    array=(*jni).NewByteArray(&jni,(jsize)bytes);
    if(array) {
        jbyte *data=(*jni).GetByteArrayElements(&jni,array,NULL);
        if(fread(data,1,(size_t)bytes,f)!=(size_t)bytes) {
            (*jni).DeleteLocalRef(&jni,array);array=NULL;
        }
    }
    fclose(f);
done:
    (*jni).ReleaseStringUTFChars(&jni,filename,(char *)name);return array;
}
static jobject new_object_v(JNIEnv *env,jclass cls,jmethodID id,va_list args) {
    (void)env;(void)cls;
    switch((uintptr_t)id) {
    case M_AUDIO_INIT: {
        jlong handle=va_arg(args,jlong);
        int rate=va_arg(args,int),channels=va_arg(args,int),bits=va_arg(args,int),buffer=va_arg(args,int);
        void *a=alex_audio_create(handle,rate,channels,bits,buffer);
        return alex_ref_track(a,ALEX_REF_OBJECT,"com/rovio/ka3d/AudioOutput",alex_audio_destroy);
    }
    case M_MAP_INIT: return plain_object("java/util/HashMap");
    case M_WEB_INIT: {
        for(int i=0;i<4;++i) (void)va_arg(args,int);
        WebObject *w=calloc(1,sizeof(*w));if(!w)return NULL;
        w->handle=va_arg(args,jlong);
        return alex_ref_track(w,ALEX_REF_OBJECT,"com/rovio/ka3d/WebViewWrapper",NULL);
    }
    default: l_error("Unknown Java constructor %u",(unsigned)(uintptr_t)id);return NULL;
    }
}
static jobject object_v(JNIEnv *env,jobject object,jmethodID id,va_list args) {
    (void)object;
    switch((uintptr_t)id) {
    case M_READ: return read_asset(va_arg(args,jstring));
    case M_APP: return (*jni).NewLocalRef(env,app_object);
    case M_LOCALE:return (*jni).NewLocalRef(env,locale_object);
    case M_STRING:return (*jni).NewStringUTF(env,"en_US");
    case M_ID:case M_ID_HASH:return (*jni).NewStringUTF(env,"amazing-alex-vita-offline");
    case M_MAP_PUT:case M_WEB_SCRIPT:return NULL;
    default:l_error("Unknown Java object call %u",(unsigned)(uintptr_t)id);return NULL;
    }
}
static jboolean boolean_v(JNIEnv *env,jobject obj,jmethodID id,va_list args) {
    (void)env;(void)obj;(void)args;
    if((uintptr_t)id!=M_SILENT && (uintptr_t)id!=M_CAN_OPEN && (uintptr_t)id!=M_CAN_EMAIL)
        l_error("Unknown Java boolean call %u",(unsigned)(uintptr_t)id);
    return JNI_FALSE;
}
static void void_v(JNIEnv *env,jobject obj,jmethodID id,va_list args) {
    (void)env;
    switch((uintptr_t)id) {
    case M_AUDIO_START:alex_audio_start(obj);break;
    case M_AUDIO_STOP:alex_audio_stop(obj);break;
    case M_ANALYTICS:break; /* No analytics are sent or queued. */
    case M_URL:case M_EMAIL:l_info("External web action unavailable in offline port");break;
    case M_WEB_LOAD:case M_WEB_RELOAD:
        failed_web_handle=((WebObject *)obj)->handle;web_failed=true;break;
    case M_WEB_DESTROY:
        if(failed_web_handle==((WebObject *)obj)->handle) web_failed=false;
        break;
    case M_WEB_SHOW:case M_WEB_HIDE:case M_WEB_SCRIPT_ASYNC:break;
    case M_TEXT:l_warn("Text entry requested; IME integration pending");break;
    case M_ALERT:l_warn("Native alert requested");break;
    default:l_error("Unknown Java void call %u",(unsigned)(uintptr_t)id);break;
    }
}
#define VAR_CALL(TYPE,NAME,TARGET) \
static TYPE NAME(JNIEnv *env,jobject obj,jmethodID id,...) { \
    va_list a;va_start(a,id);TYPE result=TARGET(env,obj,id,a);va_end(a);return result; }
VAR_CALL(jobject,object_call,object_v)
VAR_CALL(jboolean,boolean_call,boolean_v)
VAR_CALL(jobject,new_object,new_object_v)
static void void_call(JNIEnv *env,jobject obj,jmethodID id,...) {
    va_list a;va_start(a,id);void_v(env,obj,id,a);va_end(a);
}
static jfieldID field_id(JNIEnv *env,jclass cls,const char *name,const char *sig) {
    (void)env;(void)cls;(void)sig;
    if(!strcmp(name,"RELEASE"))return (jfieldID)1;
    if(!strcmp(name,"MODEL"))return (jfieldID)2;
    if(!strcmp(name,"MANUFACTURER"))return (jfieldID)3;
    l_warn("Unknown Java field %s",name);return NULL;
}
static jobject static_field(JNIEnv *env,jclass cls,jfieldID id) {
    (void)cls;return (*jni).NewStringUTF(env,(uintptr_t)id==1 ? "4.0.4" : (uintptr_t)id==2 ? "PlayStation Vita" : "Sony");
}
jobject alex_renderer(void) {return renderer_object;}
void alex_java_init(void) {
    alex_refs_install();
    renderer_object=plain_object("com/rovio/ka3d/MyRenderer");
    app_object=plain_object("com/rovio/ka3d/App");locale_object=plain_object("java/util/Locale");
    struct JNINativeInterface *t=(void *)jni;
    t->GetMethodID=method_id;t->GetStaticMethodID=method_id;
    t->NewObject=new_object;t->NewObjectV=new_object_v;
    t->CallObjectMethod=object_call;t->CallObjectMethodV=object_v;
    t->CallStaticObjectMethod=object_call;t->CallStaticObjectMethodV=object_v;
    t->CallBooleanMethod=boolean_call;t->CallBooleanMethodV=boolean_v;
    t->CallStaticBooleanMethod=boolean_call;t->CallStaticBooleanMethodV=boolean_v;
    t->CallVoidMethod=void_call;t->CallVoidMethodV=void_v;
    t->CallStaticVoidMethod=void_call;t->CallStaticVoidMethodV=void_v;
    t->GetStaticFieldID=field_id;t->GetStaticObjectField=static_field;
}
void alex_java_poll(void) {
    if(web_failed) {
        web_failed=false;
        void (*done)(JNIEnv *,jobject,jlong,jboolean,jstring)=(void *)alex_symbol("Java_com_rovio_ka3d_WebViewWrapper_urlLoadedCallback");
        jstring text=(*jni).NewStringUTF(&jni,"Online features are unavailable on this port.");
        done(&jni,NULL,failed_web_handle,JNI_FALSE,text);(*jni).DeleteLocalRef(&jni,text);
    }
}
/* FalsoJNI supplies strings, arrays and VM operations. Calls are dispatched
   above because stream/audio receivers must preserve their object identity. */
NameToMethodID nameToMethodId[]={};
NameToFieldID nameToFieldId[]={};
#define EMPTY(TYPE,NAME) TYPE NAME[]={};
EMPTY(MethodsBoolean,methodsBoolean) EMPTY(MethodsByte,methodsByte)
EMPTY(MethodsChar,methodsChar) EMPTY(MethodsDouble,methodsDouble)
EMPTY(MethodsFloat,methodsFloat) EMPTY(MethodsInt,methodsInt)
EMPTY(MethodsLong,methodsLong) EMPTY(MethodsObject,methodsObject)
EMPTY(MethodsShort,methodsShort) EMPTY(MethodsVoid,methodsVoid)
EMPTY(FieldsBoolean,fieldsBoolean) EMPTY(FieldsByte,fieldsByte)
EMPTY(FieldsChar,fieldsChar) EMPTY(FieldsDouble,fieldsDouble)
EMPTY(FieldsFloat,fieldsFloat) EMPTY(FieldsInt,fieldsInt)
EMPTY(FieldsLong,fieldsLong) EMPTY(FieldsObject,fieldsObject) EMPTY(FieldsShort,fieldsShort)
__FALSOJNI_IMPL_CONTAINER_SIZES
