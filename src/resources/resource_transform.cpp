#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include <mln/storage/response.hpp>
#include <mln/util/chrono.hpp>

#include "resources/resource_transform.hpp"

#include "diagnostics/diagnostics.hpp"
#include "diagnostics/string_view.hpp"
#include "execution/process_exit.hpp"
#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

// MapLibre's finished callback posts to the waiting request's mailbox, which
// drops the answer once MapLibre cancels the request, so the host may answer
// from any thread at any time.
struct ResourceTransformRequestObject {
  std::mutex mutex;
  // Empty once an answer is posted.
  mln::ResourceTransform::FinishedCallback finished;
};

// A host answers on its own thread while another releases, so the object must
// outlive a racing release.
template <>
struct HandleTraits<ResourceTransformRequestObject> {
  static constexpr auto kind = HandleKind::ResourceTransformRequest;
  static constexpr auto leasable = true;
};

namespace {

auto take_answer(ResourceTransformRequestObject& object)
  -> mln::ResourceTransform::FinishedCallback {
  const auto lock = std::scoped_lock{object.mutex};
  return std::exchange(object.finished, nullptr);
}

auto unanswered_error() -> mln::ResourceTransform::Result {
  return mln::unexpected<mln::Response::Error>(mln::Response::Error{
    mln::Response::Error::Reason::Other,
    "the resource transform released the request without an answer"
  });
}

auto error_reason_from_abi(mln_resource_error_reason reason)
  -> std::optional<mln::Response::Error::Reason> {
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
      return mln::Response::Error::Reason::Other;
    default:
      return std::nullopt;
  }
}

auto from_unix_ms(std::int64_t unix_ms) -> mln::Timestamp {
  return std::chrono::time_point_cast<mln::Seconds>(
    std::chrono::time_point<
      std::chrono::system_clock, std::chrono::milliseconds>{
      std::chrono::milliseconds{unix_ms}
    }
  );
}

// Answers a live request once and consumes its handle.
auto answer(
  mln_resource_transform_request request, mln::ResourceTransform::Result result
) -> mln_status {
  const auto live =
    handle_table<ResourceTransformRequestObject>().lease(request);
  if (live == nullptr) return recorded_handle_fault_status();
  {
    const auto lock = std::scoped_lock{live->mutex};
    if (!live->finished) {
      set_thread_error("resource transform request is already answered");
      return MLN_STATUS_INVALID_STATE;
    }
    // MapLibre's callback posts the answer, which can throw. The callback
    // stays until the post succeeds, so the release that follows a failed
    // answer can still fail the request.
    live->finished(std::move(result));
    live->finished = nullptr;
  }
  static_cast<void>(
    handle_table<ResourceTransformRequestObject>().remove(request)
  );
  return MLN_STATUS_OK;
}

}  // namespace

auto request_resource_transform(
  mln_resource_transform_callback callback, void* user_data,
  mln_resource_kind kind, const std::string& url,
  mln::ResourceTransform::FinishedCallback finished
) -> void {
  auto object = std::shared_ptr<ResourceTransformRequestObject>{};
  auto request = mln_resource_transform_request{MLN_HANDLE_NULL};
  try {
    object = std::make_shared<ResourceTransformRequestObject>();
    object->finished = std::move(finished);
    request = handle_table<ResourceTransformRequestObject>().insert(object);
  } catch (...) {
    if (object != nullptr) finished = std::move(object->finished);
    finished(unanswered_error());
    return;
  }
  if (process_exiting()) {
    release_resource_transform_request(request);
    return;
  }
  callback(
    user_data, kind, mln_buffer_view{.data = url.data(), .size = url.size()},
    request
  );
}

auto complete_resource_transform_request(
  mln_resource_transform_request request, mln_buffer_view url
) -> mln_status {
  if (!validate_c_string_view(url, "url")) return MLN_STATUS_INVALID_ARGUMENT;
  if (url.size == 0) {
    set_thread_error("url must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return answer(request, string_from_view(url));
}

auto fail_resource_transform_request(
  mln_resource_transform_request request,
  const mln_resource_transform_error* error
) -> mln_status {
  if (error == nullptr) {
    set_thread_error("error must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (error->size < sizeof(mln_resource_transform_error)) {
    set_thread_error("mln_resource_transform_error.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((error->fields & ~MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER) != 0U) {
    set_thread_error("mln_resource_transform_error.fields has unknown bits");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto reason = error_reason_from_abi(error->reason);
  if (!reason) {
    set_thread_error("mln_resource_transform_error.reason is unknown");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!validate_c_string_view(error->message, "message")) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto retry_after = std::optional<mln::Timestamp>{};
  if ((error->fields & MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER) != 0U) {
    retry_after = from_unix_ms(error->retry_after_unix_ms);
  }
  auto message = error->message.size == 0
                   ? std::string{"resource transform failed"}
                   : string_from_view(error->message);
  return answer(
    request, mln::unexpected<mln::Response::Error>(
               mln::Response::Error{*reason, std::move(message), retry_after}
             )
  );
}

void release_resource_transform_request(
  mln_resource_transform_request request
) noexcept {
  try {
    const auto object =
      handle_table<ResourceTransformRequestObject>().remove(request);
    if (object == nullptr) return;
    if (const auto finished = take_answer(*object)) {
      finished(unanswered_error());
    }
  } catch (...) {
    // An answer that cannot be posted leaves MapLibre to cancel the request.
  }
}

}  // namespace mln::core
