#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <mln/storage/file_source.hpp>
#include <mln/storage/http_file_source.hpp>
#include <mln/storage/resource.hpp>
#include <mln/storage/resource_options.hpp>
#include <mln/storage/response.hpp>
#include <mln/util/async_request.hpp>
#include <mln/util/client_options.hpp>

#include "../http_transport.hpp"
#include "../offline_url.hpp"

extern "C" {

struct MlnRustHttpHeader {
  const char* name;
  const char* value;
  bool sensitive;
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

auto mlnffi_rust_http_request_start(
  const char* url, const MlnRustHttpHeader* headers, std::size_t headerCount,
  MlnRustHttpCallback callback, void* userData
) -> void*;
void mlnffi_rust_http_request_cancel(void* handle);
void mlnffi_rust_http_request_free(void* handle);
void mlnffi_rust_http_response_free(MlnRustHttpResponse response);

}  // extern "C"

namespace {

constexpr auto rustHttpErrorConnection = uint8_t{1};

auto optionalCString(char* value) -> std::optional<std::string> {
  if (value == nullptr) {
    return std::nullopt;
  }

  return std::string{value};
}

auto makeResponse(
  const mln::Resource& resource, const MlnRustHttpResponse& rustResponse
) -> mln::Response {
  using Reason = mln::Response::Error::Reason;
  if (rustResponse.error != nullptr) {
    return mln::platform::http::error_response(
      rustResponse.error_reason == rustHttpErrorConnection ? Reason::Connection
                                                           : Reason::Other,
      std::string{rustResponse.error}
    );
  }

  auto body = std::shared_ptr<const std::string>{};
  if (rustResponse.data_len != 0) {
    if (rustResponse.data == nullptr) {
      return mln::platform::http::error_response(
        Reason::Other, "HTTP response missing body data"
      );
    }
    body = std::make_shared<std::string>(
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      reinterpret_cast<const char*>(rustResponse.data), rustResponse.data_len
    );
  }

  return mln::platform::http::make_response(
    resource, rustResponse.status_code,
    {
      .etag = optionalCString(rustResponse.etag),
      .modified = optionalCString(rustResponse.modified),
      .cache_control = optionalCString(rustResponse.cache_control),
      .expires = optionalCString(rustResponse.expires),
      .retry_after = optionalCString(rustResponse.retry_after),
      .x_rate_limit_reset = optionalCString(rustResponse.x_rate_limit_reset),
    },
    std::move(body)
  );
}

class RustHttpResponse {
 public:
  explicit RustHttpResponse(MlnRustHttpResponse response_)
      : response(response_) {}
  RustHttpResponse(const RustHttpResponse&) = delete;
  auto operator=(const RustHttpResponse&) -> RustHttpResponse& = delete;
  RustHttpResponse(RustHttpResponse&&) = delete;
  auto operator=(RustHttpResponse&&) -> RustHttpResponse& = delete;

  ~RustHttpResponse() { mlnffi_rust_http_response_free(response); }

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
      mlnffi_rust_http_request_cancel(handle);
      mlnffi_rust_http_request_free(handle);
    }
  }

 private:
  void* handle = nullptr;
};

}  // namespace

namespace mln {

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

class HTTPRequest : public AsyncRequest {
 public:
  HTTPRequest(
    const Resource& resource, void* platformContext,
    FileSource::Callback callback
  )
      : delivery(
          std::make_shared<platform::http::Delivery>(
            resource, std::move(callback)
          )
        ) {
    delivery->start();

    auto url = platform::offline_url(resource);
    const auto headers =
      platform::http::request_headers(resource, url, platformContext);
    auto rustHeaders = std::vector<MlnRustHttpHeader>{};
    rustHeaders.reserve(headers.size());
    for (const auto& header : headers) {
      rustHeaders.push_back(
        {header.name.c_str(), header.value.c_str(), header.sensitive}
      );
    }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto* deliveryForCallback =
      new std::shared_ptr<platform::http::Delivery>{delivery};
    handle =
      std::make_unique<RustHttpRequestHandle>(mlnffi_rust_http_request_start(
        url.c_str(), rustHeaders.data(), rustHeaders.size(), onComplete,
        deliveryForCallback
      ));
  }

  HTTPRequest(const HTTPRequest&) = delete;
  auto operator=(const HTTPRequest&) -> HTTPRequest& = delete;
  HTTPRequest(HTTPRequest&&) = delete;
  auto operator=(HTTPRequest&&) -> HTTPRequest& = delete;

  ~HTTPRequest() override { delivery->cancel(); }

 private:
  static void onComplete(void* userData, MlnRustHttpResponse response) {
    auto deliveryHolder =
      std::unique_ptr<std::shared_ptr<platform::http::Delivery>>{
        static_cast<std::shared_ptr<platform::http::Delivery>*>(userData),
    };
    auto rustResponse = RustHttpResponse{response};
    auto& delivery = **deliveryHolder;
    delivery.complete(makeResponse(delivery.resource, rustResponse.get()));
  }

  std::shared_ptr<platform::http::Delivery> delivery;
  std::unique_ptr<RustHttpRequestHandle> handle;
};

HTTPFileSource::HTTPFileSource(
  const ResourceOptions& resourceOptions, const ClientOptions& clientOptions
)
    : impl(std::make_unique<Impl>(resourceOptions, clientOptions)) {}

HTTPFileSource::~HTTPFileSource() = default;

auto HTTPFileSource::request(const Resource& resource, Callback callback)
  -> std::unique_ptr<AsyncRequest> {
  return std::make_unique<HTTPRequest>(
    resource, impl->getResourceOptions().platformContext(), std::move(callback)
  );
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

}  // namespace mln
