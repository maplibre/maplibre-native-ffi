#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <mbgl/storage/file_source.hpp>
#include <mbgl/storage/http_file_source.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/async_request.hpp>
#include <mbgl/util/async_task.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/client_options.hpp>
#include <mbgl/util/http_header.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/url.hpp>

extern "C" {

struct MlnRustHttpHeader {
  const char* name;
  const char* value;
};

struct MlnRustHttpResponse {
  uint16_t status_code;
  uint8_t error_reason;
  uint8_t* data;
  std::size_t data_len;
  char* error;
  char* etag;
  char* modified;
  char* cache_control;
  char* expires;
  char* retry_after;
  char* x_rate_limit_reset;
};

using MlnRustHttpCallback = void (*)(void*, MlnRustHttpResponse);

auto mln_rust_http_request_start(
  const char* url, const MlnRustHttpHeader* headers, std::size_t headerCount,
  MlnRustHttpCallback callback, void* userData
) -> void*;
void mln_rust_http_request_cancel(void* handle);
void mln_rust_http_request_free(void* handle);
void mln_rust_http_response_free(MlnRustHttpResponse response);

}  // extern "C"

namespace {

constexpr auto rustHttpErrorConnection = uint8_t{1};

auto optionalCString(char* value) -> std::optional<std::string> {
  if (value == nullptr) {
    return std::nullopt;
  }

  return std::string{value};
}

auto hasSuffix(const std::string& value, const std::string& suffix) -> bool {
  return value.ends_with(suffix);
}

auto isValidMapboxEndpoint(const std::string& url) -> bool {
  const auto parsed = mbgl::util::URL{url};
  const auto host = url.substr(parsed.domain.first, parsed.domain.second);
  return host == "mapbox.com" || host == "mapbox.cn" ||
         hasSuffix(host, ".mapbox.com") || hasSuffix(host, ".mapbox.cn");
}

auto offlineURL(const mbgl::Resource& resource) -> std::string {
  auto url = resource.url;
  if (
    resource.usage != mbgl::Resource::Usage::Offline ||
    !isValidMapboxEndpoint(url)
  ) {
    return url;
  }

  const auto fragment = url.find('#');
  const auto query = url.find('?');
  const auto separator = query == std::string::npos ||
                             (fragment != std::string::npos && fragment < query)
                           ? '?'
                           : '&';
  url.insert(
    fragment == std::string::npos ? url.size() : fragment,
    std::string{separator} + "offline=true"
  );
  return url;
}

class RustHttpResponse {
 public:
  explicit RustHttpResponse(MlnRustHttpResponse response_)
      : response(response_) {}
  RustHttpResponse(const RustHttpResponse&) = delete;
  auto operator=(const RustHttpResponse&) -> RustHttpResponse& = delete;
  RustHttpResponse(RustHttpResponse&&) = delete;
  auto operator=(RustHttpResponse&&) -> RustHttpResponse& = delete;

  ~RustHttpResponse() { mln_rust_http_response_free(response); }

  [[nodiscard]] auto get() const -> const MlnRustHttpResponse& {
    return response;
  }

 private:
  MlnRustHttpResponse response;
};

class RustHttpRequestHandle {
 public:
  explicit RustHttpRequestHandle(void* handle_) : handle(handle_) {}
  RustHttpRequestHandle(const RustHttpRequestHandle&) = delete;
  auto operator=(const RustHttpRequestHandle&)
    -> RustHttpRequestHandle& = delete;
  RustHttpRequestHandle(RustHttpRequestHandle&&) = delete;
  auto operator=(RustHttpRequestHandle&&) -> RustHttpRequestHandle& = delete;

  ~RustHttpRequestHandle() {
    if (handle != nullptr) {
      mln_rust_http_request_cancel(handle);
      mln_rust_http_request_free(handle);
    }
  }

 private:
  void* handle = nullptr;
};

}  // namespace

