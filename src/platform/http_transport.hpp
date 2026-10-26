// What the HTTP file sources that run their own client share: the headers a
// request sends, how a reply becomes a Response, and the delivery of that
// Response on the requesting thread. The browser's source leaves both to the
// browser instead.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <mln/storage/file_source.hpp>
#include <mln/storage/resource.hpp>
#include <mln/storage/response.hpp>
#include <mln/util/async_task.hpp>
#include <mln/util/chrono.hpp>
#include <mln/util/http_header.hpp>
#include <mln/util/string.hpp>
#include <mln/util/url.hpp>

#include "maplibre_native_c.h"
#include "runtime/runtime.hpp"

namespace mln::platform::http {

// A header that a redirect to another origin drops when `sensitive`, as every
// header the host's transform adds is.
struct RequestHeader {
  std::string name;
  std::string value;
  bool sensitive = false;
};

// The reply headers that a Response takes.
struct ReplyHeaders {
  std::optional<std::string> etag;
  std::optional<std::string> modified;
  std::optional<std::string> cache_control;
  std::optional<std::string> expires;
  std::optional<std::string> retry_after;
  std::optional<std::string> x_rate_limit_reset;
};

inline auto is_http_url(const std::string& url) -> bool {
  const auto parsed = util::URL{url};
  const auto scheme =
    std::string_view{url}.substr(parsed.scheme.first, parsed.scheme.second);
  auto equals_ignoring_ascii_case = [scheme](std::string_view expected) {
    if (scheme.size() != expected.size()) return false;
    for (std::size_t index = 0; index < scheme.size(); ++index) {
      const auto actual = static_cast<unsigned char>(scheme[index]);
      const auto lowered = static_cast<char>(
        actual >= 'A' && actual <= 'Z' ? actual + ('a' - 'A') : actual
      );
      if (lowered != expected[index]) return false;
    }
    return true;
  };
  return equals_ignoring_ascii_case("http") ||
         equals_ignoring_ascii_case("https");
}

inline auto resource_kind_to_abi(Resource::Kind kind) -> mln_resource_kind {
  switch (kind) {
    case Resource::Kind::Style:
      return MLN_RESOURCE_KIND_STYLE;
    case Resource::Kind::Source:
      return MLN_RESOURCE_KIND_SOURCE;
    case Resource::Kind::Tile:
      return MLN_RESOURCE_KIND_TILE;
    case Resource::Kind::Glyphs:
      return MLN_RESOURCE_KIND_GLYPHS;
    case Resource::Kind::SpriteImage:
      return MLN_RESOURCE_KIND_SPRITE_IMAGE;
    case Resource::Kind::SpriteJSON:
      return MLN_RESOURCE_KIND_SPRITE_JSON;
    case Resource::Kind::Image:
      return MLN_RESOURCE_KIND_IMAGE;
    case Resource::Kind::Unknown:
    default:
      return MLN_RESOURCE_KIND_UNKNOWN;
  }
}

// The headers a request for `resource` at `url` sends: its range and
// revalidation headers, the user agent, then the host's transform.
inline auto request_headers(
  const Resource& resource, const std::string& url, void* platform_context
) -> std::vector<RequestHeader> {
  auto headers = std::vector<RequestHeader>{};
  if (resource.dataRange) {
    headers.push_back(
      {"Range", std::string{"bytes="} +
                  std::to_string(resource.dataRange->first) + "-" +
                  std::to_string(resource.dataRange->second)}
    );
  }

  if (resource.priorEtag) {
    headers.push_back({"If-None-Match", *resource.priorEtag});
  } else if (resource.priorModified) {
    headers.push_back(
      {"If-Modified-Since", util::rfc1123(*resource.priorModified)}
    );
  }

  headers.push_back({"User-Agent", "MapLibreNative/1.0"});

  if (is_http_url(url)) {
    auto transformed = core::invoke_http_header_transform(
      platform_context, resource_kind_to_abi(resource.kind), url.c_str()
    );
    for (auto& [name, value] : transformed) {
      headers.push_back({std::move(name), std::move(value), true});
    }
  }
  return headers;
}

inline auto error_response(Response::Error::Reason reason, std::string message)
  -> Response {
  auto result = Response{};
  result.error = std::make_unique<Response::Error>(reason, std::move(message));
  return result;
}

// The Response for a reply with `status_code`, whose body counts only for 200
// and 206.
inline auto make_response(
  const Resource& resource, std::uint16_t status_code,
  const ReplyHeaders& headers, std::shared_ptr<const std::string> body
) -> Response {
  auto result = Response{};
  if (headers.etag) {
    result.etag = headers.etag;
  }
  if (headers.modified) {
    result.modified = util::parseTimestamp(headers.modified->c_str());
  }

  auto cache_control_expiration = std::optional<Timestamp>{};
  if (headers.cache_control) {
    const auto parsed = mln::http::CacheControl::parse(*headers.cache_control);
    cache_control_expiration = parsed.toTimePoint();
    if (cache_control_expiration) {
      result.expires = cache_control_expiration;
    }
    result.mustRevalidate = parsed.mustRevalidate;
  }
  if (headers.expires && !cache_control_expiration) {
    result.expires = util::parseTimestamp(headers.expires->c_str());
  }

  const auto code = status_code;
  if (code == 200 || code == 206) {
    result.data = body ? std::move(body) : std::make_shared<std::string>();
  } else if (
    code == 204 || (code == 404 && resource.kind == Resource::Kind::Tile)
  ) {
    result.noContent = true;
  } else if (code == 304) {
    result.notModified = true;
  } else if (code == 404) {
    result.error = std::make_unique<Response::Error>(
      Response::Error::Reason::NotFound, "HTTP status code 404"
    );
  } else if (code == 429) {
    result.error = std::make_unique<Response::Error>(
      Response::Error::Reason::RateLimit, "HTTP status code 429",
      mln::http::parseRetryHeaders(
        headers.retry_after, headers.x_rate_limit_reset
      )
    );
  } else if (code >= 500 && code < 600) {
    result.error = std::make_unique<Response::Error>(
      Response::Error::Reason::Server,
      std::string{"HTTP status code "} + util::toString(code)
    );
  } else {
    result.error = std::make_unique<Response::Error>(
      Response::Error::Reason::Other,
      std::string{"HTTP status code "} + util::toString(code)
    );
  }
  return result;
}

// Carries one request's Response from the client's thread to the requesting
// thread, and drops it once the request is canceled.
class Delivery : public std::enable_shared_from_this<Delivery> {
 public:
  Delivery(Resource resource_, FileSource::Callback callback_)
      : resource(std::move(resource_)), callback(std::move(callback_)) {}

