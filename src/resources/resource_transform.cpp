#include <memory>
#include <mutex>
#include <string>
#include <utility>

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
  explicit ResourceTransformRequestObject(std::string url_)
      : url(std::move(url_)) {}

  std::mutex mutex;
  std::string url;
  // Empty once the request is answered.
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

}  // namespace

auto request_resource_transform(
  mln_resource_transform_callback callback, void* user_data, std::uint32_t kind,
  const std::string& url, mln::ResourceTransform::FinishedCallback finished
) -> void {
  auto object = std::shared_ptr<ResourceTransformRequestObject>{};
  auto request = mln_resource_transform_request{MLN_HANDLE_NULL};
  try {
    object = std::make_shared<ResourceTransformRequestObject>(url);
    object->finished = std::move(finished);
    request = handle_table<ResourceTransformRequestObject>().insert(object);
  } catch (...) {
    if (object != nullptr) finished = std::move(object->finished);
    finished(url);
    return;
  }
  if (process_exiting()) {
    release_resource_transform_request(request);
    return;
  }
  callback(user_data, kind, url.c_str(), request);
}

auto complete_resource_transform_request(
  mln_resource_transform_request request, mln_buffer_view url
) -> mln_status {
  if (!validate_c_string_view(url, "url")) return MLN_STATUS_INVALID_ARGUMENT;
  if (url.size == 0) {
    set_thread_error("url must not be empty");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto live =
    handle_table<ResourceTransformRequestObject>().lease(request);
  if (live == nullptr) return recorded_handle_fault_status();
  auto replacement = string_from_view(url);
  const auto finished = take_answer(*live);
  if (!finished) {
    set_thread_error("resource transform request is already answered");
    return MLN_STATUS_INVALID_STATE;
  }
  finished(replacement);
  // The answer consumes the handle.
  static_cast<void>(
    handle_table<ResourceTransformRequestObject>().remove(request)
  );
  return MLN_STATUS_OK;
}

void release_resource_transform_request(
  mln_resource_transform_request request
) noexcept {
  try {
    const auto object =
      handle_table<ResourceTransformRequestObject>().remove(request);
    if (object == nullptr) return;
    if (const auto finished = take_answer(*object)) finished(object->url);
  } catch (...) {
    // An answer that cannot be posted leaves MapLibre to cancel the request.
  }
}

}  // namespace mln::core
