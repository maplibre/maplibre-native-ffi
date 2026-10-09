#include <memory>
#include <optional>
#include <string>

#include <mln/i18n/collator.hpp>

#include "platform/android/i18n.hpp"

namespace mln::platform {

class Collator::Impl {
 public:
  Impl(
    bool caseSensitive, bool diacriticSensitive,
    const std::optional<std::string>& locale
  ) {
    android::with_environment([&](JNIEnv& env) -> void {
      const auto resolved =
        android::resolve_collation_locale(env, locale.value_or(""));
      resolved_locale = android::locale_tag(env, resolved);
      const auto& cls = jni::Class<android::JavaCollator>::Singleton(env);
      static const auto get_instance = cls.GetStaticMethod<
        jni::Object<android::JavaCollator>(jni::Object<android::Locale>)>(
        env, "getInstance"
      );
      auto local = cls.Call(env, get_instance, resolved);
      static const auto set_strength =
        cls.GetMethod<void(jni::jint)>(env, "setStrength");
      static const auto primary = cls.GetStaticField<jni::jint>(env, "PRIMARY");
      static const auto secondary =
        cls.GetStaticField<jni::jint>(env, "SECONDARY");
      static const auto tertiary =
        cls.GetStaticField<jni::jint>(env, "TERTIARY");
      auto strength = cls.Get(env, primary);
      if (diacriticSensitive) {
        strength = cls.Get(env, caseSensitive ? tertiary : secondary);
      }
      local.Call(env, set_strength, strength);
      const auto& rule_cls =
        jni::Class<android::RuleBasedCollator>::Singleton(env);
      static const auto set_case_level =
        rule_cls.GetMethod<void(jni::jboolean)>(env, "setCaseLevel");
      const auto rules = jni::Cast(env, rule_cls, local);
      rules.Call(
        env, set_case_level,
        static_cast<jni::jboolean>(caseSensitive && !diacriticSensitive)
      );
      static const auto set_decomposition =
        cls.GetMethod<void(jni::jint)>(env, "setDecomposition");
      static const auto canonical =
        cls.GetStaticField<jni::jint>(env, "CANONICAL_DECOMPOSITION");
      local.Call(env, set_decomposition, cls.Get(env, canonical));
      static const auto freeze =
        cls.GetMethod<jni::Object<android::JavaCollator>()>(env, "freeze");
      local.Call(env, freeze);
      collator = jni::NewGlobal<jni::EnvAttachingDeleter>(env, local);
    });
  }

  [[nodiscard]] auto compare(
    const std::string& lhs, const std::string& rhs
  ) const -> int {
    return android::with_environment([&](JNIEnv& env) -> int {
      const auto& cls = jni::Class<android::JavaCollator>::Singleton(env);
      static const auto method =
        cls.GetMethod<jni::jint(jni::String, jni::String)>(env, "compare");
      return collator.Call(
        env, method, jni::Make<jni::String>(env, lhs),
        jni::Make<jni::String>(env, rhs)
      );
    });
  }

  auto operator==(const Impl& other) const -> bool {
    return android::with_environment([&](JNIEnv& env) -> bool {
      const auto& cls = jni::Class<android::JavaCollator>::Singleton(env);
      static const auto equals =
        cls.GetMethod<jni::jboolean(jni::Object<>)>(env, "equals");
      return resolved_locale == other.resolved_locale &&
             collator.Call(env, equals, other.collator) != JNI_FALSE;
    });
  }

  [[nodiscard]] auto resolvedLocale() const -> std::string {
    return resolved_locale;
  }

 private:
  jni::Global<jni::Object<android::JavaCollator>, jni::EnvAttachingDeleter>
    collator;
  std::string resolved_locale;
};

Collator::Collator(
  bool caseSensitive, bool diacriticSensitive,
  const std::optional<std::string>& locale
)
    : impl(std::make_shared<Impl>(caseSensitive, diacriticSensitive, locale)) {}

auto Collator::operator==(const Collator& other) const -> bool {
  return *impl == *other.impl;
}

auto Collator::compare(const std::string& lhs, const std::string& rhs) const
  -> int {
  return impl->compare(lhs, rhs);
}

auto Collator::resolvedLocale() const -> std::string {
  return impl->resolvedLocale();
}

}  // namespace mln::platform
