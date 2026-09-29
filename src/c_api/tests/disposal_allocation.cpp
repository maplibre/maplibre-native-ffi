#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <thread>

#include "c_api/test_hooks.hpp"
#include "completion/completion.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "maplibre_native_c/callback_adapter.h"
#include "render/render_session_common.hpp"
#include "runtime/runtime.hpp"

// Static linkage makes these replacements cover the library's C++ allocation
// calls. Failure is local to the submitting thread; native cleanup may
// allocate.
namespace {
thread_local bool reject_allocations = false;
}

void* operator new(std::size_t size) {
  if (reject_allocations) throw std::bad_alloc{};
  if (auto* allocation = std::malloc(size == 0 ? 1 : size)) return allocation;
  throw std::bad_alloc{};
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }
void operator delete(void* allocation, std::size_t) noexcept {
  std::free(allocation);
}
void operator delete[](void* allocation, std::size_t) noexcept {
  std::free(allocation);
}

namespace {
void require(bool success, const char* message) {
  if (!success) {
    std::fprintf(stderr, "%s\n", message);
    std::abort();
  }
}

template <typename Predicate>
void wait(Predicate predicate) {
  const auto deadline =
    std::chrono::steady_clock::now() + std::chrono::seconds{10};
  while (!predicate()) {
    require(
      std::chrono::steady_clock::now() < deadline, "retirement timed out"
    );
    std::this_thread::yield();
  }
}

struct Result {
  std::atomic_int status{MLN_STATUS_INVALID_STATE};
  std::atomic_uint releases{0};
  std::atomic_uint64_t handle{MLN_HANDLE_NULL};
};

mln_completion descriptor(Result& result) {
  return {
    .size = sizeof(mln_completion),
    .callback =
      [](void* context, const mln_completion_result* completion) {
        auto& result = *static_cast<Result*>(context);
        result.status = completion->status;
        if (completion->value != nullptr)
          result.handle = *static_cast<const mln_map*>(completion->value);
      },
    .user_data = &result,
    .release_user_data =
      [](void* context) { ++static_cast<Result*>(context)->releases; },
  };
}

template <typename Dispose>
void without_allocations(Dispose dispose) {
  reject_allocations = true;
  // Prove that the interceptor is active before testing native admission.
  try {
    auto* unexpected = ::operator new(1);
    ::operator delete(unexpected);
    require(false, "allocation fault injection was inactive");
  } catch (const std::bad_alloc&) {
  }
  const auto status = dispose();
  reject_allocations = false;
  require(
    status == MLN_STATUS_OK, "disposal rejected with allocations disabled"
  );
}

mln_runtime create_runtime(std::atomic_uint* releases = nullptr) {
  auto options = mln_runtime_options_default();
  if (releases != nullptr) {
    options.event_wake.callback = [](void*) {};
    options.event_wake.user_data = releases;
    options.event_wake.release_user_data = [](void* context) {
      ++*static_cast<std::atomic_uint*>(context);
    };
  }
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  require(
    mln_runtime_create(&options, &runtime, nullptr) == MLN_STATUS_OK,
    "runtime creation failed"
  );
  return runtime;
}

void runtime_barriers_observe_retired_command_captures() {
  struct Probe {
    std::atomic_bool retired = false;
    std::atomic_bool observed_retirement = false;
    std::atomic_bool barrier_released = false;
  } probe;
  struct Capture {
    Probe& probe;
    ~Capture() { probe.retired.store(true); }
  };
  auto runtime = create_runtime();
  auto live = mln::core::lease_runtime(runtime);
  auto result = Result{};
  auto completion = std::make_shared<mln::core::Completion>(descriptor(result));
  auto capture = std::shared_ptr<Capture>(new Capture{probe});
  require(
    mln::core::submit_runtime_command(
      live,
      [runtime, completion, capture = std::move(capture), &probe](uint64_t) {
        const mln_completion barrier = {
          .size = sizeof(mln_completion),
          .callback =
            [](void* context, const mln_completion_result* value) {
              auto& probe = *static_cast<Probe*>(context);
              probe.observed_retirement.store(
                value->status == MLN_STATUS_OK && probe.retired.load()
              );
            },
          .user_data = &probe,
          .release_user_data =
            [](void* context) {
              static_cast<Probe*>(context)->barrier_released.store(true);
            },
        };
        require(
          mln_runtime_barrier(runtime, &barrier, nullptr) == MLN_STATUS_OK,
          "nested runtime barrier was rejected"
        );
        mln::core::complete(completion, MLN_STATUS_OK);
      },
      completion
    ) == MLN_STATUS_OK,
    "capture-retirement command was rejected"
  );
  wait([&] { return probe.barrier_released.load(); });
  require(
    probe.observed_retirement.load(),
    "runtime barrier completed before command captures retired"
  );
  wait([&] { return result.releases.load() == 1; });
  live.reset();
  auto closed = Result{};
  const auto close = descriptor(closed);
  require(
    mln_runtime_release(runtime, &close, nullptr) == MLN_STATUS_OK,
    "capture-retirement runtime close was rejected"
  );
  wait([&] { return closed.releases.load() == 1; });
}

mln_map create_map(mln_runtime runtime) {
  auto result = Result{};
  const auto completion = descriptor(result);
  auto options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  require(
    mln_map_create(runtime, &options, &completion, nullptr) == MLN_STATUS_OK,
    "map creation failed"
  );
  wait([&] { return result.releases.load() == 1; });
  require(result.status == MLN_STATUS_OK, "map completion failed");
  return result.handle;
}

void unclaimed_creation_disposes_on_the_callback_thread() {
  std::atomic_uint wake_releases = 0;
  const auto runtime = create_runtime(&wake_releases);
  auto runtime_weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto result = Result{};
  auto completion = descriptor(result);
  completion.callback = [](void* context, const mln_completion_result* value) {
    auto& result = *static_cast<Result*>(context);
    result.status = value->status;
    require(value->status == MLN_STATUS_OK, "unclaimed creation failed");
    const auto map = *static_cast<const mln_map*>(value->value);
    const auto discarded = mln_completion{
      .size = sizeof(mln_completion),
      .callback = [](void*, const mln_completion_result*) {},
    };
    reject_allocations = true;
    const auto release_status = mln_map_release(map, &discarded, nullptr);
    reject_allocations = false;
    require(
      release_status == MLN_STATUS_NATIVE_ERROR,
      "the native allocator bypassed fault injection"
    );
    without_allocations([&] { return mln_map_dispose(map, nullptr); });
  };
  require(
    mln_map_create(runtime, nullptr, &completion, nullptr) == MLN_STATUS_OK,
    "creation admission failed"
  );
  wait([&] { return result.releases.load() == 1; });
  without_allocations([&] { return mln_runtime_dispose(runtime, nullptr); });
  wait([&] { return runtime_weak.expired() && wake_releases.load() == 1; });
}

void disposal_drains_queued_work_and_map_cleanup() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto live = mln::core::lease_runtime(runtime);
  auto runtime_weak = std::weak_ptr{live};
  auto rejected = Result{};
  const auto rejected_completion = descriptor(rejected);
  require(
    mln_runtime_release(runtime, &rejected_completion, nullptr) ==
      MLN_STATUS_INVALID_STATE,
    "explicit close lost child preflight"
  );
  std::atomic_bool worker_entered = false;
  std::atomic_bool worker_release = false;
  std::atomic_bool cleanup_entered = false;
  std::atomic_bool cleanup_release = false;
  require(
    mln_test_hook_block_map_cleanup(map, &cleanup_entered, &cleanup_release) ==
      MLN_STATUS_OK,
    "cleanup hook failed"
  );
  live->executor.invoke([&] {
    worker_entered = true;
    wait([&] { return worker_release.load(); });
  });
  wait([&] { return worker_entered.load(); });
  auto resize = Result{};
  auto still = Result{};
  const auto resize_completion = descriptor(resize);
  const auto still_completion = descriptor(still);
  require(
    mln_map_resize(map, {300, 200, 1.0}, &resize_completion, nullptr) ==
      MLN_STATUS_OK,
    "resize admission failed"
  );
  require(
    mln_map_request_still_image(map, &still_completion, nullptr) ==
      MLN_STATUS_OK,
    "still-image admission failed"
  );
  without_allocations([&] { return mln_runtime_dispose(runtime, nullptr); });
  require(
    !mln::core::lease_runtime(runtime), "disposed parent is publicly live"
  );
  without_allocations([&] { return mln_map_dispose(map, nullptr); });
  worker_release = true;
  live.reset();
  wait([&] { return cleanup_entered.load(); });
  require(!runtime_weak.expired(), "runtime retired before map cleanup");
  cleanup_release = true;
  wait([&] { return runtime_weak.expired(); });
  require(
    resize.releases == 1 && resize.status == MLN_STATUS_OK,
    "queued command did not finish"
  );
  require(
    still.releases == 1 && still.status == MLN_STATUS_CANCELLED,
    "queued still image did not cancel"
  );
}
void failed_creation_releases_parent_reservation() {
  const auto runtime = create_runtime();
  auto weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto result = Result{};
  const auto completion = descriptor(result);
  reject_allocations = true;
  const auto status = mln_map_create(runtime, nullptr, &completion, nullptr);
  reject_allocations = false;
  require(
    status == MLN_STATUS_NATIVE_ERROR, "creation allocation did not fail"
  );
  require(result.releases == 0, "rejected creation retained completion");
  without_allocations([&] { return mln_runtime_dispose(runtime, nullptr); });
  wait([&] { return weak.expired(); });
}

