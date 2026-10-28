#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <mln/actor/actor_ref.hpp>
#include <mln/storage/file_source.hpp>
#include <mln/storage/file_source_request.hpp>
#include <mln/storage/resource.hpp>
#include <mln/storage/response.hpp>
#include <mln/util/async_request.hpp>
#include <mln/util/chrono.hpp>

#include "resources/custom_resource_provider.hpp"

#include "delivery/queued_registration.hpp"
#include "diagnostics/diagnostics.hpp"
#include "diagnostics/string_view.hpp"
#include "execution/process_exit.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"
#include "resources/resource_request_wake.hpp"
#include "testing/sync_point.hpp"

namespace mln::core {

struct ResourceRequestObject;

// Owns both phases and a reserved decision wake on the requesting run loop.
class ProviderRequest final : public mln::AsyncRequest {
 public:
  using StartNative = std::function<
    std::unique_ptr<mln::AsyncRequest>(mln::FileSource::Callback)>;
  enum class Decision { None, PassThrough, Failure };
  enum class Failure { Released, Exception, ProcessExit, Native };
  static constexpr auto failure_messages = std::array{
    std::string_view{"resource request was released without a response"},
    std::string_view{"resource provider threw an exception"},
    std::string_view{"the process is exiting"},
    std::string_view{"native resource loading failed"},
  };
  ProviderRequest(mln::FileSource::Callback callback, StartNative start)
      : response(mln::FileSource::Callback{callback}),
        start_(std::move(start)),
        failure_callback_(std::move(callback)),
        failure_error_(
          std::make_unique<mln::Response::Error>(
            mln::Response::Error::Reason::Other
          )
        ) {
    auto capacity = std::size_t{0};
    for (const auto message : failure_messages)
      capacity = std::max(capacity, message.size());
    failure_error_->message.reserve(capacity);
  }
  ~ProviderRequest() override;
  void prepare(const std::shared_ptr<ResourceRequestObject>& object);
  // The caller holds the request mutex. The wake is reserved by prepare().
  void schedule(
    Decision decision, Failure failure = Failure::Released
  ) noexcept {
    if (decision == Decision::Failure) set_failure(failure);
    pending_ = decision;
    wake_->send();
  }
  void pass_through() {
    const auto target = response.actor();
    try {
      native_ = start_([target](const mln::Response& result) {
        target.invoke(&mln::FileSourceRequest::setResponse, result);
      });
    } catch (...) {
      deliver_failure(Failure::Native);
    }
  }
  mln::FileSourceRequest response;

 private:
  void drain();
  void set_failure(Failure failure) noexcept {
    const auto message = failure_messages[static_cast<std::size_t>(failure)];
    failure_error_->message.assign(message.data(), message.size());
  }
  void deliver_failure(Failure failure);
  StartNative start_;
  mln::FileSource::Callback failure_callback_;
  std::unique_ptr<mln::Response::Error> failure_error_;
  std::shared_ptr<ResourceRequestObject> object_;
  Decision pending_ = Decision::None;
  std::unique_ptr<mln::AsyncRequest> native_;
  std::unique_ptr<ResourceRequestWake> wake_;
};

struct ResourceRequestObject {
  ResourceRequestObject(
    mln::ActorRef<mln::FileSourceRequest> actor_, ProviderRequest* loader_
  )
      : actor(std::move(actor_)), loader(loader_) {}

