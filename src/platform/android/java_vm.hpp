#pragma once

#include <jni.h>

namespace mln::platform::android {

// Records the process's Java VM, which every platform service that calls Java
// reaches through. mln_android_init calls this.
void initialize_java_vm(JNIEnv& env);

// The recorded Java VM, or null before mln_android_init.
auto java_vm() -> JavaVM*;

// Keeps the calling thread attached to the Java VM until detach_thread, so
// Java calls on a long-lived thread do not attach and detach each time.
void attach_thread();
void detach_thread();

}  // namespace mln::platform::android