void failed_finalizer_token_creation_disposes_the_owner() {
  const auto runtime = create_runtime();
  auto weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  reject_allocations = true;
  auto* token = mln_adapter_owner_token_create(runtime);
  reject_allocations = false;
  require(token == nullptr, "token allocation failure was not injected");
  wait([&] { return weak.expired(); });
}

void disposed_parent_allows_observed_child_release() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  without_allocations([&] { return mln_runtime_dispose(runtime, nullptr); });
  auto result = Result{};
  const auto completion = descriptor(result);
  require(
    mln_map_release(map, &completion, nullptr) == MLN_STATUS_OK,
    "disposed parent rejected child release"
  );
  wait([&] { return weak.expired() && result.releases == 1; });
  require(result.status == MLN_STATUS_OK, "child release failed");
}

void disposal_waits_for_pending_child_creation() {
  const auto runtime = create_runtime();
  auto live = mln::core::lease_runtime(runtime);
  auto weak = std::weak_ptr{live};
  std::atomic_bool entered = false;
  std::atomic_bool release = false;
  live->executor.invoke([&] {
    entered = true;
    wait([&] { return release.load(); });
  });
  wait([&] { return entered.load(); });
  auto result = Result{};
  const auto completion = descriptor(result);
  require(
    mln_map_create(runtime, nullptr, &completion, nullptr) == MLN_STATUS_OK,
    "pending creation admission failed"
  );
  without_allocations([&] { return mln_runtime_dispose(runtime, nullptr); });
  live.reset();
  require(!weak.expired(), "parent retired before accepted child creation");
  release = true;
  wait([&] { return weak.expired() && result.releases == 1; });
  require(
    result.status == MLN_STATUS_INVALID_ARGUMENT,
    "queued creation did not reject disposed parent"
  );
}

