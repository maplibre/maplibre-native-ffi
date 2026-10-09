#include <atomic>
#include <stdexcept>
#include <string>

#include "platform/android/i18n.hpp"

namespace mln::platform::android {
namespace {

auto java_vm() -> std::atomic<JavaVM*>& {
  static std::atomic<JavaVM*> instance{nullptr};
  return instance;
}

auto thread_environment() -> jni::UniqueEnv& {
  thread_local jni::UniqueEnv instance;
  return instance;
}

auto default_locale(JNIEnv& env) -> jni::Local<jni::Object<Locale>> {
  const auto& cls = jni::Class<Locale>::Singleton(env);
  static const auto method =
    cls.GetStaticMethod<jni::Object<Locale>()>(env, "getDefault");
  return cls.Call(env, method);
}

}  // namespace

void initialize_i18n(JNIEnv& env) {
  try {
    jni::Class<Locale>::Singleton(env);
    jni::Class<JavaCollator>::Singleton(env);
    jni::Class<RuleBasedCollator>::Singleton(env);
    jni::Class<NumberFormat>::Singleton(env);
    jni::Class<Currency>::Singleton(env);
    java_vm().store(&jni::GetJavaVM(env), std::memory_order_release);
  } catch (const jni::PendingJavaException&) {
    throw_java_error(env);
  }
}

void attach_thread() {
  auto* jvm = java_vm().load(std::memory_order_acquire);
  auto& environment = thread_environment();
  if (jvm != nullptr && !environment) {
    environment = jni::GetAttachedEnv(*jvm);
  }
}

void detach_thread() { thread_environment().reset(); }

auto attached_environment() -> jni::UniqueEnv {
  auto* jvm = java_vm().load(std::memory_order_acquire);
  if (jvm == nullptr) {
    throw std::runtime_error(
      "Android platform services are not initialized; call mln_android_init"
    );
  }
  return jni::GetAttachedEnv(*jvm);
}

void throw_java_error(JNIEnv& env) {
  env.ExceptionClear();
  throw std::runtime_error("Android locale API call failed");
}

auto locale_for_tag(JNIEnv& env, const std::string& tag)
  -> jni::Local<jni::Object<Locale>> {
  if (tag.empty()) {
    return default_locale(env);
  }
  const auto& cls = jni::Class<Locale>::Singleton(env);
  static const auto method =
    cls.GetStaticMethod<jni::Object<Locale>(jni::String)>(
      env, "forLanguageTag"
    );
  return cls.Call(env, method, jni::Make<jni::String>(env, tag));
}

auto locale_tag(JNIEnv& env, const jni::Object<Locale>& locale) -> std::string {
  const auto& cls = jni::Class<Locale>::Singleton(env);
  static const auto method = cls.GetMethod<jni::String()>(env, "toLanguageTag");
  return jni::Make<std::string>(env, locale.Call(env, method));
}

auto resolve_collation_locale(JNIEnv& env, const std::string& tag)
  -> jni::Local<jni::Object<Locale>> {
  const auto& collator_cls = jni::Class<JavaCollator>::Singleton(env);
  static const auto available_method =
    collator_cls.GetStaticMethod<jni::Array<jni::Object<Locale>>()>(
      env, "getAvailableULocales"
    );
  const auto available = collator_cls.Call(env, available_method);
  const auto requested = locale_for_tag(env, tag);
  const auto& locale_cls = jni::Class<Locale>::Singleton(env);
  static const auto base_name =
    locale_cls.GetMethod<jni::String()>(env, "getBaseName");
  static const auto locale_constructor =
    locale_cls.GetConstructor<jni::String>(env);
  const auto base =
    locale_cls.New(env, locale_constructor, requested.Call(env, base_name));
  const auto fallback = default_locale(env);
  auto preferences = jni::Array<jni::Object<Locale>>::New(env, 2);
  preferences.Set(env, 0, base);
  preferences.Set(env, 1, fallback);
  static const auto resolve_method =
    locale_cls.GetStaticMethod<jni::Object<Locale>(
      jni::Array<jni::Object<Locale>>, jni::Array<jni::Object<Locale>>,
      jni::Array<jni::jboolean>
    )>(env, "acceptLanguage");
  const auto fallback_used = jni::Array<jni::jboolean>::New(env, 1);
  auto resolved =
    locale_cls.Call(env, resolve_method, preferences, available, fallback_used);
  if (!resolved) {
    static const auto root_field =
      locale_cls.GetStaticField<jni::Object<Locale>>(env, "ROOT");
    resolved = locale_cls.Get(env, root_field);
  }

  static const auto get_extension =
    locale_cls.GetMethod<jni::String(jni::jchar)>(env, "getExtension");
  constexpr auto unicode_key = static_cast<jni::jchar>('u');
  const auto extension = requested.Call(env, get_extension, unicode_key);
  if (!extension) {
    return resolved;
  }

  const auto& builder_cls = jni::Class<LocaleBuilder>::Singleton(env);
  static const auto builder_constructor = builder_cls.GetConstructor<>(env);
  const auto builder = builder_cls.New(env, builder_constructor);
  static const auto set_locale =
    builder_cls.GetMethod<jni::Object<LocaleBuilder>(jni::Object<Locale>)>(
      env, "setLocale"
    );
  static const auto set_extension =
    builder_cls.GetMethod<jni::Object<LocaleBuilder>(jni::jchar, jni::String)>(
      env, "setExtension"
    );
  static const auto build =
    builder_cls.GetMethod<jni::Object<Locale>()>(env, "build");
  builder.Call(env, set_locale, resolved);
  builder.Call(env, set_extension, unicode_key, extension);
  const auto candidate = builder.Call(env, build);

  const auto collation_key = jni::Make<jni::String>(env, "collation");
  static const auto equivalent_method =
    collator_cls
      .GetStaticMethod<jni::Object<Locale>(jni::String, jni::Object<Locale>)>(
        env, "getFunctionalEquivalent"
      );
  const auto equivalent =
    collator_cls.Call(env, equivalent_method, collation_key, candidate);
  static const auto get_keyword =
    locale_cls.GetMethod<jni::String(jni::String)>(env, "getKeywordValue");
  static const auto set_keyword =
    locale_cls.GetMethod<jni::Object<Locale>(jni::String, jni::String)>(
      env, "setKeywordValue"
    );
  return candidate.Call(
    env, set_keyword, collation_key,
    equivalent.Call(env, get_keyword, collation_key)
  );
}

}  // namespace mln::platform::android
