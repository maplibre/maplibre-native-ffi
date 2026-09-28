#include <jni.h>
#include <stdint.h>

#include "GraphicsSupport.h"

JNIEXPORT jlongArray JNICALL
Java_org_maplibre_nativeffi_render_TestVulkanDriver_create(
  JNIEnv* env, jobject self
) {
  (void)self;
  mln_test_graphics* graphics = mln_test_graphics_create(true);
  if (!graphics) return NULL;
  const jlong values[] = {
    (jlong)(uintptr_t)graphics,
    (jlong)(uintptr_t)graphics->instance,
    (jlong)(uintptr_t)graphics->physical_device,
    (jlong)(uintptr_t)graphics->device,
    (jlong)(uintptr_t)graphics->queue,
    (jlong)graphics->queue_family,
    (jlong)(uintptr_t)graphics->get_instance_proc_addr,
    (jlong)(uintptr_t)graphics->get_device_proc_addr,
  };
  jlongArray result = (*env)->NewLongArray(env, 8);
  if (!result) {
    mln_test_graphics_destroy(graphics);
    return NULL;
  }
  (*env)->SetLongArrayRegion(env, result, 0, 8, values);
  return result;
}

JNIEXPORT void JNICALL
Java_org_maplibre_nativeffi_render_TestVulkanDriver_destroy(
  JNIEnv* env, jobject self, jlong graphics
) {
  (void)env;
  (void)self;
  mln_test_graphics_destroy((mln_test_graphics*)(uintptr_t)graphics);
}