  // Runs on the requesting thread, whose run loop delivers the Response.
  void start() {
    auto weak = std::weak_ptr<Delivery>{shared_from_this()};
    async = std::make_unique<util::AsyncTask>([weak] {
      if (auto delivery = weak.lock()) {
        delivery->deliver();
      }
    });
  }

  void cancel() {
    auto async_to_destroy = std::unique_ptr<util::AsyncTask>{};
    {
      std::scoped_lock lock(mutex);
      canceled = true;
      callback = nullptr;
      async_to_destroy = std::move(async);
    }
  }

  [[nodiscard]] auto is_canceled() -> bool {
    std::scoped_lock lock(mutex);
    return canceled;
  }

  void complete(Response response_) {
    std::scoped_lock lock(mutex);
    if (canceled || async == nullptr) {
      return;
    }
    response = std::move(response_);
    async->send();
  }

  const Resource resource;

 private:
  void deliver() {
    auto callback_copy = FileSource::Callback{};
    auto response_copy = Response{};
    {
      std::scoped_lock lock(mutex);
      if (canceled || !callback) {
        return;
      }
      callback_copy = callback;
      response_copy = response;
    }
    callback_copy(response_copy);
  }

  FileSource::Callback callback;
  Response response;
  std::unique_ptr<util::AsyncTask> async;
  std::mutex mutex;
  bool canceled = false;
};

}  // namespace mln::platform::http
