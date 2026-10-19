#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

#include "completion/completion.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

// The generated table defines result types and shapes from header annotations.
// Commands have no table entry and complete through complete_command.
template <auto Function>
struct CompletionResult;

#include "completion/completion_result_generated.inc"

template <auto Function>
class CompletionValue;

class ErasedCompletionValue final {
  template <auto Function>
  friend class CompletionValue;
  friend class ValuelessCompletion;

  static auto deliver(
    const CompletionSink& sink, const void* value, std::size_t count,
    std::uint32_t size, const delivery::CompletionCapture* capture
  ) noexcept -> void;
};

// Delivers the successful result of the C function Function. The value type
// and shape come from the generated table, so a call site that passes another
// type, an array to a single-value result, or no value to a result that is not
// nullable fails to compile.
template <auto Function>
class CompletionValue final {
  static_assert(
    CompletionResult<Function>::has_value,
    "this function delivers no value; complete it through ValuelessCompletion"
  );

 public:
  using Type = typename CompletionResult<Function>::type;
  static constexpr std::uint32_t size = sizeof(Type);
  static constexpr bool array = CompletionResult<Function>::array;
  static constexpr bool nullable = CompletionResult<Function>::nullable;
  // How a queue copies and disposes the value. A value type without a
  // generated capture fails to compile here.
  static constexpr const delivery::CompletionCapture* capture =
    &delivery::completion_capture<Type>;

  static auto deliver(const CompletionSink& sink, const Type& value) noexcept
    -> void
    requires(!array)
  {
    ErasedCompletionValue::deliver(sink, &value, 1, size, capture);
  }

  static auto deliver(
    const CompletionSink& sink, std::span<const Type> values
  ) noexcept -> void
    requires(array)
  {
    const auto* data = values.data();
    if constexpr (nullable) {
      // A null value reports an absent array, so a present empty one points
      // at storage it never reads.
      static const Type empty{};
      if (data == nullptr) data = &empty;
    }
    ErasedCompletionValue::deliver(sink, data, values.size(), size, capture);
  }

  static auto deliver_absent(const CompletionSink& sink) noexcept -> void
    requires(nullable)
  {
    ErasedCompletionValue::deliver(sink, nullptr, 0, 0, capture);
  }

  static auto complete(
    const std::shared_ptr<Completion>& completion, Type value
  ) noexcept -> void
    requires(!array)
  {
    completion->resolve([value](const CompletionSink& sink) {
      deliver(sink, value);
    });
  }
};

class ValuelessCompletion;

template <auto Function>
constexpr auto valueless_completion() noexcept -> ValuelessCompletion;

// Completes a function whose header declares no value, delivering any status
// with a null value. Only valueless_completion<&function>() makes one, and only
// for such a function, so code shared by several functions takes one from each
// entry point rather than completing without a value on its own.
class ValuelessCompletion final {
 public:
  auto deliver(
    const CompletionSink& sink, mln_status status = MLN_STATUS_OK,
    const std::string& diagnostic = {}
  ) const noexcept -> void;

  auto complete(
    const std::shared_ptr<Completion>& completion,
    mln_status status = MLN_STATUS_OK, std::string diagnostic = {}
  ) const noexcept -> void;

 private:
  constexpr ValuelessCompletion() noexcept = default;

  template <auto Function>
  friend constexpr auto valueless_completion() noexcept -> ValuelessCompletion;
};

template <auto Function>
constexpr auto valueless_completion() noexcept -> ValuelessCompletion {
  static_assert(
    !CompletionResult<Function>::has_value,
    "this function delivers a value; complete it through CompletionValue"
  );
  return ValuelessCompletion{};
}

}  // namespace mln::core