  mutable std::mutex mutex;
  std::condition_variable state_changed;
  bool cancelled = false;
  bool completed = false;
  bool retired = false;
  // The provider callback is running, has returned holding the handle, or saw
  // the handle released before it returned.
  bool deciding = false;
  bool claimed = false;
  bool released_while_deciding = false;
  mln_resource_request_cancel_callback cancel_callback = nullptr;
  mln_user_data_release cancel_release = nullptr;
  void* cancel_user_data = nullptr;
  bool cancel_callback_registered = false;
  bool cancel_callback_running = false;
  // Set when the callback released its own request: the callback wrapper
  // removes the table entry once the callback returns.
  bool remove_after_cancel_callback = false;
  std::thread::id cancel_callback_thread;
  mln_resource_request handle = MLN_HANDLE_NULL;
  mln::ActorRef<mln::FileSourceRequest> actor;
  ProviderRequest* loader;
  // The cached bytes MapLibre withheld from the requester until this request
  // revalidates them. Set before the provider sees the handle, then only read.
  std::shared_ptr<const std::string> prior_data;
  std::optional<mln::Timestamp> prior_modified;
  std::optional<std::string> prior_etag;
};

ProviderRequest::~ProviderRequest() {
  if (object_ != nullptr) {
    const auto lock = std::scoped_lock{object_->mutex};
    object_->cancelled = true;
    object_->loader = nullptr;
  }
}

void ProviderRequest::prepare(
  const std::shared_ptr<ResourceRequestObject>& object
) {
  object_ = object;
  wake_ = std::make_unique<ResourceRequestWake>([this] { drain(); });
}

void ProviderRequest::deliver_failure(Failure failure) {
  set_failure(failure);
  auto result = mln::Response{};
  result.error = std::move(failure_error_);
  // The callback may destroy this request. Move its storage before invoking it.
  auto callback = std::move(failure_callback_);
  callback(result);
}

void ProviderRequest::drain() {
  auto decision = Decision::None;
  {
    const auto lock = std::scoped_lock{object_->mutex};
    if (object_->cancelled) return;
    decision = std::exchange(pending_, Decision::None);
  }
  if (decision == Decision::PassThrough) {
    pass_through();
  } else if (decision == Decision::Failure) {
    // The diagnostic and callback storage were reserved before publication.
    auto result = mln::Response{};
    result.error = std::move(failure_error_);
    auto callback = std::move(failure_callback_);
    callback(result);
  }
}

// Host code may complete a request from any thread, and mbgl's cancel path runs
// on its own, so the object must be leasable to outlive a racing release.
template <>
struct HandleTraits<ResourceRequestObject> {
  static constexpr auto kind = HandleKind::ResourceRequest;
  static constexpr auto leasable = true;
};

namespace {

auto error_response(std::string message, mln::Response::Error::Reason reason)
  -> mln::Response {
  auto response = mln::Response{};
  response.error =
    std::make_unique<mln::Response::Error>(reason, std::move(message));
  return response;
}

auto to_unix_ms(const mln::Timestamp& timestamp) -> std::int64_t {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
           timestamp.time_since_epoch()
  )
    .count();
}

auto from_unix_ms(std::int64_t unix_ms) -> mln::Timestamp {
  return std::chrono::time_point_cast<mln::Seconds>(
    std::chrono::time_point<
      std::chrono::system_clock, std::chrono::milliseconds>{
      std::chrono::milliseconds{unix_ms}
    }
  );
}

auto kind_to_abi(mln::Resource::Kind kind) -> mln_resource_kind {
  switch (kind) {
    case mln::Resource::Kind::Style:
      return MLN_RESOURCE_KIND_STYLE;
    case mln::Resource::Kind::Source:
      return MLN_RESOURCE_KIND_SOURCE;
    case mln::Resource::Kind::Tile:
      return MLN_RESOURCE_KIND_TILE;
    case mln::Resource::Kind::Glyphs:
      return MLN_RESOURCE_KIND_GLYPHS;
    case mln::Resource::Kind::SpriteImage:
      return MLN_RESOURCE_KIND_SPRITE_IMAGE;
    case mln::Resource::Kind::SpriteJSON:
      return MLN_RESOURCE_KIND_SPRITE_JSON;
    case mln::Resource::Kind::Image:
      return MLN_RESOURCE_KIND_IMAGE;
    case mln::Resource::Kind::Unknown:
    default:
      return MLN_RESOURCE_KIND_UNKNOWN;
  }
}

auto loading_method_to_abi(mln::Resource::LoadingMethod method)
  -> mln_resource_loading_method {
  switch (method) {
    case mln::Resource::LoadingMethod::CacheOnly:
      return MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY;
    case mln::Resource::LoadingMethod::NetworkOnly:
      return MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY;
    case mln::Resource::LoadingMethod::All:
    case mln::Resource::LoadingMethod::None:
    default:
      return MLN_RESOURCE_LOADING_METHOD_ALL;
  }
}

auto error_reason_from_abi(mln_resource_error_reason reason)
  -> mln::Response::Error::Reason {
  switch (reason) {
    case MLN_RESOURCE_ERROR_REASON_NOT_FOUND:
      return mln::Response::Error::Reason::NotFound;
    case MLN_RESOURCE_ERROR_REASON_SERVER:
      return mln::Response::Error::Reason::Server;
    case MLN_RESOURCE_ERROR_REASON_CONNECTION:
      return mln::Response::Error::Reason::Connection;
    case MLN_RESOURCE_ERROR_REASON_RATE_LIMIT:
      return mln::Response::Error::Reason::RateLimit;
    case MLN_RESOURCE_ERROR_REASON_OTHER:
    case MLN_RESOURCE_ERROR_REASON_NONE:
    default:
      return mln::Response::Error::Reason::Other;
  }
}

auto response_from_abi(const mln_resource_response& provider_response)
  -> mln::Response {
  if (provider_response.size < sizeof(mln_resource_response)) {
    return error_response(
      "mln_resource_response.size is too small",
      mln::Response::Error::Reason::Other
    );
  }
  constexpr auto known_fields = MLN_RESOURCE_RESPONSE_MODIFIED |
                                MLN_RESOURCE_RESPONSE_EXPIRES |
                                MLN_RESOURCE_RESPONSE_RETRY_AFTER;
  if ((provider_response.fields & ~known_fields) != 0U) {
    return error_response(
      "mln_resource_response.fields contains unknown bits",
      mln::Response::Error::Reason::Other
    );
  }
  switch (provider_response.status) {
    case MLN_RESOURCE_RESPONSE_STATUS_OK:
    case MLN_RESOURCE_RESPONSE_STATUS_ERROR:
    case MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT:
    case MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED:
      break;
    default:
      return error_response(
        "resource provider returned an unknown response status",
        mln::Response::Error::Reason::Other
      );
  }
  if (provider_response.byte_count != 0 && provider_response.bytes == nullptr) {
    return error_response(
      "resource provider returned a null byte buffer",
      mln::Response::Error::Reason::Other
    );
  }

  auto response = mln::Response{};
  response.noContent =
    provider_response.status == MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT;
  response.notModified =
    provider_response.status == MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED;
  response.mustRevalidate = provider_response.must_revalidate;
  if ((provider_response.fields & MLN_RESOURCE_RESPONSE_MODIFIED) != 0U) {
    response.modified = from_unix_ms(provider_response.modified_unix_ms);
  }
  if ((provider_response.fields & MLN_RESOURCE_RESPONSE_EXPIRES) != 0U) {
    response.expires = from_unix_ms(provider_response.expires_unix_ms);
  }
  if (provider_response.etag.size != 0) {
    response.etag = string_from_view(provider_response.etag);
  }

  if (provider_response.status == MLN_RESOURCE_RESPONSE_STATUS_ERROR) {
    auto message = std::string{"resource provider failed"};
    if (provider_response.error_message.size != 0) {
      message = string_from_view(provider_response.error_message);
    }
    auto retry_after = std::optional<mln::Timestamp>{};
    if ((provider_response.fields & MLN_RESOURCE_RESPONSE_RETRY_AFTER) != 0U) {
      retry_after = from_unix_ms(provider_response.retry_after_unix_ms);
    }
    response.error = std::make_unique<mln::Response::Error>(
      error_reason_from_abi(provider_response.error_reason), std::move(message),
      retry_after
    );
    return response;
  }

  if (!response.notModified && !response.noContent) {
    auto data = std::make_shared<std::string>();
    data->resize(provider_response.byte_count);
    if (provider_response.byte_count != 0) {
      const auto bytes =
        std::span{provider_response.bytes, provider_response.byte_count};
      std::ranges::copy(bytes, data->begin());
    }
    response.data = std::move(data);
  }
  return response;
}

// The caller holds object.mutex and keeps the object alive after handle
// release.
void fail_locked(
  ResourceRequestObject& object, ProviderRequest::Failure failure
) noexcept {
  if (object.completed || object.cancelled) {
    return;
  }
  object.loader->schedule(ProviderRequest::Decision::Failure, failure);
  object.completed = true;
}

// The cancel callback of a queue cancel handler, which queues the cancel.
void queue_request_cancel(
  void* registration, mln_resource_request handle
) noexcept {
  static_cast<mln::delivery::QueuedRegistration*>(registration)
    ->push_reserved(
      mln::delivery::ResourceRequestCancelArguments{.handle = handle}
    );
}

// Runs the registered cancel callback, if any, for a request that has not been
// completed. Callers hold no lock; the callback may call back into this handle.
void run_cancel_callback(ResourceRequestObject& object) noexcept {
  mln_resource_request_cancel_callback callback = nullptr;
  mln_user_data_release release = nullptr;
  void* user_data = nullptr;
  {
    const std::scoped_lock lock(object.mutex);
    if (object.completed || object.cancel_callback == nullptr) {
      return;
    }
    callback = std::exchange(object.cancel_callback, nullptr);
    release = std::exchange(object.cancel_release, nullptr);
    user_data = std::exchange(object.cancel_user_data, nullptr);
    object.cancel_callback_running = true;
    object.cancel_callback_thread = std::this_thread::get_id();
  }
  if (!process_exiting()) {
    try {
      callback(user_data, object.handle);
    } catch (...) {
      // Host callbacks must not unwind through MapLibre's cancel path.
    }
  }
  // The callback runs at most once, so its context retires as it returns.
  if (release != nullptr && !process_exiting()) {
    try {
      release(user_data);
    } catch (...) {
      // Host callbacks must not unwind through MapLibre's cancel path.
    }
  }
  auto remove_from_table = false;
  {
    const std::scoped_lock lock(object.mutex);
    object.cancel_callback_running = false;
    remove_from_table =
      std::exchange(object.remove_after_cancel_callback, false);
  }
  object.state_changed.notify_all();
  if (remove_from_table) {
    handle_table<ResourceRequestObject>().remove(object.handle);
  }
}

// Retires the id so no later call can reach this request. Idempotent, and
// safe to run on several threads at once. A release on another thread waits
// for a running cancel callback; a self-release leaves removal to the callback.
void retire_request(mln_resource_request handle) noexcept {
  auto object = handle_table<ResourceRequestObject>().try_lease(handle);
  if (object == nullptr) {
    return;
  }
  // A registration whose callback never ran retires with the request.
  mln_user_data_release release = nullptr;
  void* user_data = nullptr;
  {
    auto lock = std::unique_lock{object->mutex};
    object->retired = true;
    object->cancel_callback = nullptr;
    release = std::exchange(object->cancel_release, nullptr);
    user_data = std::exchange(object->cancel_user_data, nullptr);
    if (object->cancel_callback_running) {
      if (object->cancel_callback_thread == std::this_thread::get_id()) {
        object->remove_after_cancel_callback = true;
        return;
      }
      mln::testing::hit(mln::testing::SyncPoint::ResourceRequestCancelWait);
      object->state_changed.wait(lock, [&object] {
        return !object->cancel_callback_running;
      });
    }
  }
  if (release != nullptr) {
    try {
      if (!process_exiting()) release(user_data);
    } catch (...) {
      // Host callbacks must not unwind through the release path.
    }
  }
  handle_table<ResourceRequestObject>().remove(handle);
}

auto bytes_from_string(const std::string& value) -> const std::uint8_t* {
  // C APIs conventionally expose byte buffers as uint8_t even when native data
  // is stored as std::string.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const std::uint8_t*>(value.data());
}

auto text_view(const std::string& value) -> mln_buffer_view {
  return {.data = value.data(), .size = value.size()};
}

auto make_request_view(
  const mln::Resource& resource, const std::string& resolved_url
) -> mln_resource_request_info {
  const auto* prior_data = resource.priorData == nullptr
                             ? nullptr
                             : bytes_from_string(*resource.priorData);
  auto fields = std::uint32_t{0};
  if (resource.dataRange) fields |= MLN_RESOURCE_REQUEST_RANGE;
  if (resource.priorModified) fields |= MLN_RESOURCE_REQUEST_PRIOR_MODIFIED;
  if (resource.priorExpires) fields |= MLN_RESOURCE_REQUEST_PRIOR_EXPIRES;
  auto request = mln_resource_request_info{
    .size = sizeof(mln_resource_request_info),
    .fields = fields,
    .requested_url = text_view(resource.url),
    .resolved_url = text_view(resolved_url),
    .kind = kind_to_abi(resource.kind),
    .loading_method = loading_method_to_abi(resource.loadingMethod),
    .priority = resource.priority == mln::Resource::Priority::Low
                  ? MLN_RESOURCE_PRIORITY_LOW
                  : MLN_RESOURCE_PRIORITY_REGULAR,
    .usage = resource.usage == mln::Resource::Usage::Offline
               ? MLN_RESOURCE_USAGE_OFFLINE
               : MLN_RESOURCE_USAGE_ONLINE,
    .storage_policy =
      resource.storagePolicy == mln::Resource::StoragePolicy::Volatile
        ? MLN_RESOURCE_STORAGE_POLICY_VOLATILE
        : MLN_RESOURCE_STORAGE_POLICY_PERMANENT,
    .range =
      resource.dataRange
        ? mln_resource_range{.start = resource.dataRange->first,
                             .end = resource.dataRange->second}
        : mln_resource_range{},
    .prior_modified_unix_ms =
      resource.priorModified ? to_unix_ms(*resource.priorModified) : 0,
    .prior_expires_unix_ms =
      resource.priorExpires ? to_unix_ms(*resource.priorExpires) : 0,
    .prior_etag = resource.priorEtag ? text_view(*resource.priorEtag)
                                     : mln_buffer_view{.data = "", .size = 0},
    .prior_data = prior_data,
    .prior_data_size =
      resource.priorData == nullptr ? 0 : resource.priorData->size(),
  };
  return request;
}

struct CustomProviderInvocation {
  mln_resource_request handle = MLN_HANDLE_NULL;
  // Held for the whole invocation, so a host that releases inline cannot free
  // the object out from under the code that runs after the callback returns.
  std::shared_ptr<ResourceRequestObject> object;
  const mln::Resource& resource;
  std::string resolved_url;
  mln_resource_provider_callback callback = nullptr;
  void* user_data = nullptr;
};

// Answers a request that the provider did not answer with an error, and
// retires it.
auto fail_request(
  ResourceRequestObject& object, ProviderRequest::Failure failure
) noexcept -> void {
  {
    const std::scoped_lock lock(object.mutex);
    object.deciding = false;
    fail_locked(object, failure);
  }
  retire_request(object.handle);
}

auto invoke_custom_provider(CustomProviderInvocation invocation) noexcept
  -> bool {
  try {
    if (process_exiting()) {
      fail_request(*invocation.object, ProviderRequest::Failure::ProcessExit);
      return true;
    }
    const auto request =
      make_request_view(invocation.resource, invocation.resolved_url);
    {
      const std::scoped_lock lock(invocation.object->mutex);
      invocation.object->deciding = true;
    }
    invocation.callback(invocation.user_data, &request, invocation.handle);
    // The provider holds the handle until an answer consumes it or it is
    // released. A release during the callback fails the request only now, so
    // that nothing above runs against a failed request.
    {
      const std::scoped_lock lock(invocation.object->mutex);
      invocation.object->deciding = false;
      invocation.object->claimed = true;
      if (invocation.object->released_while_deciding) {
        fail_locked(*invocation.object, ProviderRequest::Failure::Released);
      }
    }
    return true;
  } catch (...) {
    fail_request(*invocation.object, ProviderRequest::Failure::Exception);
    return true;
  }
}

}  // namespace

