// The hand-written half of the Android JNI shim: native registration, native
// memory access, and the upcall entry points. See mln_jni.h.
#include <android/api-level.h>
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
    "%s.%s threw; native receives its failure result", upcall->owner,
    upcall->name
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
  (*env)->CallStaticVoidMethodA(env, upcall->type, upcall->method, arguments);
  return mln_jni_upcall_returned(env, upcall);
}

bool mln_jni_upcall_int(
  mln_jni_upcall* upcall, const jvalue* arguments, jint* result
) {
  JNIEnv* env = mln_jni_thread_env();
  if (env == NULL) {
    return false;
  }
  jint value =
    (*env)->CallStaticIntMethodA(env, upcall->type, upcall->method, arguments);
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
  jlong value =
    (*env)->CallStaticLongMethodA(env, upcall->type, upcall->method, arguments);
  if (!mln_jni_upcall_returned(env, upcall)) {
    return false;
  }
  *result = value;
  return true;
}

// org.maplibre.nativeffi.internal.memory.NativeMemory. Accesses go through
// memcpy, which the compiler lowers to one load or store, so a caller's
// alignment never matters.

static jint mln_jni_address_size(void) { return (jint)sizeof(void*); }

static jlong mln_jni_allocate(jlong size) {
  if (size < 0 || (uint64_t)size > SIZE_MAX) {
    return 0;
  }
  return MLN_JNI_ADDRESS(calloc(1, size == 0 ? 1 : (size_t)size));
}

static jlong mln_jni_allocate_uninitialized(jlong size) {
  if (size < 0 || (uint64_t)size > SIZE_MAX) {
    return 0;
  }
  return MLN_JNI_ADDRESS(malloc(size == 0 ? 1 : (size_t)size));
}

static void mln_jni_free(jlong address) {
  free(MLN_JNI_POINTER(void*, address));
}

static void mln_jni_clear(jlong address, jlong size) {
  memset(MLN_JNI_POINTER(void*, address), 0, (size_t)size);
}

// Defines the getter and setter of one primitive Java type.
#define MLN_JNI_ACCESSORS(name, java_type)                          \
  static java_type mln_jni_get_##name(jlong at) {                   \
    java_type value;                                                \
    memcpy(&value, MLN_JNI_POINTER(const void*, at), sizeof value); \
    return value;                                                   \
  }                                                                 \
  static void mln_jni_put_##name(jlong at, java_type value) {       \
    memcpy(MLN_JNI_POINTER(void*, at), &value, sizeof value);       \
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

static jlong mln_jni_string_length(jlong address) {
  return (jlong)strlen(MLN_JNI_POINTER(const char*, address));
}

static jlong mln_jni_put_utf8(
  JNIEnv* env, jclass type, jlong address, jstring value
) {
  (void)type;
  const jsize length = (*env)->GetStringLength(env, value);
  // ART may allocate a UTF-16 copy when a compact string is pinned.
  jchar buffer[128];
  const bool copied = length <= (jsize)(sizeof buffer / sizeof buffer[0]);
  const jchar* text;
  if (copied) {
    (*env)->GetStringRegion(env, value, 0, length, buffer);
    if ((*env)->ExceptionCheck(env)) return 0;
    text = buffer;
  } else {
    text = (*env)->GetStringCritical(env, value, NULL);
  }
  if (text == NULL) return 0;
  uint8_t* output = MLN_JNI_POINTER(uint8_t*, address);
  uint8_t* start = output;
  for (jsize i = 0; i < length; ++i) {
    uint32_t code = text[i];
    if (
      code >= 0xd800 && code <= 0xdbff && i + 1 < length &&
      text[i + 1] >= 0xdc00 && text[i + 1] <= 0xdfff
    ) {
      code = 0x10000 + ((code - 0xd800) << 10) + text[++i] - 0xdc00;
    } else if (code >= 0xd800 && code <= 0xdfff) {
      code = '?';
    }
    if (code < 0x80) {
      *output++ = (uint8_t)code;
    } else if (code < 0x800) {
      *output++ = (uint8_t)(0xc0 | (code >> 6));
      *output++ = (uint8_t)(0x80 | (code & 0x3f));
    } else if (code < 0x10000) {
      *output++ = (uint8_t)(0xe0 | (code >> 12));
      *output++ = (uint8_t)(0x80 | ((code >> 6) & 0x3f));
      *output++ = (uint8_t)(0x80 | (code & 0x3f));
    } else {
      *output++ = (uint8_t)(0xf0 | (code >> 18));
      *output++ = (uint8_t)(0x80 | ((code >> 12) & 0x3f));
      *output++ = (uint8_t)(0x80 | ((code >> 6) & 0x3f));
      *output++ = (uint8_t)(0x80 | (code & 0x3f));
    }
  }
  if (!copied) (*env)->ReleaseStringCritical(env, value, text);
  return (jlong)(output - start);
}

