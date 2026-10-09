#include <cstdint>
#include <optional>
#include <string>

#include <mln/i18n/number_format.hpp>

#include "platform/android/i18n.hpp"

namespace mln::platform {

auto formatNumber(
  double number, const std::string& localeId, const std::string& currency,
  std::optional<uint8_t> minFractionDigits,
  std::optional<uint8_t> maxFractionDigits
) -> std::string {
  return android::with_environment([&](JNIEnv& env) -> std::string {
    const auto locale = android::locale_for_tag(env, localeId);
    const auto& cls = jni::Class<android::NumberFormat>::Singleton(env);
    static const auto decimal_method = cls.GetStaticMethod<
      jni::Object<android::NumberFormat>(jni::Object<android::Locale>)>(
      env, "getNumberInstance"
    );
    static const auto currency_method = cls.GetStaticMethod<
      jni::Object<android::NumberFormat>(jni::Object<android::Locale>)>(
      env, "getCurrencyInstance"
    );
    const auto formatter = cls.Call(
      env, currency.empty() ? decimal_method : currency_method, locale
    );

    if (!currency.empty()) {
      const auto& currency_cls = jni::Class<android::Currency>::Singleton(env);
      static const auto get_currency =
        currency_cls
          .GetStaticMethod<jni::Object<android::Currency>(jni::String)>(
            env, "getInstance"
          );
      static const auto set_currency =
        cls.GetMethod<void(jni::Object<android::Currency>)>(env, "setCurrency");
      formatter.Call(
        env, set_currency,
        currency_cls.Call(
          env, get_currency, jni::Make<jni::String>(env, currency)
        )
      );
    }

    static const auto set_min =
      cls.GetMethod<void(jni::jint)>(env, "setMinimumFractionDigits");
    static const auto set_max =
      cls.GetMethod<void(jni::jint)>(env, "setMaximumFractionDigits");
    if (minFractionDigits) {
      formatter.Call(env, set_min, static_cast<jni::jint>(*minFractionDigits));
    }
    if (maxFractionDigits) {
      formatter.Call(env, set_max, static_cast<jni::jint>(*maxFractionDigits));
    }

    static const auto format =
      cls.GetMethod<jni::String(jni::jdouble)>(env, "format");
    return jni::Make<std::string>(
      env, formatter.Call(env, format, static_cast<jni::jdouble>(number))
    );
  });
}

}  // namespace mln::platform
