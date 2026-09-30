// JNI entry points over tests/graphics for the Android device suite. Each one
// forwards to the function of the same name in mln_test_graphics.h.

#include <jni.h>
#include <stdint.h>

#include "mln_test_graphics.h"

JNIEXPORT jlong JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_create(
  JNIEnv* env, jobject self, jint backend
) {
  (void)env;
  (void)self;
  return (jlong)(uintptr_t)mln_test_graphics_create((uint32_t)backend);
}

// Returns the context's queue family index, then its pointers in the order of
// mln_test_graphics_context, or null when the lookup fails.
JNIEXPORT jlongArray JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_context(
  JNIEnv* env, jobject self, jlong graphics
) {
  (void)self;
  mln_test_graphics_context context;
  if (!mln_test_graphics_get_context(
        (mln_test_graphics*)(uintptr_t)graphics, &context
      )) {
    return NULL;
  }
  const jlong values[] = {
    (jlong)context.vulkan_queue_family_index,
    (jlong)(uintptr_t)context.metal_device,
    (jlong)(uintptr_t)context.vulkan_instance,
    (jlong)(uintptr_t)context.vulkan_physical_device,
    (jlong)(uintptr_t)context.vulkan_device,
    (jlong)(uintptr_t)context.vulkan_queue,
    (jlong)(uintptr_t)context.vulkan_get_instance_proc_addr,
    (jlong)(uintptr_t)context.vulkan_get_device_proc_addr,
    (jlong)(uintptr_t)context.egl_display,
    (jlong)(uintptr_t)context.egl_config,
    (jlong)(uintptr_t)context.egl_context,
  };
  const jsize count = (jsize)(sizeof(values) / sizeof(values[0]));
  jlongArray result = (*env)->NewLongArray(env, count);
  if (result != NULL) {
    (*env)->SetLongArrayRegion(env, result, 0, count, values);
  }
  return result;
}

JNIEXPORT jboolean JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_makeCurrent(
  JNIEnv* env, jobject self, jlong graphics
) {
  (void)env;
  (void)self;
  return mln_test_graphics_make_current((mln_test_graphics*)(uintptr_t)graphics)
           ? JNI_TRUE
           : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_lastError(
  JNIEnv* env, jobject self
) {
  (void)self;
  return (*env)->NewStringUTF(env, mln_test_graphics_last_error());
}
