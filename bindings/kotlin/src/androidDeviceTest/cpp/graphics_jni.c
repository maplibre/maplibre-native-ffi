#include <jni.h>
#include <stdint.h>

#include "mln_test_graphics.h"

JNIEXPORT jlongArray JNICALL
Java_org_maplibre_nativeffi_render_TestVulkanDriver_create(
  JNIEnv* env, jobject self
) {
  (void)self;
  mln_test_graphics* graphics =
    mln_test_graphics_create(MLN_TEST_GRAPHICS_BACKEND_VULKAN);
  if (!graphics) return NULL;
  mln_test_graphics_context context;
  if (!mln_test_graphics_get_context(graphics, &context)) {
    mln_test_graphics_destroy(graphics);
    return NULL;
  }
  const jlong values[] = {
    (jlong)(uintptr_t)graphics,
    (jlong)(uintptr_t)context.vulkan_instance,
    (jlong)(uintptr_t)context.vulkan_physical_device,
    (jlong)(uintptr_t)context.vulkan_device,
    (jlong)(uintptr_t)context.vulkan_queue,
    (jlong)context.vulkan_queue_family_index,
    (jlong)(uintptr_t)context.vulkan_get_instance_proc_addr,
    (jlong)(uintptr_t)context.vulkan_get_device_proc_addr,
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