void a_pending_operation_keeps_only_its_runtime_alive() {
  const auto pending_runtime = create_runtime();
  const auto ready_runtime = create_runtime();
  auto pending_weak = std::weak_ptr{mln::core::lease_runtime(pending_runtime)};
  auto ready_weak = std::weak_ptr{mln::core::lease_runtime(ready_runtime)};
  auto result = Result{};
  auto completion = descriptor(result);
  std::atomic_bool entered = false;
  void* operation = nullptr;
  require(
    mln_test_hook_enqueue_pending_runtime_operation(
      pending_runtime, &entered, &operation, &completion
    ) == MLN_STATUS_OK,
    "pending operation admission failed"
  );
  wait([&] { return entered.load(); });
  without_allocations([&] {
    return mln_runtime_dispose(pending_runtime, nullptr);
  });
  without_allocations([&] {
    return mln_runtime_dispose(ready_runtime, nullptr);
  });
  wait([&] { return ready_weak.expired(); });
  require(
    !pending_weak.expired(), "runtime retired before its pending operation"
  );
  mln_test_hook_complete_runtime_operation(operation);
  wait([&] { return pending_weak.expired(); });
  require(
    result.releases == 1 && result.status == MLN_STATUS_OK,
    "pending operation did not finish before retirement"
  );
}

