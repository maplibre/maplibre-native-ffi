#include <atomic>
#include <mutex>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>

#include "platform/android/asset_manager.hpp"

namespace mln::platform {
namespace {

std::mutex mutex;
jobject asset_manager_global = nullptr;  // keeps AAssetManager_fromJava valid
std::atomic<AAssetManager*> asset_manager{nullptr};

auto jni_error(JNIEnv* env, const char* message) -> const char* {
  if (env->ExceptionCheck() != 0U) {
    env->ExceptionClear();
  }
  return message;
}

}  // namespace

auto android_retain_asset_manager(void* jni_env, void* context) -> const char* {
  if (jni_env == nullptr || context == nullptr) {
    return "jni_env and context must not be null";
  }
  if (asset_manager.load(std::memory_order_acquire) != nullptr) {
    return nullptr;
  }

  const std::scoped_lock lock(mutex);
  if (asset_manager.load(std::memory_order_relaxed) != nullptr) {
    return nullptr;
  }

  auto* env = static_cast<JNIEnv*>(jni_env);
  auto* context_object = static_cast<jobject>(context);

  auto* context_class = env->GetObjectClass(context_object);
  if (context_class == nullptr) {
    return jni_error(env, "Android Context class lookup failed");
  }

  auto get_assets = env->GetMethodID(
    context_class, "getAssets", "()Landroid/content/res/AssetManager;"
  );
  env->DeleteLocalRef(context_class);
  if (get_assets == nullptr) {
    return jni_error(env, "Android getAssets lookup failed");
  }

  auto* assets = env->CallObjectMethodA(context_object, get_assets, nullptr);
  if (env->ExceptionCheck() != 0U || assets == nullptr) {
    return jni_error(env, "Android AssetManager lookup failed");
  }

  auto* global = env->NewGlobalRef(assets);
  env->DeleteLocalRef(assets);
  if (global == nullptr) {
    return jni_error(env, "Android AssetManager global reference failed");
  }

  auto* native = AAssetManager_fromJava(env, global);
  if (native == nullptr) {
    env->DeleteGlobalRef(global);
    return "Android AAssetManager conversion failed";
  }

  asset_manager_global = global;
  asset_manager.store(native, std::memory_order_release);
  return nullptr;
}

auto android_asset_manager() noexcept -> AAssetManager* {
  return asset_manager.load(std::memory_order_acquire);
}

}  // namespace mln::platform
