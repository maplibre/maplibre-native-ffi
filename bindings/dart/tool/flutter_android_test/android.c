#include <jni.h>
#include <maplibre_native_c.h>

// Initialization borrows JNIEnv on the Android UI thread before Dart starts.
JNIEXPORT jstring JNICALL
Java_org_maplibre_nativeffi_testhost_MainActivity_initialize(
  JNIEnv* env, jobject activity, jobject context
) {
  (void)activity;
  mln_diagnostic diagnostic = {.size = sizeof(mln_diagnostic)};
  if (mln_android_init(env, context, &diagnostic) == MLN_STATUS_OK) {
    return NULL;
  }
  return (*env)->NewStringUTF(env, diagnostic.message);
}
