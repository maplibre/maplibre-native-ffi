// The hand-written half of the Android JNI shim: native registration, native
// memory access, and the upcall entry points. See mln_jni.h.
#include <android/log.h>
#include <maplibre_native_c.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "mln_jni.h"

#define MLN_JNI_PACKAGE "org/maplibre/nativeffi/internal/"
#define MLN_JNI_LOG(...) \
  __android_log_print(ANDROID_LOG_ERROR, "maplibre-native-ffi", __VA_ARGS__)

static JavaVM* mln_jni_vm;
static jclass mln_jni_upcalls_class;
static pthread_key_t mln_jni_detach_key;

// Detaches a thread that an upcall attached, when the thread exits.
static void mln_jni_detach(void* env) {
  (void)env;
  (*mln_jni_vm)->DetachCurrentThread(mln_jni_vm);
}

// This thread's JNIEnv, attaching a native thread as a daemon on first use.
static JNIEnv* mln_jni_thread_env(void) {
  JNIEnv* env = NULL;
  jint status =
    (*mln_jni_vm)->GetEnv(mln_jni_vm, (void**)&env, JNI_VERSION_1_6);
  if (status == JNI_OK) {
    return env;
  }
  if (
    status != JNI_EDETACHED ||
    (*mln_jni_vm)->AttachCurrentThreadAsDaemon(mln_jni_vm, &env, NULL) != JNI_OK
  ) {
    MLN_JNI_LOG("cannot attach a native thread for an upcall");
    return NULL;
  }
  pthread_setspecific(mln_jni_detach_key, env);
  return env;
}

// Clears an exception that escaped an upcall, which native cannot receive.
static bool mln_jni_upcall_returned(JNIEnv* env, const mln_jni_upcall* upcall) {
  if (!(*env)->ExceptionCheck(env)) {
    return true;
  }
  MLN_JNI_LOG(
    "Upcalls.%s threw; native receives its failure result", upcall->name
  );
  (*env)->ExceptionDescribe(env);
  (*env)->ExceptionClear(env);
  return false;
}

bool mln_jni_upcall_void(mln_jni_upcall* upcall, const jvalue* arguments) {
  JNIEnv* env = mln_jni_thread_env();
  if (env == NULL) {
    return false;
  }
  (*env)->CallStaticVoidMethodA(
    env, mln_jni_upcalls_class, upcall->method, arguments
  );
  return mln_jni_upcall_returned(env, upcall);
}

bool mln_jni_upcall_int(
  mln_jni_upcall* upcall, const jvalue* arguments, jint* result
) {
  JNIEnv* env = mln_jni_thread_env();
  if (env == NULL) {
    return false;
  }
  jint value = (*env)->CallStaticIntMethodA(
    env, mln_jni_upcalls_class, upcall->method, arguments
  );
  if (!mln_jni_upcall_returned(env, upcall)) {
    return false;
  }
  *result = value;
  return true;
}

bool mln_jni_upcall_long(
  mln_jni_upcall* upcall, const jvalue* arguments, jlong* result
) {
  JNIEnv* env = mln_jni_thread_env();
  if (env == NULL) {
    return false;
  }
  jlong value = (*env)->CallStaticLongMethodA(
    env, mln_jni_upcalls_class, upcall->method, arguments
  );
  if (!mln_jni_upcall_returned(env, upcall)) {
    return false;
  }
  *result = value;
  return true;
}

// org.maplibre.nativeffi.internal.memory.NativeMemory. Accesses go through
// memcpy, which the compiler lowers to one load or store, so a caller's
// alignment never matters.

static jint mln_jni_address_size(JNIEnv* env, jclass type) {
  (void)env;
  (void)type;
  return (jint)sizeof(void*);
}

static jlong mln_jni_allocate(JNIEnv* env, jclass type, jlong size) {
  (void)env;
  (void)type;
  if (size < 0 || (uint64_t)size > SIZE_MAX) {
    return 0;
  }
  return MLN_JNI_ADDRESS(calloc(1, size == 0 ? 1 : (size_t)size));
}

static void mln_jni_free(JNIEnv* env, jclass type, jlong address) {
  (void)env;
  (void)type;
  free(MLN_JNI_POINTER(void*, address));
}

// Defines the getter and setter of one primitive Java type.
#define MLN_JNI_ACCESSORS(name, java_type)                                  \
  static java_type mln_jni_get_##name(JNIEnv* env, jclass type, jlong at) { \
    (void)env;                                                              \
    (void)type;                                                             \
    java_type value;                                                        \
    memcpy(&value, MLN_JNI_POINTER(const void*, at), sizeof value);         \
    return value;                                                           \
  }                                                                         \
  static void mln_jni_put_##name(                                           \
    JNIEnv* env, jclass type, jlong at, java_type value                     \
  ) {                                                                       \
    (void)env;                                                              \
    (void)type;                                                             \
    memcpy(MLN_JNI_POINTER(void*, at), &value, sizeof value);               \
  }

MLN_JNI_ACCESSORS(byte, jbyte)
MLN_JNI_ACCESSORS(short, jshort)
MLN_JNI_ACCESSORS(int, jint)
MLN_JNI_ACCESSORS(long, jlong)
MLN_JNI_ACCESSORS(float, jfloat)
MLN_JNI_ACCESSORS(double, jdouble)

