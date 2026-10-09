#pragma once

#include <string>
#include <utility>

// The JNI component headers require the umbrella's declaration order.
// clang-format off
#include <jni/jni.hpp> // IWYU pragma: keep
// clang-format on

// IWYU pragma: begin_exports
#include <jni.h>
#include <jni/advanced_ownership.hpp>
#include <jni/array.hpp>
#include <jni/class.hpp>
#include <jni/errors.hpp>
#include <jni/functions.hpp>
#include <jni/make.hpp>
#include <jni/object.hpp>
#include <jni/ownership.hpp>
#include <jni/string.hpp>
#include <jni/types.hpp>
#include <jni/unique.hpp>
// IWYU pragma: end_exports

namespace mln::platform::android {

struct Locale {
  static constexpr auto Name() { return "android/icu/util/ULocale"; }
};

struct LocaleBuilder {
  static constexpr auto Name() { return "android/icu/util/ULocale$Builder"; }
};

struct JavaCollator {
  static constexpr auto Name() { return "android/icu/text/Collator"; }
};

struct RuleBasedCollator {
  static constexpr auto Name() { return "android/icu/text/RuleBasedCollator"; }
};

struct NumberFormat {
  static constexpr auto Name() { return "android/icu/text/NumberFormat"; }
};

struct Currency {
  static constexpr auto Name() { return "android/icu/util/Currency"; }
};

void initialize_i18n(JNIEnv& env);

void attach_thread();
void detach_thread();

auto attached_environment() -> jni::UniqueEnv;

[[noreturn]] void throw_java_error(JNIEnv& env);

template <typename Function>
auto with_environment(Function&& function) {
  const auto environment = attached_environment();
  auto& env = *environment;
  try {
    return std::forward<Function>(function)(env);
  } catch (const jni::PendingJavaException&) {
    throw_java_error(env);
  }
}

auto locale_for_tag(JNIEnv& env, const std::string& tag)
  -> jni::Local<jni::Object<Locale>>;
auto locale_tag(JNIEnv& env, const jni::Object<Locale>& locale) -> std::string;
auto resolve_collation_locale(JNIEnv& env, const std::string& tag)
  -> jni::Local<jni::Object<Locale>>;

}  // namespace mln::platform::android
