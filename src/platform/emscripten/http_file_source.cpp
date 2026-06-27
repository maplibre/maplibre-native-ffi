// Browser HTTP via emscripten_fetch. We keep this in C++ rather than the Rust
// ureq stack used on desktop: fetch respects CORS, cookies, and the page cache;
// ureq opens raw sockets that WASM cannot use. A future Rust wrapper around
// fetch would not shrink the binary compared to this direct binding.

#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <mbgl/storage/http_file_source.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/async_request.hpp>
#include <mbgl/util/async_task.hpp>
#include <mbgl/util/client_options.hpp>
#include <mbgl/util/string.hpp>

#include <emscripten/fetch.h>

namespace mbgl {

namespace {

auto makeResponse(const Resource& resource, emscripten_fetch_t* fetch)
  -> Response {
  auto result = Response{};
  const auto code = fetch->status;

  if (fetch->numBytes > 0 && fetch->data != nullptr) {
    result.data = std::make_shared<std::string>(
      reinterpret_cast<const char*>(fetch->data),
      static_cast<std::size_t>(fetch->numBytes)
    );
  }

  if (code == 200 || code == 206) {
    if (!result.data) {
      result.error = std::make_unique<Response::Error>(
        Response::Error::Reason::Other, "HTTP response missing body data"
      );
    }
  } else if (
    code == 204 || (code == 404 && resource.kind == Resource::Kind::Tile)
  ) {
    result.noContent = true;
  } else if (code == 404) {
    result.error = std::make_unique<Response::Error>(
      Response::Error::Reason::NotFound, "HTTP status code 404"
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

class FetchRequestState
    : public std::enable_shared_from_this<FetchRequestState> {
 public:
  FetchRequestState(Resource resource_, FileSource::Callback callback_)
      : resource(std::move(resource_)), callback(std::move(callback_)) {}

  void start() {
    async = std::make_unique<util::AsyncTask>([weak = weak_from_this()]() {
      if (const auto state = weak.lock()) {
        state->deliver();
      }
    });

    emscripten_fetch_attr_t attributes{};
    emscripten_fetch_attr_init(&attributes);
    std::strcpy(attributes.requestMethod, "GET");
    attributes.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attributes.onsuccess = [](emscripten_fetch_t* fetch) {
      onFetchComplete(fetch);
    };
    attributes.onerror = [](emscripten_fetch_t* fetch) {
      onFetchComplete(fetch);
    };
    attributes.userData =
      new std::shared_ptr<FetchRequestState>(shared_from_this());

    const auto url = resource.url;
    fetch = emscripten_fetch(&attributes, url.c_str());
  }

  void cancel() {
    auto* fetch_to_close = static_cast<emscripten_fetch_t*>(nullptr);
    {
      std::scoped_lock lock(mutex);
      canceled = true;
      callback = nullptr;
      fetch_to_close = fetch;
      fetch = nullptr;
    }
    if (fetch_to_close != nullptr) {
      emscripten_fetch_close(fetch_to_close);
    }
  }

 private:
  static void onFetchComplete(emscripten_fetch_t* fetch) {
    auto* holder_ptr =
      static_cast<std::shared_ptr<FetchRequestState>*>(fetch->userData);
    if (holder_ptr == nullptr) {
      return;
    }
    fetch->userData = nullptr;
    const auto holder = std::unique_ptr<std::shared_ptr<FetchRequestState>>{
      holder_ptr,
    };
    (*holder)->complete(fetch);
  }

  void complete(emscripten_fetch_t* completed_fetch) {
    auto should_deliver = false;
    {
      std::scoped_lock lock(mutex);
      if (canceled) {
        return;
      }
      response = makeResponse(resource, completed_fetch);
      fetch = nullptr;
      pending_delivery = shared_from_this();
      should_deliver = true;
    }
    emscripten_fetch_close(completed_fetch);
    if (should_deliver && async) {
      async->send();
    }
  }

  void deliver() {
    std::shared_ptr<FetchRequestState> keep_alive;
    FileSource::Callback callback_copy;
    Response response_copy;
    {
      std::scoped_lock lock(mutex);
      keep_alive = std::move(pending_delivery);
      if (canceled || !callback) {
        return;
      }
      callback_copy = callback;
      response_copy = response;
      callback = nullptr;
    }
    callback_copy(response_copy);
  }

  Resource resource;
  FileSource::Callback callback;
  Response response;
  std::unique_ptr<util::AsyncTask> async;
  std::shared_ptr<FetchRequestState> pending_delivery;
  emscripten_fetch_t* fetch = nullptr;
  std::mutex mutex;
  bool canceled = false;
};

class FetchRequest : public AsyncRequest {
 public:
  FetchRequest(Resource resource, FileSource::Callback callback)
      : state(
          std::make_shared<FetchRequestState>(
            std::move(resource), std::move(callback)
          )
        ) {
    state->start();
  }

  ~FetchRequest() override { state->cancel(); }

 private:
  std::shared_ptr<FetchRequestState> state;
};

}  // namespace

class HTTPFileSource::Impl {
 public:
  Impl(
    const ResourceOptions& resource_options, const ClientOptions& client_options
  )
      : resource_options_(resource_options.clone()),
        client_options_(client_options.clone()) {}

  void setResourceOptions(ResourceOptions options) {
    std::scoped_lock lock(resource_options_mutex_);
    resource_options_ = options.clone();
  }

  ResourceOptions getResourceOptions() {
    std::scoped_lock lock(resource_options_mutex_);
    return resource_options_.clone();
  }

  void setClientOptions(ClientOptions options) {
    std::scoped_lock lock(client_options_mutex_);
    client_options_ = options.clone();
  }

  ClientOptions getClientOptions() {
    std::scoped_lock lock(client_options_mutex_);
    return client_options_.clone();
  }

 private:
  friend HTTPFileSource;

  mutable std::mutex resource_options_mutex_;
  mutable std::mutex client_options_mutex_;
  ResourceOptions resource_options_;
  ClientOptions client_options_;
};

HTTPFileSource::HTTPFileSource(
  const ResourceOptions& resource_options, const ClientOptions& client_options
)
    : impl(std::make_unique<Impl>(resource_options, client_options)) {}

HTTPFileSource::~HTTPFileSource() = default;

std::unique_ptr<AsyncRequest> HTTPFileSource::request(
  const Resource& resource, Callback callback
) {
  return std::make_unique<FetchRequest>(resource, std::move(callback));
}

void HTTPFileSource::setResourceOptions(ResourceOptions options) {
  impl->setResourceOptions(std::move(options));
}

ResourceOptions HTTPFileSource::getResourceOptions() {
  return impl->getResourceOptions();
}

void HTTPFileSource::setClientOptions(ClientOptions options) {
  impl->setClientOptions(std::move(options));
}

ClientOptions HTTPFileSource::getClientOptions() {
  return impl->getClientOptions();
}

}  // namespace mbgl
