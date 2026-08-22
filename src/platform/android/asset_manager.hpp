#pragma once

struct AAssetManager;

namespace mln::platform {

// Retains the AssetManager of a borrowed JNI Context.
// Returns a static error string on failure, or nullptr on success or when
// retention already completed.
auto android_retain_asset_manager(void* jni_env, void* context) -> const char*;

auto android_asset_manager() noexcept -> AAssetManager*;

}  // namespace mln::platform