auto request_custom_resource(
  const mln::Resource& resource, std::string resolved_url,
  mln_resource_provider_callback provider_callback, void* user_data,
  mln::FileSource::Callback file_source_callback,
  ProviderRequest::StartNative start_native
) -> std::unique_ptr<mln::AsyncRequest> {
  auto request = std::make_unique<ProviderRequest>(
    std::move(file_source_callback), std::move(start_native)
  );
  auto object = std::make_shared<ResourceRequestObject>(
    request->response.actor(), request.get()
  );
  request->prepare(object);
  object->prior_data = resource.priorData;
  object->prior_modified = resource.priorModified;
  object->prior_etag = resource.priorEtag;
  const auto handle = handle_table<ResourceRequestObject>().insert(object);
  object->handle = handle;
  // mbgl runs this on every request destruction, including after a response
  // was delivered, so a completed request does not report cancellation.
  request->response.onCancel([object]() noexcept -> void {
    {
      const std::scoped_lock lock(object->mutex);
      object->cancelled = true;
    }
    run_cancel_callback(*object);
    mln::testing::hit(mln::testing::SyncPoint::ResourceRequestCancelled);
  });
  const auto handled = invoke_custom_provider(
    CustomProviderInvocation{
      .handle = handle,
      .object = object,
      .resource = resource,
      .resolved_url = std::move(resolved_url),
      .callback = provider_callback,
      .user_data = user_data,
    }
  );
  return handled ? std::move(request) : nullptr;
}