void disposal_retires_an_attached_graph_after_driver_quiescence() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto runtime_weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto map_weak =
    std::weak_ptr{mln::core::handle_table<mln::core::MapObject>().lease(map)};
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  auto options = mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  auto capabilities = mln_render_session_capabilities{};
  capabilities.size = sizeof(capabilities);
  auto attach = Result{};
  const auto completion = descriptor(attach);
  auto handle = mln_render_session{MLN_HANDLE_NULL};
  require(
    mln::core::start_attach_render_session(
      session, mln::core::RenderSessionKind::Surface, &options, capabilities,
      &handle, &completion
    ) == MLN_STATUS_OK,
    "session admission failed"
  );
  wait([&] { return attach.releases.load() == 1; });
  require(attach.status == MLN_STATUS_OK, "session attachment failed");
  auto weak = std::weak_ptr{session};
  std::atomic_bool entered = false;
  std::atomic_bool release = false;
  auto blocked = Result{};
  const auto blocked_completion = descriptor(blocked);
  require(
    mln::core::enqueue_blocking_test_render_operation(
      handle, &entered, &release, &blocked_completion
    ) == MLN_STATUS_OK,
    "driver operation rejected"
  );
  wait([&] { return entered.load(); });
  auto* runtime_token = mln_adapter_owner_token_create(runtime);
  auto* map_token = mln_adapter_owner_token_create(map);
  auto* session_token = mln_adapter_owner_token_create(handle);
  require(
    runtime_token && map_token && session_token, "owner token allocation failed"
  );
  without_allocations([&] {
    mln_adapter_owner_finalize(runtime_token);
    mln_adapter_owner_finalize(map_token);
    mln_adapter_owner_finalize(session_token);
    return MLN_STATUS_OK;
  });
  require(
    !mln::core::lease_render_session(handle),
    "disposed session remained callable"
  );
  require(!map_weak.expired(), "map retired while its driver was active");
  require(
    !runtime_weak.expired(), "runtime retired while its driver was active"
  );
  session.reset();
  release = true;
  wait([&] {
    return weak.expired() && map_weak.expired() && runtime_weak.expired();
  });
  require(
    blocked.releases == 1 && blocked.status == MLN_STATUS_OK,
    "in-flight driver completion did not finish exactly once"
  );
}

