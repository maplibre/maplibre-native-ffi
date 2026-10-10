#define MLN_BUILDING_C

// Adapts synchronous MapLibre callback contracts to hosts that can only receive
// callbacks asynchronously through void listener functions.
//
// Deferred callbacks answer on MapLibre threads and hand copied calls to a
// listener or a Dart port; hosts run them on their own execution contexts.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "maplibre_native_c/callback_adapter.h"

#include "c_api/boundary.hpp"
#include "c_api/callback_capture.hpp"
#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "runtime/runtime.hpp"

namespace {

using AdapterCompletionRecord = mln::capture::Record;

// Dart native API major version 2 fixes this integer-message prefix. Only
// kInt64 is used, so the larger union alternatives are never accessed.
struct DartIntegerMessage {
  std::int32_t type = 3;
  union {
    std::int64_t value = 0;
    struct {
      std::intptr_t length;
      DartIntegerMessage** values;
    } array;
    struct {
      std::intptr_t pointer;
      std::intptr_t size;
      void (*finalize)(void*, void*);
    } native_pointer;
    std::uintptr_t reserved[5];
  };
};
struct DartWake {
  using Post = bool (*)(std::int64_t, DartIntegerMessage*);
  Post post;
  std::int64_t port;

  void notify(std::int64_t value) const noexcept {
    auto message = DartIntegerMessage{3, {value}};
    static_cast<void>(post(port, &message));
  }
};

std::mutex dart_port_mutex;
std::uintptr_t dart_port_id = 0;
std::map<std::uintptr_t, DartWake> dart_ports;

template <std::size_t Count>
void dart_port_notify(
  void* context, const std::int64_t (&values)[Count]
) noexcept {
  auto wake = std::optional<DartWake>{};
  {
    const auto lock = std::lock_guard{dart_port_mutex};
    const auto found =
      dart_ports.find(reinterpret_cast<std::uintptr_t>(context));
    if (found == dart_ports.end()) return;
    wake = found->second;
  }
  auto messages = std::array<DartIntegerMessage, Count>{};
  auto pointers = std::array<DartIntegerMessage*, Count>{};
  for (auto index = std::size_t{}; index < Count; ++index) {
    messages[index].value = values[index];
    pointers[index] = &messages[index];
  }
  auto message = DartIntegerMessage{};
  message.type = 6;
  message.array = {Count, pointers.data()};
  static_cast<void>(wake->post(wake->port, &message));
}

#include "c_api/callback_port_generated.inc"

struct AdapterArena {
  struct Block {
    Block* previous;
  };
  Block* blocks = nullptr;
  void* cursor = nullptr;
  std::size_t available = 0;
  std::size_t next_capacity = 1024;

  auto allocate(std::size_t size, std::size_t alignment) -> void* {
    size = std::max(size, std::size_t{1});
    if (!cursor || !std::align(alignment, size, cursor, available)) {
      constexpr auto maximum = std::numeric_limits<std::size_t>::max();
      if (
        alignment - 1 > maximum - sizeof(Block) ||
        size > maximum - sizeof(Block) - (alignment - 1)
      )
        throw std::bad_alloc{};
      const auto capacity = std::max(next_capacity, size + alignment - 1);
      auto* block =
        new (::operator new(sizeof(Block) + capacity)) Block{blocks};
      blocks = block;
      cursor = reinterpret_cast<std::byte*>(block) + sizeof(Block);
      available = capacity;
      next_capacity =
        capacity <= (maximum - sizeof(Block)) / 2 ? capacity * 2 : capacity;
      static_cast<void>(std::align(alignment, size, cursor, available));
    }
    auto* result = cursor;
    cursor = static_cast<std::byte*>(cursor) + size;
    available -= size;
    return result;
  }
  std::vector<std::uint64_t> handles;
  std::vector<std::pair<mln_runtime_callback_release, void*>> releases;