namespace mbgl {

class HTTPFileSource::Impl {
 public:
  Impl(
    const ResourceOptions& resourceOptions, const ClientOptions& clientOptions
  )
      : resource_options_(resourceOptions.clone()),
        client_options_(clientOptions.clone()) {}

  void setResourceOptions(ResourceOptions options) {
    resource_options_ = std::move(options);
  }

  auto getResourceOptions() -> ResourceOptions {
    return resource_options_.clone();
  }

  void setClientOptions(ClientOptions options) {
    client_options_ = std::move(options);
  }

  auto getClientOptions() -> ClientOptions { return client_options_.clone(); }

 private:
  ResourceOptions resource_options_;
  ClientOptions client_options_;
};

class HTTPRequestState : public std::enable_shared_from_this<HTTPRequestState> {
 public:
  HTTPRequestState(Resource resource_, FileSource::Callback callback_)
      : resource(std::move(resource_)), callback(std::move(callback_)) {}

  void startAsyncTask() {
    auto weak = std::weak_ptr<HTTPRequestState>{shared_from_this()};
    async = std::make_unique<util::AsyncTask>([weak] {
      if (auto state = weak.lock()) {
        state->deliver();
      }
    });
  }

  void cancel() {
    auto asyncToDestroy = std::unique_ptr<util::AsyncTask>{};
    {
      std::scoped_lock lock(mutex);
      canceled = true;
      callback = nullptr;
      asyncToDestroy = std::move(async);
    }
  }

  void complete(const MlnRustHttpResponse& rustResponse) {
    std::scoped_lock lock(mutex);
    if (canceled || async == nullptr) {
      return;
    }

    response = makeResponse(rustResponse);
    async->send();
  }

