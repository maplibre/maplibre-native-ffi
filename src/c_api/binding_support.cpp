#define MLN_BUILDING_C

#include <cstdint>
#include <cstdio>
#include <memory>

#include "maplibre_native_c/binding_support.h"

#include "c_api/boundary.hpp"
#include "delivery/capture.hpp"
#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

namespace {

// Dart native API major version 2 fixes this integer-message prefix. Only
// kInt64 is used, so the larger union alternatives are never accessed.
struct DartIntegerMessage {
  std::int32_t type = 3;
  union {
    std::int64_t value = 0;
    std::uintptr_t reserved[5];
  };
};

struct DartPort {
  using Post = bool (*)(std::int64_t, DartIntegerMessage*);
  Post post;
  std::int64_t port;

  void notify(std::int64_t value) const noexcept {
    auto message = DartIntegerMessage{3, {value}};
    static_cast<void>(post(port, &message));
  }
};

struct OwnerToken {
  std::uint64_t handle = 0;
};

auto dispose_handle(std::uint64_t handle) noexcept -> mln_status {
  return mln::delivery::dispose_owner(
    static_cast<mln::core::HandleKind>(mln::core::handle_kind_of(handle)),
    handle
  );
}

}  // namespace

extern "C" MLN_API auto mln_binding_owner_token_create(
  std::uint64_t handle
) noexcept -> void* {
  try {
    return new OwnerToken{handle};
  } catch (...) {
    static_cast<void>(dispose_handle(handle));
    return nullptr;
  }
}

extern "C" MLN_API void mln_binding_owner_token_destroy(void* token) noexcept {
  delete static_cast<OwnerToken*>(token);
}

extern "C" MLN_API void mln_binding_owner_finalize(void* token) noexcept {
  const auto owner =
    std::unique_ptr<OwnerToken>{static_cast<OwnerToken*>(token)};
  if (owner == nullptr) return;
  const auto kind = mln::core::handle_kind_of(owner->handle);
  // An isolate's shutdown finalizes its owners while the process exits, and
  // nothing waits for a session's wake releases then, so a finalized session
  // quarantines rather than detaching through graphics calls.
  const auto status =
    kind == static_cast<std::uint8_t>(mln::core::HandleKind::RenderSession)
      ? mln::c_api::status_boundary(
          nullptr,
          [&] {
            return mln::core::render_session_dispose_quarantined(
              static_cast<mln_render_session>(owner->handle)
            );
          }
        )
      : dispose_handle(owner->handle);
  if (status != MLN_STATUS_OK) {
    static_cast<void>(std::fprintf(
      stderr,
      "maplibre_native_ffi: owner finalization failed for handle %llu "
      "(status %d)\n",
      static_cast<unsigned long long>(owner->handle), static_cast<int>(status)
    ));
  }
}

extern "C" MLN_API auto mln_binding_render_session_abandon_at_exit(
  mln_render_session session, mln_render_abandon_result* out_result,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_abandon_keeping_graphics(
      session, out_result
    );
  });
}

extern "C" MLN_API auto mln_binding_dart_wake_create(
  void* post_cobject, std::int64_t port, mln_wake* out_wake,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (post_cobject == nullptr || port == 0 || out_wake == nullptr) {
      mln::core::set_thread_error(
        "Dart wake requires a posting function, port and output"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto state = std::make_unique<DartPort>(
      DartPort{reinterpret_cast<DartPort::Post>(post_cobject), port}
    );
    *out_wake = {};
    out_wake->callback = [](void* context) {
      static_cast<DartPort*>(context)->notify(0);
    };
    out_wake->release_user_data = [](void* context) {
      const auto state =
        std::unique_ptr<DartPort>{static_cast<DartPort*>(context)};
      state->notify(1);
    };
    out_wake->user_data = state.release();
    return MLN_STATUS_OK;
  });
}