void abandoned_frame_preserves_its_session_owner_without_synthesizing_gpu_sync() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto runtime_weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  session->self = mln::core::register_render_session(session);
  session->state = MLN_RENDER_SESSION_STATE_ATTACHED;
  session->attached = true;
  session->acquired_frame_count = 1;
  require(
    mln::core::map_attach_render_target_session(map, session.get()) ==
      MLN_STATUS_OK,
    "frame test attachment failed"
  );
  auto frame = std::make_shared<mln_acquired_frame_object>();
  frame->session = session;
  const auto handle =
    mln::core::handle_table<mln_acquired_frame_object>().insert(frame);
  auto session_weak = std::weak_ptr{session};
  auto frame_weak = std::weak_ptr{frame};
  without_allocations([&] { return mln_runtime_dispose(runtime, nullptr); });
  without_allocations([&] { return mln_map_dispose(map, nullptr); });
  without_allocations([&] {
    return mln_acquired_frame_dispose(handle, nullptr);
  });
  require(
    static_cast<bool>(mln::core::lease_render_session(session->self)),
    "frame disposal consumed its independent session owner"
  );
  without_allocations([&] {
    return mln_render_session_dispose(session->self, nullptr);
  });
  frame.reset();
  session.reset();
  wait([&] {
    return frame_weak.expired() && session_weak.expired() &&
           runtime_weak.expired();
  });
}

void borrowed_views_hold_the_session_through_sibling_disposal() {
  const auto runtime = create_runtime();
  const auto map = create_map(runtime);
  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  session->self = mln::core::register_render_session(session);
  session->state = MLN_RENDER_SESSION_STATE_ATTACHED;
  session->attached = true;
  session->acquired_frame_count = 2;
  require(
    mln::core::map_attach_render_target_session(map, session.get()) ==
      MLN_STATUS_OK,
    "view test attachment failed"
  );
  auto frame = std::make_shared<mln_acquired_frame_object>();
  frame->session = session;
  auto frame_id =
    mln::core::handle_table<mln_acquired_frame_object>().insert(frame);
  auto sibling = std::make_shared<mln_acquired_frame_object>();
  sibling->session = session;
  const auto sibling_id =
    mln::core::handle_table<mln_acquired_frame_object>().insert(sibling);
  auto reentrant_destroy_status = std::atomic_int{MLN_STATUS_OK};
  session->driver_work.push_back(
    mln::core::RenderDriverWork{
      .execute = {},
      .abandon = [id = session->self, &reentrant_destroy_status] {
        reentrant_destroy_status = mln_render_session_destroy(id, nullptr);
      },
    }
  );
  void* scope = nullptr;
  without_allocations([&] {
    return mln_adapter_acquired_frame_view_begin(frame_id, &scope, nullptr);
  });
  auto result =
    mln_render_abandon_result{sizeof(mln_render_abandon_result), 0, 0, 0};
  require(
    mln_render_session_abandon(session->self, &result, nullptr) ==
      MLN_STATUS_BUSY,
    "abandon ignored a live view"
  );
  auto sync = mln_gpu_sync_default();
  require(
    mln_acquired_frame_release(&frame_id, &sync, nullptr) == MLN_STATUS_BUSY,
    "release ignored a live view"
  );
  without_allocations([&] { return mln_map_dispose(map, nullptr); });
  without_allocations([&] { return mln_runtime_dispose(runtime, nullptr); });
  without_allocations([&] {
    return mln_acquired_frame_dispose(sibling_id, nullptr);
  });
  void* rejected = nullptr;
  require(
    mln_adapter_acquired_frame_view_begin(frame_id, &rejected, nullptr) ==
        MLN_STATUS_TARGET_LOST &&
      rejected == nullptr,
    "sibling disposal allowed a later view"
  );
  {
    const auto lock = std::scoped_lock{session->control_mutex};
    require(
      session->state == MLN_RENDER_SESSION_STATE_ATTACHED,
      "disposal retired an active view"
    );
  }
  without_allocations([&] {
    mln_adapter_acquired_frame_view_end(scope);
    return MLN_STATUS_OK;
  });
  require(
    mln_acquired_frame_release(&frame_id, &sync, nullptr) == MLN_STATUS_OK,
    "released view did not release its frame"
  );
  without_allocations([&] {
    return mln_render_session_destroy(session->self, nullptr);
  });
  require(
    reentrant_destroy_status == MLN_STATUS_BUSY,
    "abandonment callback waited for its own retirement"
  );
}