static jbyteArray mln_jni_get_bytes(
  JNIEnv* env, jclass type, jlong address, jint count
) {
  (void)type;
  jbyteArray result = (*env)->NewByteArray(env, count);
  if (result != NULL && count > 0) {
    (*env)->SetByteArrayRegion(
      env, result, 0, count, MLN_JNI_POINTER(const jbyte*, address)
    );
  }
  return result;
}

static void mln_jni_put_bytes(
  JNIEnv* env, jclass type, jlong address, jbyteArray value
) {
  (void)type;
  jsize count = (*env)->GetArrayLength(env, value);
  (*env)->GetByteArrayRegion(
    env, value, 0, count, MLN_JNI_POINTER(jbyte*, address)
  );
}

static jlong mln_jni_string_length(JNIEnv* env, jclass type, jlong address) {
  (void)env;
  (void)type;
  return (jlong)strlen(MLN_JNI_POINTER(const char*, address));
}

static const JNINativeMethod mln_jni_memory_methods[] = {
  {"addressSizeNative", "()I", (void*)mln_jni_address_size},
  {"allocateNative", "(J)J", (void*)mln_jni_allocate},
  {"free", "(J)V", (void*)mln_jni_free},
  {"getByte", "(J)B", (void*)mln_jni_get_byte},
  {"putByte", "(JB)V", (void*)mln_jni_put_byte},
  {"getShort", "(J)S", (void*)mln_jni_get_short},
  {"putShort", "(JS)V", (void*)mln_jni_put_short},
  {"getInt", "(J)I", (void*)mln_jni_get_int},
  {"putInt", "(JI)V", (void*)mln_jni_put_int},
  {"getLong", "(J)J", (void*)mln_jni_get_long},
  {"putLong", "(JJ)V", (void*)mln_jni_put_long},
  {"getFloat", "(J)F", (void*)mln_jni_get_float},
  {"putFloat", "(JF)V", (void*)mln_jni_put_float},
  {"getDouble", "(J)D", (void*)mln_jni_get_double},
  {"putDouble", "(JD)V", (void*)mln_jni_put_double},
  {"getBytes", "(JI)[B", (void*)mln_jni_get_bytes},
  {"putBytes", "(J[B)V", (void*)mln_jni_put_bytes},
  {"stringLength", "(J)J", (void*)mln_jni_string_length},
};

// org.maplibre.nativeffi.internal.c.Jni.

static jlong mln_jni_upcall_stub(JNIEnv* env, jclass type, jint index) {
  (void)env;
  (void)type;
  if (index < 0 || (size_t)index >= mln_jni_upcall_count) {
    return 0;
  }
  return MLN_JNI_ADDRESS(mln_jni_upcall_stubs[index]);
}

// mln_android_init takes the calling JNIEnv and the Context object, the one C
// call whose arguments are Java references.
static jint mln_jni_android_init(
  JNIEnv* env, jclass type, jobject context, jlong diagnostic
) {
  return (jint)mln_android_init(
    env, type, context, MLN_JNI_POINTER(mln_diagnostic*, diagnostic)
  );
}

static const JNINativeMethod mln_jni_runtime_methods[] = {
  {"upcallStub", "(I)J", (void*)mln_jni_upcall_stub},
  {"androidInit", "(Ljava/lang/Object;J)I", (void*)mln_jni_android_init},
};

static bool mln_jni_register(
  JNIEnv* env, const char* name, const JNINativeMethod* methods, size_t count
) {
  jclass type = (*env)->FindClass(env, name);
  if (
    type == NULL ||
    (*env)->RegisterNatives(env, type, methods, (jint)count) != JNI_OK
  ) {
    MLN_JNI_LOG("cannot register the natives of %s", name);
    return false;
  }
  (*env)->DeleteLocalRef(env, type);
  return true;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
  (void)reserved;
  JNIEnv* env = NULL;
  if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }
  mln_jni_vm = vm;
  if (
    pthread_key_create(&mln_jni_detach_key, mln_jni_detach) != 0 ||
    !mln_jni_register(
      env, MLN_JNI_PACKAGE "c/C", mln_jni_methods, mln_jni_method_count
    ) ||
    !mln_jni_register(
      env, MLN_JNI_PACKAGE "memory/NativeMemory", mln_jni_memory_methods,
      sizeof mln_jni_memory_methods / sizeof mln_jni_memory_methods[0]
    ) ||
    !mln_jni_register(
      env, MLN_JNI_PACKAGE "c/Jni", mln_jni_runtime_methods,
      sizeof mln_jni_runtime_methods / sizeof mln_jni_runtime_methods[0]
    )
  ) {
    return JNI_ERR;
  }
  // FindClass here resolves through the class loader that loaded this
  // library; an attached native thread would see only the system classes.
  jclass upcalls = (*env)->FindClass(env, MLN_JNI_PACKAGE "c/Upcalls");
  if (upcalls == NULL) {
    MLN_JNI_LOG("cannot find the Upcalls class");
    return JNI_ERR;
  }
  mln_jni_upcalls_class = (*env)->NewGlobalRef(env, upcalls);
  (*env)->DeleteLocalRef(env, upcalls);
  for (size_t index = 0; index < mln_jni_upcall_count; ++index) {
    mln_jni_upcall* upcall = &mln_jni_upcalls[index];
    upcall->method = (*env)->GetStaticMethodID(
      env, mln_jni_upcalls_class, upcall->name, upcall->signature
    );
    if (upcall->method == NULL) {
      MLN_JNI_LOG("cannot find Upcalls.%s%s", upcall->name, upcall->signature);
      return JNI_ERR;
    }
  }
  return JNI_VERSION_1_6;
}