  ~AdapterArena() {
    for (const auto& [release, context] : releases) {
      release(context);
    }
    for (const auto handle : handles) {
      const auto* type =
        mln::core::handle_kind_name(mln::core::handle_kind_of(handle));
      static_cast<void>(mln::capture::dispose_owner(type, handle));
    }
    while (blocks) {
      auto* previous = blocks->previous;
      ::operator delete(blocks);
      blocks = previous;
    }
  }
};
struct DartRelease {
  DartWake wake;
  std::uint64_t registration;
  std::unique_ptr<AdapterArena> arena;
};
std::mutex dart_release_mutex;
std::uint64_t dart_release_registration = 0;
std::map<void*, DartRelease> dart_releases;

struct AdapterOwnerToken {
  std::uint64_t handle = 0;
};

struct AdapterCompletionState {
  std::uint32_t copy_kind = MLN_ADAPTER_COMPLETION_COPY_FLAT;
  std::size_t element_size = 0;
  mln_adapter_completion_listener listener = nullptr;
  void* user_data = nullptr;
  std::optional<DartWake> dart_port;
  std::int64_t dart_token = 0;
};

void deliver_completion(
  const AdapterCompletionState& state, AdapterCompletionRecord* record
) noexcept {
  if (!state.dart_port) {
    state.listener(state.user_data, record ? &record->view : nullptr);
    return;
  }
  auto token = DartIntegerMessage{3, {state.dart_token}};
  auto payload = DartIntegerMessage{};
  payload.type = 11;
  payload.native_pointer = {
    reinterpret_cast<std::intptr_t>(record ? &record->view : nullptr), 0,
    [](void*, void* peer) {
      mln_adapter_completion_record_destroy(
        static_cast<mln_adapter_completion_record*>(peer)
      );
    }
  };
  DartIntegerMessage* values[] = {&token, &payload};
  auto message = DartIntegerMessage{};
  message.type = 6;
  message.array = {2, values};
  // The VM owns the payload after serialization, including failed delivery.
  static_cast<void>(state.dart_port->post(state.dart_port->port, &message));
}

// A deferred callback context. Registrations guarantee that release follows
// the final call, so the state stays immutable while calls read it.
struct AdapterDeferredState {
  std::uint32_t callback = 0;
  mln_adapter_deferred_call_listener listener = nullptr;
  void* user_data = nullptr;
  std::optional<DartWake> dart_port;
};

auto create_deferred_state(std::uint32_t callback, void** out_context)
  -> std::unique_ptr<AdapterDeferredState> {
  if (
    mln::capture::deferred_function(callback) == nullptr ||
    out_context == nullptr || *out_context != nullptr
  ) {
    mln::core::set_thread_error("deferred callback arguments are invalid");
    return nullptr;
  }
  auto state = std::make_unique<AdapterDeferredState>();
  state->callback = callback;
  return state;
}

void post_deferred(
  const DartWake& port, std::uint32_t callback,
  mln_adapter_deferred_call_record* record
) noexcept {
  auto kind = DartIntegerMessage{3, {static_cast<std::int64_t>(callback)}};
  auto payload = DartIntegerMessage{};
  payload.type = 11;
  payload.native_pointer = {
    reinterpret_cast<std::intptr_t>(record), 0, [](void*, void* peer) {
      mln_adapter_deferred_call_record_destroy(
        static_cast<mln_adapter_deferred_call_record*>(peer)
      );
    }
  };
  DartIntegerMessage* values[] = {&kind, &payload};
  auto message = DartIntegerMessage{};
  message.type = 6;
  message.array = {2, values};
  // The VM owns the payload after serialization, including failed delivery.
  static_cast<void>(port.post(port.port, &message));
}

auto adapter_completion_callback(
  void* user_data, const mln_completion_result* result
) noexcept -> void {
  const auto* state = static_cast<const AdapterCompletionState*>(user_data);
  if (state == nullptr || result == nullptr || state->listener == nullptr)
    return;
  AdapterCompletionRecord* record = nullptr;
  try {
    record = mln::capture::copy(*result, state->copy_kind, state->element_size);
  } catch (...) {
    mln::capture::discard(state->copy_kind, *result);
    deliver_completion(*state, nullptr);
    return;
  }
  deliver_completion(*state, record);
}

auto adapter_completion_release(void* user_data) noexcept -> void {
  delete static_cast<AdapterCompletionState*>(user_data);
}

auto matches_rule(std::uint32_t rule_kind, std::uint32_t request_kind) -> bool {
  return rule_kind == MLN_ADAPTER_RESOURCE_KIND_ANY ||
         rule_kind == request_kind;
}

// Returns how many pattern characters the element at `index` spans, or zero
// when it does not match `candidate`.
auto literal_span(std::string_view pattern, std::size_t index, char candidate)
  -> std::size_t {
  const auto element = pattern[index];
  if (element == '?') {
    return candidate == '/' ? 0 : 1;
  }
  if (element == '\\' && index + 1 < pattern.size()) {
    return pattern[index + 1] == candidate ? 2 : 0;
  }
  return element == candidate ? 1 : 0;
}

// Matches one glob pattern against a candidate URL, in the language
// callback_adapter.h documents. A '*' run stops at a '/' unless the pattern
// spelled '**'. Matching is iterative and allocation-free so that a host
// pattern cannot overflow the MapLibre thread this runs on; the worst case
// costs the product of the two lengths.
auto glob_matches(std::string_view pattern, std::string_view candidate)
  -> bool {
  auto pattern_index = std::size_t{0};
  auto candidate_index = std::size_t{0};

  struct WildcardCheckpoint {
    std::size_t pattern_index = std::string_view::npos;
    std::size_t candidate_index = 0;
  };
  auto segment_wildcard = WildcardCheckpoint{};
  auto spanning_wildcard = WildcardCheckpoint{};

  const auto backtrack = [&]() -> bool {
    if (segment_wildcard.pattern_index != std::string_view::npos) {
      if (
        segment_wildcard.candidate_index < candidate.size() &&
        candidate[segment_wildcard.candidate_index] != '/'
      ) {
        ++segment_wildcard.candidate_index;
        pattern_index = segment_wildcard.pattern_index;
        candidate_index = segment_wildcard.candidate_index;
        return true;
      }
      segment_wildcard = {};
    }
    if (
      spanning_wildcard.pattern_index == std::string_view::npos ||
      spanning_wildcard.candidate_index == candidate.size()
    ) {
      return false;
    }
    ++spanning_wildcard.candidate_index;
    segment_wildcard = {};
    pattern_index = spanning_wildcard.pattern_index;
    candidate_index = spanning_wildcard.candidate_index;
    return true;
  };

  while (true) {
    if (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
      const auto run_start = pattern_index;
      while (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
        ++pattern_index;
      }
      const auto checkpoint =
        WildcardCheckpoint{pattern_index, candidate_index};
      if (pattern_index - run_start > 1) {
        spanning_wildcard = checkpoint;
        segment_wildcard = {};
      } else {
        segment_wildcard = checkpoint;
      }
      continue;
    }
    if (candidate_index < candidate.size()) {
      const auto span =
        pattern_index < pattern.size()
          ? literal_span(pattern, pattern_index, candidate[candidate_index])
          : 0;
      if (span != 0) {
        pattern_index += span;
        ++candidate_index;
        continue;
      }
    } else if (pattern_index == pattern.size()) {
      return true;
    }
    if (!backtrack()) {
      return false;
    }
  }
}

constexpr auto KnownUrlMatchFlags =
  static_cast<std::uint32_t>(MLN_ADAPTER_URL_MATCH_GLOB);

constexpr auto KnownRouteFlags =
  static_cast<std::uint32_t>(MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB) |
  static_cast<std::uint32_t>(MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL);

static_assert(
  static_cast<std::uint32_t>(MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB) ==
    static_cast<std::uint32_t>(MLN_ADAPTER_URL_MATCH_GLOB),
  "a resource route selects glob matching with the shared flag bit"
);

// A null url, an absent candidate, or a flag bit outside known_flags matches
// nothing rather than everything.
auto url_matches(
  std::uint32_t flags, std::uint32_t known_flags, const char* url,
  const char* candidate
) -> bool {
  if (url == nullptr || candidate == nullptr || (flags & ~known_flags) != 0) {
    return false;
  }
  const auto pattern = std::string_view{url};
  const auto target = std::string_view{candidate};
  if ((flags & static_cast<std::uint32_t>(MLN_ADAPTER_URL_MATCH_GLOB)) != 0) {
    return glob_matches(pattern, target);
  }
  return pattern == target;
}

auto has_flag(std::uint32_t flags, mln_adapter_resource_route_flags flag)
  -> bool {
  return (flags & static_cast<std::uint32_t>(flag)) != 0;
}

auto route_matches_url(
  const mln_adapter_resource_route& route, const mln_resource_request& request
) -> bool {
  const auto* candidate =
    has_flag(route.flags, MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL)
      ? request.requested_url
      : request.resolved_url;
  return url_matches(route.flags, KnownRouteFlags, route.url, candidate);
}

auto request_matches_route(
  std::span<const mln_adapter_resource_route> routes,
  const mln_resource_request& request
) -> bool {
  return std::ranges::any_of(routes, [&request](const auto& route) -> bool {
    return matches_rule(route.kind, request.kind) &&
           route_matches_url(route, request);
  });
}

void destroy_owner_token(void* token) noexcept {
  static_cast<void>(std::unique_ptr<AdapterOwnerToken>{
    static_cast<AdapterOwnerToken*>(token),
  });
}

}  // namespace

