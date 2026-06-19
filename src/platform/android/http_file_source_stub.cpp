#include <memory>
#include <utility>

#include <mbgl/storage/http_file_source.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/async_request.hpp>
#include <mbgl/util/client_options.hpp>

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

HTTPFileSource::HTTPFileSource(
  const ResourceOptions& resourceOptions, const ClientOptions& clientOptions
)
    : impl(std::make_unique<Impl>(resourceOptions, clientOptions)) {}

HTTPFileSource::~HTTPFileSource() = default;

auto HTTPFileSource::request(const Resource& resource, Callback callback)
  -> std::unique_ptr<AsyncRequest> {
  static_cast<void>(resource);

  auto response = Response{};
  response.error = std::make_unique<Response::Error>(
    Response::Error::Reason::Other,
    "Android native HTTP is not implemented in this build"
  );
  callback(response);
  return nullptr;
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
