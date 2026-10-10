#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include "completion/completion.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

// The value that a completion function delivers, as its header declares it:
// `type` is the C type that `result=` names, `array` reports `shape=array`, and
// `nullable` reports `nullable=true`. tools/bindgen specializes it for each
// function with a value result, so a function without one has no table entry.
template <auto Function>
struct CompletionResult;

#include "completion/completion_result_generated.inc"

template <auto Function>
class CompletionValue;

// The type-erased delivery behind CompletionValue, which alone may call it.
class ErasedCompletionValue final {
  template <auto Function>
  friend class CompletionValue;

  static auto deliver(
    const mln_completion& descriptor, const void* value, std::size_t count
  ) noexcept -> void;
};

// Delivers the successful result of the C function Function. The value type
// and shape come from the generated table, so a call site that passes another
// type, an array to a single-value result, or no value to a result that is not
// nullable fails to compile.
template <auto Function>
class CompletionValue final {
 public:
  using Type = typename CompletionResult<Function>::type;
  static constexpr bool array = CompletionResult<Function>::array;
  static constexpr bool nullable = CompletionResult<Function>::nullable;

  static auto deliver(
    const mln_completion& descriptor, const Type& value
  ) noexcept -> void
    requires(!array)
  {
    ErasedCompletionValue::deliver(descriptor, &value, 1);
  }

  static auto deliver(
    const mln_completion& descriptor, std::span<const Type> values
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
    ErasedCompletionValue::deliver(descriptor, data, values.size());
  }

  static auto deliver_absent(const mln_completion& descriptor) noexcept -> void
    requires(nullable)
  {
    ErasedCompletionValue::deliver(descriptor, nullptr, 0);
  }

  // Resolves completion with a value that points into no storage of its own.
  static auto complete(
    const std::shared_ptr<Completion>& completion, Type value
  ) noexcept -> void
    requires(!array)
  {
    completion->resolve([value](const mln_completion& descriptor) {
      deliver(descriptor, value);
    });
  }
};

}  // namespace mln::core