void undelivered_dart_completion_disposes_its_owned_result() {
  struct Message {
    std::int32_t type;
    union {
      std::int64_t integer;
      struct {
        std::intptr_t count;
        Message** values;
      } array;
      struct {
        std::intptr_t pointer;
        std::intptr_t size;
        void (*finalize)(void*, void*);
      } native_pointer;
    };
  };
  static auto deliveries = std::atomic_uint{};
  const auto post = +[](std::int64_t port, Message* message) -> bool {
    require(
      port == 23 && message->type == 6 && message->array.count == 2,
      "completion port message shape changed"
    );
    require(
      message->array.values[0]->integer == 31, "completion token changed"
    );
    const auto& payload = *message->array.values[1];
    require(
      payload.type == 11 && payload.native_pointer.pointer != 0,
      "completion message lost its native finalizer"
    );
    without_allocations([&] {
      payload.native_pointer.finalize(
        nullptr, reinterpret_cast<void*>(payload.native_pointer.pointer)
      );
      return MLN_STATUS_OK;
    });
    ++deliveries;
    return false;
  };
  auto releases = std::atomic_uint{};
  const auto runtime = create_runtime(&releases);
  auto weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto completion = mln_completion{};
  require(
    mln_adapter_dart_completion_create(
      MLN_ADAPTER_COMPLETION_COPY_MAP, sizeof(mln_map),
      reinterpret_cast<void*>(post), 23, 31, &completion, nullptr
    ) == MLN_STATUS_OK,
    "Dart completion creation failed"
  );
  require(
    mln_map_create(runtime, nullptr, &completion, nullptr) == MLN_STATUS_OK,
    "Dart completion map creation failed"
  );
  wait([&] { return deliveries.load() == 1; });
  without_allocations([&] { return mln_runtime_dispose(runtime, nullptr); });
  wait([&] { return weak.expired() && releases.load() == 1; });
}

void dart_notification_ports_copy_records_before_retirement() {
  struct Message {
    std::int32_t type;
    union {
      std::int64_t integer;
      struct {
        std::intptr_t count;
        Message** values;
      } array;
    };
  };
  static auto deliveries = unsigned{};
  const auto post = +[](std::int64_t port, Message* message) -> bool {
    require(port == 19, "unexpected notification port");
    if (deliveries++ == 0) {
      require(
        message->type == 6 && message->array.count == 4,
        "tile notification shape changed"
      );
      require(
        message->array.values[0]->integer ==
            MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE &&
          message->array.values[1]->integer == 5 &&
          message->array.values[2]->integer == 7 &&
          message->array.values[3]->integer == 9,
        "tile notification lost its copied values"
      );
    } else {
      require(
        message->type == 3 && message->integer == 0,
        "notification port did not retire"
      );
    }
    return false;
  };
  auto* context =
    mln_adapter_dart_port_create(reinterpret_cast<void*>(post), 19);
  require(context != nullptr, "notification port creation failed");
  const auto callback =
    reinterpret_cast<mln_custom_geometry_source_tile_callback>(
      mln_adapter_dart_port_function(
        MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE
      )
    );
  without_allocations([&] {
    callback(context, {5, 7, 9});
    mln_adapter_dart_port_release(context);
    mln_adapter_dart_port_release(context);
    callback(context, {0, 0, 0});
    return MLN_STATUS_OK;
  });
  require(
    deliveries == 2, "retired notification port accepted another callback"
  );
}

