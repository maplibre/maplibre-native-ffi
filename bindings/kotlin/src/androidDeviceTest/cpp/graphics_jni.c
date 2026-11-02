// JNI entry points over tests/graphics for the Android device suite. Each one
// forwards to the function of the same name in mln_test_graphics.h, the
// texture ones to mln_test_graphics_texture_*, except the last two, which call
// a completion's function pointers as native would.

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

JNIEXPORT jlong JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_textureCreate(
  JNIEnv* env, jobject self, jlong graphics, jint width, jint height
) {
  (void)env;
  (void)self;
  return (jlong)(uintptr_t)mln_test_graphics_texture_create(
    (mln_test_graphics*)(uintptr_t)graphics, (uint32_t)width, (uint32_t)height
  );
}

// Returns the texture's fields in the order of mln_test_graphics_texture_info,
// or null when the lookup fails.
JNIEXPORT jlongArray JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_textureInfo(
  JNIEnv* env, jobject self, jlong texture
) {
  (void)self;
  mln_test_graphics_texture_info info;
  if (!mln_test_graphics_texture_get_info(
        (const mln_test_graphics_texture*)(uintptr_t)texture, &info
      )) {
    return NULL;
  }
  const jlong values[] = {
    (jlong)info.vulkan_image,
    (jlong)info.vulkan_image_view,
    (jlong)(uintptr_t)info.metal_texture,
    (jlong)info.width,
    (jlong)info.height,
    (jlong)info.format,
    (jlong)info.vulkan_final_layout,
    (jlong)info.opengl_texture,
    (jlong)info.opengl_target,
  };
  const jsize count = (jsize)(sizeof(values) / sizeof(values[0]));
  jlongArray result = (*env)->NewLongArray(env, count);
  if (result != NULL) {
    (*env)->SetLongArrayRegion(env, result, 0, count, values);
  }
  return result;
}

JNIEXPORT void JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_textureDestroy(
  JNIEnv* env, jobject self, jlong texture
) {
  (void)env;
  (void)self;
  mln_test_graphics_texture_destroy(
    (mln_test_graphics_texture*)(uintptr_t)texture
  );
}

JNIEXPORT jstring JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_lastError(
  JNIEnv* env, jobject self
) {
  (void)self;
  return (*env)->NewStringUTF(env, mln_test_graphics_last_error());
}

JNIEXPORT void JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_callCompletion(
  JNIEnv* env, jobject self, jlong callback, jlong user_data, jlong result
) {
  (void)env;
  (void)self;
  ((void (*)(void*, const void*))(uintptr_t)callback)(
    (void*)(uintptr_t)user_data, (const void*)(uintptr_t)result
  );
}

JNIEXPORT void JNICALL
Java_org_maplibre_nativeffi_render_TestGraphicsJni_callCompletionRelease(
  JNIEnv* env, jobject self, jlong release, jlong user_data
) {
  (void)env;
  (void)self;
  ((void (*)(void*))(uintptr_t)release)((void*)(uintptr_t)user_data);
}