auto mln::capture::deliver_deferred(
  void* context, std::uint32_t kind, DeferredRecord* record
) noexcept -> bool {
  const auto* state = static_cast<const AdapterDeferredState*>(context);
  if (state == nullptr || state->callback != kind) return false;
  if (state->dart_port) {
    post_deferred(*state->dart_port, kind, &record->view);
  } else {
    state->listener(state->user_data, &record->view);
  }
  return true;
}

extern "C" MLN_API auto mln_adapter_deferred_callback_create(
  std::uint32_t callback, mln_adapter_deferred_call_listener listener,
  void* listener_user_data, void** out_context, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    auto state = create_deferred_state(callback, out_context);
    if (state == nullptr || listener == nullptr) {
      mln::core::set_thread_error("deferred callback arguments are invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    state->listener = listener;
    state->user_data = listener_user_data;
    *out_context = state.release();
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API auto mln_adapter_dart_deferred_callback_create(
  std::uint32_t callback, void* post_cobject, std::int64_t port,
  void** out_context, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    auto state = create_deferred_state(callback, out_context);
    if (state == nullptr || post_cobject == nullptr || port == 0) {
      mln::core::set_thread_error("deferred callback arguments are invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    state->dart_port =
      DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port};
    *out_context = state.release();
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API auto mln_adapter_deferred_callback_function(
  std::uint32_t callback
) noexcept -> void* {
  return mln::capture::deferred_function(callback);
}

extern "C" MLN_API void mln_adapter_deferred_callback_release(
  void* context
) noexcept {
  const auto state = std::unique_ptr<AdapterDeferredState>{
    static_cast<AdapterDeferredState*>(context)
  };
  if (state == nullptr) return;
  if (state->dart_port) {
    state->dart_port->notify(0);
  } else {
    state->listener(state->user_data, nullptr);
  }
}

extern "C" MLN_API void mln_adapter_deferred_call_record_adopt(
  mln_adapter_deferred_call_record* record
) noexcept {
  if (record == nullptr || record->owner == nullptr) return;
  static_cast<mln::capture::DeferredRecord*>(record->owner)->claimed = true;
}

extern "C" MLN_API void mln_adapter_deferred_call_record_destroy(
  mln_adapter_deferred_call_record* record
) noexcept {
  if (record == nullptr || record->owner == nullptr) return;
  mln::capture::destroy_deferred(
    static_cast<mln::capture::DeferredRecord*>(record->owner)
  );
}

extern "C" MLN_API auto mln_adapter_completion_create(
  std::uint32_t copy_kind, std::size_t element_size,
  mln_adapter_completion_listener listener, void* user_data,
  mln_completion* out_completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (
      out_completion == nullptr || out_completion->size != 0 ||
      out_completion->callback != nullptr ||
      out_completion->user_data != nullptr ||
      out_completion->release_user_data != nullptr || listener == nullptr ||
      !mln::capture::valid_kind(copy_kind)
    ) {
      mln::core::set_thread_error("completion adapter arguments are invalid");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    auto state = std::make_unique<AdapterCompletionState>();
    state->copy_kind = copy_kind;
    state->element_size = element_size;
    state->listener = listener;
    state->user_data = user_data;
    *out_completion = mln_completion{
      .size = sizeof(mln_completion),
      .callback = adapter_completion_callback,
      .user_data = state.get(),
      .release_user_data = adapter_completion_release,
    };
    static_cast<void>(state.release());
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API auto mln_adapter_dart_completion_create(
  std::uint32_t copy_kind, std::size_t element_size, void* post_cobject,
  std::int64_t port, std::int64_t token, mln_completion* out_completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (!post_cobject || !port) {
      mln::core::set_thread_error(
        "post_cobject and port must name a Dart receive port"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto status = mln_adapter_completion_create(
      copy_kind, element_size, [](void*, mln_adapter_completion_record*) {},
      nullptr, out_completion, nullptr
    );
    if (status != MLN_STATUS_OK) return status;
    auto* state =
      static_cast<AdapterCompletionState*>(out_completion->user_data);
    state->dart_port =
      DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port};
    state->dart_token = token;
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API void mln_adapter_completion_reject(
  mln_completion* completion
) noexcept {
  if (
    completion == nullptr ||
    completion->callback != adapter_completion_callback ||
    completion->release_user_data != adapter_completion_release
  ) {
    return;
  }
  adapter_completion_release(completion->user_data);
  *completion = {};
}

extern "C" MLN_API void mln_adapter_completion_record_adopt(
  mln_adapter_completion_record* record
) noexcept {
  if (record == nullptr || record->owner == nullptr) return;
  static_cast<AdapterCompletionRecord*>(record->owner)->claimed = true;
}

extern "C" MLN_API void mln_adapter_completion_record_destroy(
  mln_adapter_completion_record* record
) noexcept {
  if (record == nullptr || record->owner == nullptr) return;
  mln::capture::destroy(static_cast<AdapterCompletionRecord*>(record->owner));
}

extern "C" MLN_API auto mln_adapter_dart_wake_create(
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
    auto state = std::make_unique<DartWake>(
      DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port}
    );
    *out_wake = {};
    out_wake->size = sizeof(mln_wake);
    out_wake->callback = [](void* context) {
      static_cast<DartWake*>(context)->notify(0);
    };
    out_wake->release_user_data = [](void* context) {
      const auto state =
        std::unique_ptr<DartWake>{static_cast<DartWake*>(context)};
      state->notify(1);
    };
    out_wake->user_data = state.release();
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API auto mln_adapter_dart_port_create(
  void* post_cobject, std::int64_t port
) noexcept -> void* {
  if (!post_cobject || !port) return nullptr;
  try {
    const auto lock = std::lock_guard{dart_port_mutex};
    if (dart_port_id == std::numeric_limits<std::uintptr_t>::max())
      return nullptr;
    const auto id = ++dart_port_id;
    dart_ports.emplace(
      id, DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port}
    );
    return reinterpret_cast<void*>(id);
  } catch (...) {
    return nullptr;
  }
}

extern "C" MLN_API auto mln_adapter_dart_port_function(
  std::uint32_t id
) noexcept -> void* {
  return dart_port_function(id);
}

extern "C" MLN_API void mln_adapter_dart_port_release(void* context) noexcept {
  auto wake = std::optional<DartWake>{};
  {
    const auto lock = std::lock_guard{dart_port_mutex};
    const auto entry =
      dart_ports.find(reinterpret_cast<std::uintptr_t>(context));
    if (entry == dart_ports.end()) return;
    wake = entry->second;
    dart_ports.erase(entry);
  }
  wake->notify(0);
}

extern "C" MLN_API auto mln_adapter_arena_create() noexcept -> void* {
  try {
    return new AdapterArena{};
  } catch (...) {
    return nullptr;
  }
}

extern "C" MLN_API auto mln_adapter_arena_allocate(
  void* arena, std::size_t size, std::size_t alignment
) noexcept -> void* {
  if (!arena || !alignment || (alignment & (alignment - 1))) return nullptr;
  try {
    auto* data = static_cast<AdapterArena*>(arena)->allocate(size, alignment);
    std::memset(data, 0, size);
    return data;
  } catch (...) {
    return nullptr;
  }
}

extern "C" MLN_API void mln_adapter_arena_destroy(void* arena) noexcept {
  delete static_cast<AdapterArena*>(arena);
}

extern "C" MLN_API auto mln_adapter_arena_adopt_handle(
  void* arena, std::uint64_t handle, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  const auto status =
    mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
      if (!arena) {
        mln::core::set_thread_error("arena must not be null");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      static_cast<AdapterArena*>(arena)->handles.push_back(handle);
      return MLN_STATUS_OK;
    });
  if (status != MLN_STATUS_OK) {
    const auto* type =
      mln::core::handle_kind_name(mln::core::handle_kind_of(handle));
    static_cast<void>(mln::capture::dispose_owner(type, handle));
  }
  return status;
}

extern "C" MLN_API auto mln_adapter_arena_adopt_release(
  void* arena, mln_runtime_callback_release release, void* context,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  const auto status =
    mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
      if (release == nullptr) {
        mln::core::set_thread_error("release must not be null");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      if (!arena) {
        mln::core::set_thread_error("arena must not be null");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      static_cast<AdapterArena*>(arena)->releases.emplace_back(
        release, context
      );
      return MLN_STATUS_OK;
    });
  if (status != MLN_STATUS_OK && release != nullptr) release(context);
  return status;
}

extern "C" MLN_API auto mln_adapter_dart_release_register(
  void* post_cobject, std::int64_t port, void* context, void* arena,
  std::uint64_t* out_registration, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  auto owned_arena =
    std::unique_ptr<AdapterArena>{static_cast<AdapterArena*>(arena)};
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (out_registration) *out_registration = 0;
    if (
      post_cobject == nullptr || port == 0 || context == nullptr ||
      !out_registration
    ) {
      mln::core::set_thread_error(
        "Dart release requires a posting function, port, context and output"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto lock = std::lock_guard{dart_release_mutex};
    if (dart_release_registration == std::numeric_limits<std::int64_t>::max()) {
      mln::core::set_thread_error(
        "Dart release registration identifiers exhausted"
      );
      return MLN_STATUS_NATIVE_ERROR;
    }
    const auto id = ++dart_release_registration;
    const auto [entry, inserted] = dart_releases.try_emplace(context);
    if (!inserted) {
      mln::core::set_thread_error("Dart release context is already registered");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    entry->second = DartRelease{
      DartWake{reinterpret_cast<DartWake::Post>(post_cobject), port}, id,
      std::move(owned_arena)
    };
    *out_registration = id;
    return MLN_STATUS_OK;
  });
}

extern "C" MLN_API void mln_adapter_dart_release(void* context) noexcept {
  auto notification = std::optional<DartRelease>{};
  {
    const auto lock = std::lock_guard{dart_release_mutex};
    const auto entry = dart_releases.find(context);
    if (entry == dart_releases.end()) return;
    notification = std::move(entry->second);
    dart_releases.erase(entry);
  }
  notification->wake.notify(
    static_cast<std::int64_t>(notification->registration)
  );
}

extern "C" MLN_API auto mln_adapter_owner_token_create(
  std::uint64_t handle
) noexcept -> void* {
  try {
    auto token = std::make_unique<AdapterOwnerToken>();
    token->handle = handle;
    return token.release();
  } catch (...) {
    const auto* type =
      mln::core::handle_kind_name(mln::core::handle_kind_of(handle));
    static_cast<void>(mln::capture::dispose_owner(type, handle));
    return nullptr;
  }
}

extern "C" MLN_API void mln_adapter_owner_token_destroy(void* token) noexcept {
  destroy_owner_token(token);
}

extern "C" MLN_API void mln_adapter_owner_finalize(void* token) noexcept {
  auto* owner = static_cast<AdapterOwnerToken*>(token);
  if (owner != nullptr) {
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
        : mln::capture::dispose_owner(
            mln::core::handle_kind_name(kind), owner->handle
          );
    if (status != MLN_STATUS_OK) {
      static_cast<void>(std::fprintf(
        stderr,
        "maplibre_native_ffi: owner finalization failed for handle %llu "
        "(status %d)\n",
        static_cast<unsigned long long>(owner->handle), static_cast<int>(status)
      ));
    }
  }
  destroy_owner_token(token);
}

extern "C" MLN_API auto mln_adapter_render_session_abandon_at_exit(
  mln_render_session session, mln_render_abandon_result* out_result,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_abandon_keeping_graphics(
      session, out_result
    );
  });
}

extern "C" MLN_API auto mln_adapter_resource_transform_rewrite_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) noexcept -> mln_status {
  if (user_data == nullptr || url == nullptr || out_response == nullptr) {
    return MLN_STATUS_OK;
  }

  const auto& table =
    *static_cast<const mln_adapter_resource_rewrite_rules*>(user_data);
  for (const auto& rule : std::span{table.rules, table.count}) {
    if (
      matches_rule(rule.kind, kind) &&
      url_matches(rule.flags, KnownUrlMatchFlags, rule.url, url)
    ) {
      if (rule.replacement_url == nullptr) {
        return MLN_STATUS_OK;
      }
      return mln_resource_transform_response_set_url(
        out_response, rule.replacement_url, std::strlen(rule.replacement_url),
        nullptr
      );
    }
  }
  return MLN_STATUS_OK;
}

extern "C" MLN_API auto mln_adapter_http_header_transform_callback(
  void* user_data, std::uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) noexcept -> mln_status {
  if (user_data == nullptr || url == nullptr || out_response == nullptr) {
    return MLN_STATUS_OK;
  }

  const auto& table =
    *static_cast<const mln_adapter_http_header_transform_rules*>(user_data);
  if (table.rules == nullptr && table.count != 0) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  for (const auto& rule : std::span{table.rules, table.count}) {
    if (
      !matches_rule(rule.kind, kind) ||
      !url_matches(rule.flags, KnownUrlMatchFlags, rule.url, url)
    ) {
      continue;
    }
    if (rule.headers == nullptr && rule.header_count != 0) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    for (const auto& header : std::span{rule.headers, rule.header_count}) {
      const auto name_size =
        header.name == nullptr ? 0 : std::strlen(header.name);
      const auto value_size =
        header.value == nullptr ? 0 : std::strlen(header.value);
      const auto status = mln_http_header_transform_response_set(
        out_response, header.name, name_size, header.value, value_size, nullptr
      );
      if (status != MLN_STATUS_OK) {
        return status;
      }
    }
    return MLN_STATUS_OK;
  }
  return MLN_STATUS_OK;
}

extern "C" MLN_API auto mln_adapter_http_header_validate(
  const char* name, const char* value, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::validate_http_header(
      name, name == nullptr ? 0 : std::strlen(name), value,
      value == nullptr ? 0 : std::strlen(value)
    );
  });
}

extern "C" MLN_API auto mln_adapter_resource_provider_rules_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) noexcept -> std::uint32_t {
  if (
    user_data == nullptr || request == nullptr ||
    request->requested_url == nullptr || handle == MLN_HANDLE_NULL
  ) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }

  const auto& table =
    *static_cast<const mln_adapter_resource_provider_rules*>(user_data);
  for (const auto& rule : std::span{table.rules, table.count}) {
    if (
      matches_rule(rule.kind, request->kind) &&
      url_matches(
        rule.flags, KnownUrlMatchFlags, rule.requested_url,
        request->requested_url
      )
    ) {
      static_cast<void>(
        mln_resource_request_complete(handle, &rule.response, nullptr)
      );
      mln_resource_request_release(handle);
      return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
    }
  }
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

extern "C" MLN_API auto mln_adapter_routed_resource_provider_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) noexcept -> std::uint32_t {
  // Each route decides which URL it compares, so route matching handles an
  // absent URL.
  if (user_data == nullptr || request == nullptr || handle == MLN_HANDLE_NULL) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }
  const auto& provider =
    *static_cast<const mln_adapter_routed_resource_provider*>(user_data);
  if (
    provider.callback == nullptr ||
    (provider.routes == nullptr && provider.route_count != 0) ||
    !request_matches_route(
      std::span{provider.routes, provider.route_count}, *request
    )
  ) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }
  return provider.callback(provider.user_data, request, handle);
}

