// The JNI runtime that the generated Android shim calls into.
//
// The Kotlin binding marshals every value in common code, so a JNI downcall
// passes only primitives: integers, floats, and addresses carried as jlong. A
// downcall never touches a Java object, so it creates no local reference and
// raises no Java exception. Everything JNI needs beyond that lives in
// mln_jni.c: registering the natives, attaching native threads for upcalls, and
// clearing an exception that escapes an upcall.
#ifndef MLN_JNI_H
#define MLN_JNI_H

#include <jni.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Addresses travel zero-extended, as Memory.kt reads them, so a 32-bit address
// at or above 2^31 is the same Long whichever side produced it.

// A pointer argument from its Kotlin Long carrier.
#define MLN_JNI_POINTER(type, value) ((type)(uintptr_t)(value))

// A record that C takes by value, from the address of the caller's copy.
#define MLN_JNI_RECORD(type, value) (*(const type*)(uintptr_t)(value))

// A pointer as its Kotlin Long carrier.
#define MLN_JNI_ADDRESS(value) ((jlong)(uintptr_t)(value))

// One static method of org.maplibre.nativeffi.internal.c.Upcalls, which an
// upcall stub calls. JNI_OnLoad resolves its method ID.
typedef struct mln_jni_upcall {
  const char* name;
  const char* signature;
  jmethodID method;
} mln_jni_upcall;

// Calls an Upcalls method from any thread, attaching a native thread for its
// lifetime. Each returns false, leaving *result untouched, when the thread
// cannot attach or the method throws; the Kotlin methods catch everything, so
// that happens only on a JVM error such as running out of memory.
bool mln_jni_upcall_void(mln_jni_upcall* upcall, const jvalue* arguments);
bool mln_jni_upcall_int(
  mln_jni_upcall* upcall, const jvalue* arguments, jint* result
);
bool mln_jni_upcall_long(
  mln_jni_upcall* upcall, const jvalue* arguments, jlong* result
);

// Defined by the generated shim: the natives of
// org.maplibre.nativeffi.internal.c.C, the Upcalls methods, and one C function
// per upcall, in the order UpcallStubs indexes them.
extern const JNINativeMethod mln_jni_methods[];
extern const size_t mln_jni_method_count;
extern mln_jni_upcall mln_jni_upcalls[];
extern void* const mln_jni_upcall_stubs[];
extern const size_t mln_jni_upcall_count;

#endif  // MLN_JNI_H
