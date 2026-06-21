#pragma once

#include "maplibre_native_c/android.h"

static inline mln_status mln_android_init_javacpp(
  void* jni_env, void* jni_class, void* context
) {
  (void)jni_class;
  return mln_android_init(jni_env, context);
}