void dart_deferred_callbacks_post_records_before_retirement() {
  struct Message {
    std::int32_t type;
    union {
      std::int64_t integer;
      struct {
        std::intptr_t count;
        Message** values;
      } array;
      struct {
        std::intptr_t pointer;
        std::intptr_t size;
        void (*finalize)(void*, void*);
      } native_pointer;
    };
  };
  static auto deliveries = unsigned{};
  const auto post = +[](std::int64_t port, Message* message) -> bool {
    require(port == 29, "unexpected deferred callback port");
    if (deliveries++ == 0) {
      require(
        message->type == 6 && message->array.count == 2 &&
          message->array.values[0]->integer ==
            MLN_ADAPTER_DEFERRED_LOG_CALLBACK,
        "deferred callback message shape changed"
      );
      const auto& payload = *message->array.values[1];
      require(
        payload.type == 11 && payload.native_pointer.pointer != 0,
        "deferred record lost its native finalizer"
      );
      const auto* record = reinterpret_cast<mln_adapter_deferred_call_record*>(
        payload.native_pointer.pointer
      );
      const auto* arguments =
        static_cast<const mln_adapter_log_callback_arguments*>(
          record->arguments
        );
      require(
        arguments->code == 7 &&
          std::strcmp(arguments->message, "deferred") == 0,
        "deferred record lost its copied arguments"
      );
      // A closed port finalizes the undelivered record.
      payload.native_pointer.finalize(
        nullptr, reinterpret_cast<void*>(payload.native_pointer.pointer)
      );
    } else {
      require(
        message->type == 3 && message->integer == 0,
        "deferred callback context did not retire"
      );
    }
    return false;
  };
  void* context = nullptr;
  require(
    mln_adapter_dart_deferred_callback_create(
      MLN_ADAPTER_DEFERRED_LOG_CALLBACK, reinterpret_cast<void*>(post), 29,
      &context, nullptr
    ) == MLN_STATUS_OK,
    "Dart deferred callback creation failed"
  );
  auto* address =
    mln_adapter_deferred_callback_function(MLN_ADAPTER_DEFERRED_LOG_CALLBACK);
  const auto callback = reinterpret_cast<mln_log_callback>(address);
  require(
    callback(
      context, MLN_LOG_SEVERITY_INFO, MLN_LOG_EVENT_GENERAL, 7, "deferred"
    ) == 1,
    "deferred log callback did not consume its record"
  );
  mln_adapter_deferred_callback_release(context);
  require(deliveries == 2, "deferred callback did not post and retire");
}

void dart_ports_release_after_the_host_closes() {
  struct Message {
    std::int32_t type;
    std::int64_t value;
  };
  static unsigned deliveries = 0;
  static std::int64_t last_value = -1;
  const auto post = +[](std::int64_t port, Message* message) -> bool {
    require(port == 17 && message->type == 3, "invalid Dart integer message");
    ++deliveries;
    last_value = message->value;
    return false;  // A closed isolate port rejects delivery.
  };
  auto wake = mln_wake{};
  require(
    mln_adapter_dart_wake_create(
      reinterpret_cast<void*>(post), 17, &wake, nullptr
    ) == MLN_STATUS_OK,
    "Dart wake creation failed"
  );
  without_allocations([&] {
    wake.callback(wake.user_data);
    wake.release_user_data(wake.user_data);
    return MLN_STATUS_OK;
  });
  require(deliveries == 2 && last_value == 1, "Dart wake did not retire");
  auto arena = mln_adapter_arena_create();
  require(arena != nullptr, "native callback arena creation failed");
  auto* context =
    mln_adapter_arena_allocate(arena, 32, alignof(std::max_align_t));
  require(context != nullptr, "native callback arena allocation failed");
  auto* initial = static_cast<unsigned char*>(context);
  for (std::size_t i = 0; i < 32; ++i)
    require(initial[i] == 0, "callback arena memory was not zeroed");
  std::memset(initial, 0x5a, 32);
  auto* aligned = mln_adapter_arena_allocate(arena, 4096, 256);
  require(
    aligned && reinterpret_cast<std::uintptr_t>(aligned) % 256 == 0,
    "callback arena did not preserve extended alignment"
  );
  std::memset(aligned, 0xa5, 4096);
  require(
    !mln_adapter_arena_allocate(
      arena, std::numeric_limits<std::size_t>::max(), 16
    ),
    "callback arena accepted overflowing allocation"
  );
  for (std::size_t i = 0; i < 32; ++i)
    require(
      initial[i] == 0x5a, "callback arena growth invalidated earlier storage"
    );

  auto runtime_releases = std::atomic_uint{};
  const auto runtime = create_runtime(&runtime_releases);
  require(
    mln_adapter_arena_adopt_handle(arena, runtime, nullptr) == MLN_STATUS_OK,
    "native callback owner adoption failed"
  );
  static auto context_releases = unsigned{};
  require(
    mln_adapter_arena_adopt_release(
      arena, [](void*) { ++context_releases; }, nullptr, nullptr
    ) == MLN_STATUS_OK,
    "native callback release adoption failed"
  );
  auto registration = std::uint64_t{};
  require(
    mln_adapter_dart_release_register(
      reinterpret_cast<void*>(post), 17, context, arena, &registration, nullptr
    ) == MLN_STATUS_OK,
    "Dart release registration failed"
  );
  without_allocations([&] {
    mln_adapter_dart_release(context);
    mln_adapter_dart_release(context);
    return MLN_STATUS_OK;
  });
  require(
    deliveries == 3 && last_value == static_cast<std::int64_t>(registration),
    "Dart release did not retire exactly once"
  );
  wait([&] { return runtime_releases.load() == 1; });
  require(
    context_releases == 1, "closed isolate leaked an adopted context release"
  );
  arena = mln_adapter_arena_create();
  auto second_registration = std::uint64_t{};
  require(
    mln_adapter_dart_release_register(
      reinterpret_cast<void*>(post), 17, context, arena, &second_registration,
      nullptr
    ) == MLN_STATUS_OK,
    "reused callback address registration failed"
  );
  require(
    second_registration != registration,
    "reused callback address reused its notification ID"
  );
  mln_adapter_dart_release(context);
}

}  // namespace