// API 24-25 ignore CriticalNative, so their entry points retain the JNI prefix.
#define MLN_JNI_FALLBACK(result, name, parameters, arguments) \
  static result name##_jni parameters {                       \
    (void)env;                                                \
    (void)type;                                               \
    return name arguments;                                    \
  }
MLN_JNI_FALLBACK(jint, mln_jni_address_size, (JNIEnv * env, jclass type), ())
MLN_JNI_FALLBACK(
  jlong, mln_jni_allocate, (JNIEnv * env, jclass type, jlong size), (size)
)
MLN_JNI_FALLBACK(
  jlong, mln_jni_allocate_uninitialized,
  (JNIEnv * env, jclass type, jlong size), (size)
)
MLN_JNI_FALLBACK(
  void, mln_jni_free, (JNIEnv * env, jclass type, jlong address), (address)
)
MLN_JNI_FALLBACK(
  void, mln_jni_clear, (JNIEnv * env, jclass type, jlong address, jlong size),
  (address, size)
)
MLN_JNI_FALLBACK(
  jlong, mln_jni_string_length, (JNIEnv * env, jclass type, jlong address),
  (address)
)
#define MLN_JNI_ACCESSOR_FALLBACK(name, java_type)                             \
  MLN_JNI_FALLBACK(                                                            \
    java_type, mln_jni_get_##name, (JNIEnv * env, jclass type, jlong at), (at) \
  )                                                                            \
  MLN_JNI_FALLBACK(                                                            \
    void, mln_jni_put_##name,                                                  \
    (JNIEnv * env, jclass type, jlong at, java_type value), (at, value)        \
  )
MLN_JNI_ACCESSOR_FALLBACK(byte, jbyte)
MLN_JNI_ACCESSOR_FALLBACK(short, jshort)
MLN_JNI_ACCESSOR_FALLBACK(int, jint)
MLN_JNI_ACCESSOR_FALLBACK(long, jlong)
MLN_JNI_ACCESSOR_FALLBACK(float, jfloat)
MLN_JNI_ACCESSOR_FALLBACK(double, jdouble)