 private:
  auto makeResponse(const MlnRustHttpResponse& rustResponse) const -> Response {
    auto result = Response{};

    auto etag = optionalCString(rustResponse.etag);
    if (etag) {
      result.etag = std::move(etag);
    }

    auto modified = optionalCString(rustResponse.modified);
    if (modified) {
      result.modified = util::parseTimestamp(modified->c_str());
    }

    auto cacheControlExpiration = std::optional<Timestamp>{};
    auto cacheControl = optionalCString(rustResponse.cache_control);
    if (cacheControl) {
      const auto parsed = http::CacheControl::parse(*cacheControl);
      cacheControlExpiration = parsed.toTimePoint();
      if (cacheControlExpiration) {
        result.expires = cacheControlExpiration;
      }
      result.mustRevalidate = parsed.mustRevalidate;
    }

    auto expires = optionalCString(rustResponse.expires);
    if (expires && !cacheControlExpiration) {
      result.expires = util::parseTimestamp(expires->c_str());
    }

    if (rustResponse.error != nullptr) {
      auto reason = Response::Error::Reason::Other;
      if (rustResponse.error_reason == rustHttpErrorConnection) {
        reason = Response::Error::Reason::Connection;
      }
      result.error = std::make_unique<Response::Error>(
        reason, std::string{rustResponse.error}
      );
      return result;
    }

    const auto code = rustResponse.status_code;
    if (code == 200 || code == 206) {
      if (rustResponse.data_len == 0) {
        result.data = std::make_shared<std::string>();
      } else if (rustResponse.data == nullptr) {
        result.error = std::make_unique<Response::Error>(
          Response::Error::Reason::Other, "HTTP response missing body data"
        );
        return result;
      } else {
        result.data = std::make_shared<std::string>(
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          reinterpret_cast<const char*>(rustResponse.data),
          rustResponse.data_len
        );
      }
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
        http::parseRetryHeaders(
          optionalCString(rustResponse.retry_after),
          optionalCString(rustResponse.x_rate_limit_reset)
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

  void deliver() {
    auto callbackCopy = FileSource::Callback{};
    auto responseCopy = Response{};

    {
      std::scoped_lock lock(mutex);
      if (canceled || !callback) {
        return;
      }

      callbackCopy = callback;
      responseCopy = response;
    }

    callbackCopy(responseCopy);
  }

  Resource resource;
  FileSource::Callback callback;
  Response response;
  std::unique_ptr<util::AsyncTask> async;
  std::mutex mutex;
  bool canceled = false;
};

class HTTPRequest : public AsyncRequest {
 public:
  HTTPRequest(const Resource& resource, FileSource::Callback callback)
      : state(
          std::make_shared<HTTPRequestState>(resource, std::move(callback))
        ) {
    state->startAsyncTask();

    auto headers = makeHeaders(resource);
    auto url = offlineURL(resource);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto* stateForCallback = new std::shared_ptr<HTTPRequestState>{state};
    handle =
      std::make_unique<RustHttpRequestHandle>(mln_rust_http_request_start(
        url.c_str(), headers.data(), headers.size(), onComplete,
        stateForCallback
      ));
  }

  HTTPRequest(const HTTPRequest&) = delete;
  auto operator=(const HTTPRequest&) -> HTTPRequest& = delete;
  HTTPRequest(HTTPRequest&&) = delete;
  auto operator=(HTTPRequest&&) -> HTTPRequest& = delete;

  ~HTTPRequest() override { state->cancel(); }

 private:
  static auto makeHeaders(const Resource& resource)
    -> std::vector<MlnRustHttpHeader> {
    header_storage.clear();
    auto headers = std::vector<MlnRustHttpHeader>{};

    if (resource.dataRange) {
      header_storage.emplace_back(
        "Range", std::string{"bytes="} +
                   std::to_string(resource.dataRange->first) + "-" +
                   std::to_string(resource.dataRange->second)
      );
    }

    if (resource.priorEtag) {
      header_storage.emplace_back("If-None-Match", *resource.priorEtag);
    } else if (resource.priorModified) {
      header_storage.emplace_back(
        "If-Modified-Since", util::rfc1123(*resource.priorModified)
      );
    }

    header_storage.emplace_back("User-Agent", "MapLibreNative/1.0");

    headers.reserve(header_storage.size());
    for (const auto& [name, value] : header_storage) {
      headers.push_back({name.c_str(), value.c_str()});
    }

    return headers;
  }

  static void onComplete(void* userData, MlnRustHttpResponse response) {
    auto stateHolder = std::unique_ptr<std::shared_ptr<HTTPRequestState>>{
      static_cast<std::shared_ptr<HTTPRequestState>*>(userData),
    };
    auto rustResponse = RustHttpResponse{response};
    (*stateHolder)->complete(rustResponse.get());
  }

  static thread_local std::vector<std::pair<std::string, std::string>>
    header_storage;

  std::shared_ptr<HTTPRequestState> state;
  std::unique_ptr<RustHttpRequestHandle> handle;
};

thread_local std::vector<std::pair<std::string, std::string>>
  HTTPRequest::header_storage;

HTTPFileSource::HTTPFileSource(
  const ResourceOptions& resourceOptions, const ClientOptions& clientOptions
)
    : impl(std::make_unique<Impl>(resourceOptions, clientOptions)) {}

HTTPFileSource::~HTTPFileSource() = default;

auto HTTPFileSource::request(const Resource& resource, Callback callback)
  -> std::unique_ptr<AsyncRequest> {
  return std::make_unique<HTTPRequest>(resource, std::move(callback));
}

void HTTPFileSource::setResourceOptions(ResourceOptions options) {
  impl->setResourceOptions(std::move(options));
}

auto HTTPFileSource::getResourceOptions() -> ResourceOptions {
  return impl->getResourceOptions();
}

void HTTPFileSource::setClientOptions(ClientOptions options) {
  impl->setClientOptions(std::move(options));
}

auto HTTPFileSource::getClientOptions() -> ClientOptions {
  return impl->getClientOptions();
}

}  // namespace mbgl