int main() {
  std::set_terminate([] {
    if (auto error = std::current_exception()) {
      try {
        std::rethrow_exception(error);
      } catch (const std::exception& exception) {
        std::fprintf(stderr, "unhandled exception: %s\n", exception.what());
      } catch (...) {
        std::fprintf(stderr, "unhandled nonstandard exception\n");
      }
    } else {
      std::fprintf(stderr, "termination without an active exception\n");
    }
    std::fflush(stderr);
    std::abort();
  });
#define RUN_DISPOSAL_TEST(name)                  \
  do {                                           \
    std::fprintf(stderr, "running " #name "\n"); \
    std::fflush(stderr);                         \
    name();                                      \
  } while (false)
  RUN_DISPOSAL_TEST(runtime_barriers_observe_retired_command_captures);
  RUN_DISPOSAL_TEST(borrowed_views_hold_the_session_through_sibling_disposal);
  RUN_DISPOSAL_TEST(dart_ports_release_after_the_host_closes);
  RUN_DISPOSAL_TEST(dart_notification_ports_copy_records_before_retirement);
  RUN_DISPOSAL_TEST(dart_deferred_callbacks_post_records_before_retirement);
  RUN_DISPOSAL_TEST(undelivered_dart_completion_disposes_its_owned_result);
  RUN_DISPOSAL_TEST(failed_finalizer_token_creation_disposes_the_owner);
  RUN_DISPOSAL_TEST(disposal_retires_an_attached_graph_after_driver_quiescence);
  RUN_DISPOSAL_TEST(
    abandoned_frame_preserves_its_session_owner_without_synthesizing_gpu_sync
  );
  RUN_DISPOSAL_TEST(unclaimed_creation_disposes_on_the_callback_thread);
  RUN_DISPOSAL_TEST(disposal_drains_queued_work_and_map_cleanup);
  RUN_DISPOSAL_TEST(failed_creation_releases_parent_reservation);
  RUN_DISPOSAL_TEST(disposed_parent_allows_observed_child_release);
  RUN_DISPOSAL_TEST(disposal_waits_for_pending_child_creation);
  RUN_DISPOSAL_TEST(a_pending_operation_keeps_only_its_runtime_alive);
#undef RUN_DISPOSAL_TEST
  return 0;
}