#define MLN_JNI_MEMORY_METHODS(suffix)                               \
  {"addressSizeNative", "()I", (void*)mln_jni_address_size##suffix}, \
    {"allocateNative", "(J)J", (void*)mln_jni_allocate##suffix},     \
    {"allocateUninitializedNative", "(J)J",                          \
     (void*)mln_jni_allocate_uninitialized##suffix},                 \
    {"free", "(J)V", (void*)mln_jni_free##suffix},                   \
    {"clear", "(JJ)V", (void*)mln_jni_clear##suffix},                \
    {"getByte", "(J)B", (void*)mln_jni_get_byte##suffix},            \
    {"putByte", "(JB)V", (void*)mln_jni_put_byte##suffix},           \
    {"getShort", "(J)S", (void*)mln_jni_get_short##suffix},          \
    {"putShort", "(JS)V", (void*)mln_jni_put_short##suffix},         \
    {"getInt", "(J)I", (void*)mln_jni_get_int##suffix},              \
    {"putInt", "(JI)V", (void*)mln_jni_put_int##suffix},             \
    {"getLong", "(J)J", (void*)mln_jni_get_long##suffix},            \
    {"putLong", "(JJ)V", (void*)mln_jni_put_long##suffix},           \
    {"getFloat", "(J)F", (void*)mln_jni_get_float##suffix},          \
    {"putFloat", "(JF)V", (void*)mln_jni_put_float##suffix},         \
    {"getDouble", "(J)D", (void*)mln_jni_get_double##suffix},        \
    {"putDouble", "(JD)V", (void*)mln_jni_put_double##suffix},       \
    {"stringLength", "(J)J", (void*)mln_jni_string_length##suffix},  \
    {"getBytes", "(JI)[B", (void*)mln_jni_get_bytes},                \
    {"putBytes", "(J[B)V", (void*)mln_jni_put_bytes},                \
    {"putUtf8", "(JLjava/lang/String;)J", (void*)mln_jni_put_utf8},

static const JNINativeMethod mln_jni_memory_methods[] = {
  MLN_JNI_MEMORY_METHODS(_jni)
};
static const JNINativeMethod mln_jni_memory_critical_methods[] = {
  MLN_JNI_MEMORY_METHODS()
};

// org.maplibre.nativeffi.internal.c.Jni.

static jlong mln_jni_upcall_stub(JNIEnv* env, jclass type, jstring name) {
  (void)type;
  const char* chars = (*env)->GetStringUTFChars(env, name, NULL);
  if (chars == NULL) {
    return 0;
  }
  jlong stub = 0;
  for (size_t index = 0; index < mln_jni_upcall_count && stub == 0; ++index) {
    if (strcmp(mln_jni_upcalls[index]->name, chars) == 0) {
      stub = MLN_JNI_ADDRESS(mln_jni_upcall_stubs[index]);
    }
  }
  (*env)->ReleaseStringUTFChars(env, name, chars);
  return stub;
}

// mln_android_init takes the calling JNIEnv and the Context object, the one C
// call whose arguments are Java references.
static jint mln_jni_android_init(
  JNIEnv* env, jclass type, jobject context, jlong diagnostic
) {
  (void)type;
  return (jint)mln_android_init(
    env, context, MLN_JNI_POINTER(mln_diagnostic*, diagnostic)
  );
}

// Releases the context of a native registration that native code did not
// accept, through the host's own release function.
static void mln_jni_call_user_data_release(
  JNIEnv* env, jclass type, jlong release, jlong user_data
) {
  (void)env;
  (void)type;
  MLN_JNI_POINTER(mln_user_data_release, release)
  (MLN_JNI_POINTER(void*, user_data));
}

static const JNINativeMethod mln_jni_runtime_methods[] = {
  {"upcallStub", "(Ljava/lang/String;)J", (void*)mln_jni_upcall_stub},
  {"androidInit", "(Ljava/lang/Object;J)I", (void*)mln_jni_android_init},
  {"callUserDataRelease", "(JJ)V", (void*)mln_jni_call_user_data_release},
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
  const int api_level = android_get_device_api_level();
  if (api_level < 0) {
    MLN_JNI_LOG("cannot read the Android API level");
    return JNI_ERR;
  }
  const bool critical = api_level >= 26;
  if (
    pthread_key_create(&mln_jni_detach_key, mln_jni_detach) != 0 ||
    !mln_jni_register(
      env, MLN_JNI_PACKAGE "c/C",
      critical ? mln_jni_critical_methods : mln_jni_methods,
      mln_jni_method_count
    ) ||
    !mln_jni_register(
      env, MLN_JNI_PACKAGE "memory/NativeMemory",
      critical ? mln_jni_memory_critical_methods : mln_jni_memory_methods,
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
  for (size_t index = 0; index < mln_jni_upcall_count; ++index) {
    mln_jni_upcall* upcall = mln_jni_upcalls[index];
    for (size_t earlier = 0; earlier < index && upcall->type == NULL;
         ++earlier) {
      if (strcmp(mln_jni_upcalls[earlier]->owner, upcall->owner) == 0) {
        upcall->type = mln_jni_upcalls[earlier]->type;
      }
    }
    if (upcall->type == NULL) {
      jclass type = (*env)->FindClass(env, upcall->owner);
      if (type == NULL) {
        MLN_JNI_LOG("cannot find the class %s", upcall->owner);
        return JNI_ERR;
      }
      upcall->type = (*env)->NewGlobalRef(env, type);
      (*env)->DeleteLocalRef(env, type);
    }
    upcall->method = (*env)->GetStaticMethodID(
      env, upcall->type, upcall->name, upcall->signature
    );
    if (upcall->method == NULL) {
      MLN_JNI_LOG(
        "cannot find %s.%s%s", upcall->owner, upcall->name, upcall->signature
      );
      return JNI_ERR;
    }
  }
  return JNI_VERSION_1_6;
}
