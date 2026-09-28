#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>

#include "maplibre_native_c/wake.h"

namespace mln::core {

class Wake final {
 public:
  explicit Wake(const mln_wake& descriptor) noexcept;
  Wake(const Wake&) = delete;
  Wake(Wake&&) = delete;
  auto operator=(const Wake&) -> Wake& = delete;
  auto operator=(Wake&&) -> Wake& = delete;
  ~Wake();

  auto accept() noexcept -> void;
  auto reject() noexcept -> void;
  auto notify() noexcept -> void;

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  mln_wake descriptor_{};
  std::size_t in_flight_ = 0;
  bool accepted_ = false;
  bool closing_ = false;
};

auto validate_wake(const mln_wake* wake) -> mln_status;

}  // namespace mln::core