auto complete_resource_request(
  mln_resource_request handle, const mln_resource_response* response
) -> mln_status {
  if (response == nullptr) {
    set_thread_error("response must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    response->size >= sizeof(mln_resource_response) &&
    (!validate_c_string_view(response->etag, "etag") ||
     !validate_c_string_view(response->error_message, "error_message"))
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = handle_table<ResourceRequestObject>().lease(handle);
  if (live == nullptr) return recorded_handle_fault_status();
  {
    const std::scoped_lock lock(live->mutex);
    if (live->retired) {
      return report_handle_fault(
        HandleTraits<ResourceRequestObject>::kind, handle, HandleFault::Stale
      );
    }
    if (live->completed) {
      set_thread_error("resource request is already completed");
      return MLN_STATUS_INVALID_STATE;
    }
    if (live->cancelled) {
      set_thread_error("resource request is cancelled");
      return MLN_STATUS_INVALID_STATE;
    }
    auto native_response = response_from_abi(*response);
    // Revalidation delivers the cached bytes that MapLibre withheld, as its
    // online source does for a 304.
    if (native_response.notModified && live->prior_data != nullptr) {
      native_response.data = live->prior_data;
      if (!native_response.modified)
        native_response.modified = live->prior_modified;
      if (!native_response.etag) native_response.etag = live->prior_etag;
      native_response.notModified = false;
    }
    live->completed = true;
    try {
      live->actor.invoke(
        &mln::FileSourceRequest::setResponse, std::move(native_response)
      );
    } catch (...) {
      set_thread_error("resource request can no longer accept a response");
      return MLN_STATUS_INVALID_STATE;
    }
  }
  // An answered request consumes its handle. It is not cancelled, so no
  // cancel callback runs that the retirement would wait for.
  retire_request(handle);
  return MLN_STATUS_OK;
}

auto pass_through_resource_request(mln_resource_request handle) -> mln_status {
  const auto live = handle_table<ResourceRequestObject>().lease(handle);
  if (live == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{live->mutex};
    if (live->retired) {
      return report_handle_fault(
        HandleTraits<ResourceRequestObject>::kind, handle, HandleFault::Stale
      );
    }
    if (live->completed || live->cancelled) {
      set_thread_error("resource request is completed or cancelled");
      return MLN_STATUS_INVALID_STATE;
    }
    live->loader->schedule(ProviderRequest::Decision::PassThrough);
    live->completed = true;
  }
  // As in complete_resource_request(), the answer consumes the handle.
  retire_request(handle);
  return MLN_STATUS_OK;
}

auto resource_request_cancelled(
  mln_resource_request handle, bool* out_cancelled
) -> mln_status {
  if (out_cancelled == nullptr) {
    set_thread_error("out_cancelled must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live = handle_table<ResourceRequestObject>().lease(handle);
  if (live == nullptr) return recorded_handle_fault_status();
  const std::scoped_lock lock(live->mutex);
  if (live->retired) {
    return report_handle_fault(
      HandleTraits<ResourceRequestObject>::kind, handle, HandleFault::Stale
    );
  }
  *out_cancelled = live->cancelled && !live->completed;
  return MLN_STATUS_OK;
}

auto set_resource_request_cancel_callback(
  mln_resource_request handle,
  const mln_resource_request_cancel_handler* handler, bool* out_cancelled
) -> mln_status {
  if (handler == nullptr) {
    set_thread_error("cancel handler must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (handler->size < sizeof(mln_resource_request_cancel_handler)) {
    set_thread_error("mln_resource_request_cancel_handler.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_cancelled == nullptr) {
    set_thread_error("out_cancelled must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto queue = std::shared_ptr<mln::delivery::DeliveryQueueState>{};
  const auto queue_status = mln::delivery::resolve_registration_queue(
    "cancel handler", handler->queue, handler->callback != nullptr, queue
  );
  if (queue_status != MLN_STATUS_OK) return queue_status;
  const auto live = handle_table<ResourceRequestObject>().lease(handle);
  if (live == nullptr) return recorded_handle_fault_status();
  // A queue handler's registration stands in for user_data. One the request
  // does not store is freed without retiring.
  using Registration = mln::delivery::QueuedRegistration;
  auto registration = std::unique_ptr<Registration>{};
  if (queue != nullptr) {
    registration = std::make_unique<Registration>(
      std::move(queue), handler->user_data, handler->release_user_data,
      std::initializer_list<Registration::Callback>{
        Registration::callback<mln::delivery::ResourceRequestCancelArguments>(
          handler->callback
        )
      }
    );
    registration->reserve(
      mln::delivery::ResourceRequestCancelArguments{.handle = handle}
    );
  }
  const std::scoped_lock lock(live->mutex);
  if (live->retired) {
    return report_handle_fault(
      HandleTraits<ResourceRequestObject>::kind, handle, HandleFault::Stale
    );
  }
  if (live->cancel_callback_registered) {
    set_thread_error("resource request already has a cancel callback");
    return MLN_STATUS_INVALID_STATE;
  }
  live->cancel_callback_registered = true;
  *out_cancelled = live->cancelled && !live->completed;
  if (*out_cancelled) return MLN_STATUS_OK;
  if (registration != nullptr) {
    live->cancel_callback = &queue_request_cancel;
    live->cancel_release = &mln::delivery::QueuedRegistration::retire;
    live->cancel_user_data = registration.release();
  } else {
    live->cancel_callback = handler->callback;
    live->cancel_release = handler->release_user_data;
    live->cancel_user_data = handler->user_data;
  }
  return MLN_STATUS_OK;
}

void release_resource_request(mln_resource_request handle) noexcept {
  if (
    const auto object = handle_table<ResourceRequestObject>().try_lease(handle)
  ) {
    const std::scoped_lock lock(object->mutex);
    if (!object->retired) {
      if (object->claimed) {
        fail_locked(*object, ProviderRequest::Failure::Released);
      } else if (object->deciding) {
        object->released_while_deciding = true;
      }
    }
  }
  retire_request(handle);
}

}  // namespace mln::core