extern "C" MLN_API void mln_adapter_custom_geometry_callbacks_retire(
  mln_custom_geometry_source_tile_callback fetch_tile,
  mln_custom_geometry_source_tile_callback cancel_tile, void* user_data
) noexcept {
  constexpr auto RetirementTile = mln_canonical_tile_id{
    .z = std::numeric_limits<std::uint8_t>::max(),
    .x = 0,
    .y = 0,
  };
  if (fetch_tile != nullptr) {
    fetch_tile(user_data, RetirementTile);
  }
  if (cancel_tile != nullptr) {
    cancel_tile(user_data, RetirementTile);
  }
}

extern "C" MLN_API void mln_adapter_custom_mvt_vector_callbacks_retire(
  mln_custom_mvt_vector_source_tile_callback fetch_tile,
  mln_custom_mvt_vector_source_tile_callback cancel_tile, void* user_data
) noexcept {
  constexpr auto RetirementTile = mln_canonical_tile_id{
    .z = std::numeric_limits<std::uint8_t>::max(),
    .x = 0,
    .y = 0,
  };
  if (fetch_tile != nullptr) {
    fetch_tile(user_data, RetirementTile);
  }
  if (cancel_tile != nullptr) {
    cancel_tile(user_data, RetirementTile);
  }
}
