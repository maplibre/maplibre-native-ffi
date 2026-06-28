// Browser HTTP via emscripten_fetch. The transport stays browser-native so it
// respects CORS, cookies, and the page cache; this file owns the native request
// lifecycle and maps fetch results into MapLibre responses.

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
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
#include <mbgl/util/http_header.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/util.hpp>

#include <emscripten.h>
#include <emscripten/fetch.h>

namespace mbgl {
namespace {

// FETCH_LOAD_TO_MEMORY copies each completed XHR body into the fixed pthread
// wasm heap before C++ can drop obsolete responses, so bound active transport
// requests below OnlineFileSource's platform-wide concurrency limit.
constexpr auto maxActiveFetches = std::size_t{16};
std::atomic_bool traceHttpEnabled = false;
std::atomic_uint64_t nextRequestId = 1;

void traceHttp(const char* format, ...) {
  if (!traceHttpEnabled.load(std::memory_order_relaxed)) {
    return;
  }
  std::fprintf(stderr, "browser http: ");
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
  std::fprintf(stderr, "\n");
}

auto lowerASCII(std::string value) -> std::string {
  std::transform(
    value.begin(), value.end(), value.begin(),
    [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
  );
  return value;
}

struct Header {
  std::string name;
  std::string value;
};

auto responseHeaders(emscripten_fetch_t* fetch) -> std::vector<Header> {
  auto headers = std::vector<Header>{};
  const auto size = emscripten_fetch_get_response_headers_length(fetch);
  if (size == 0) {
    return headers;
  }

  auto raw = std::string(size + 1, '\0');
  emscripten_fetch_get_response_headers(fetch, raw.data(), raw.size());
  auto** unpacked = emscripten_fetch_unpack_response_headers(raw.c_str());
  if (unpacked == nullptr) {
    return headers;
  }

  for (auto** item = unpacked; item[0] != nullptr && item[1] != nullptr;
       item += 2) {
    headers.push_back({lowerASCII(item[0]), item[1]});
  }
  emscripten_fetch_free_unpacked_response_headers(unpacked);
  return headers;
}

auto headerValue(const std::vector<Header>& headers, const char* name)
  -> std::optional<std::string> {
  const auto expected = lowerASCII(name);
  auto value = std::optional<std::string>{};
  for (const auto& header : headers) {
    if (header.name != expected) {
      continue;
    }
    if (!value) {
      value = header.value;
    } else {
      *value += ", ";
      *value += header.value;
    }
  }
  return value;
}

void applyCacheHeaders(Response& response, const std::vector<Header>& headers) {
  response.etag = headerValue(headers, "etag");
  if (const auto modified = headerValue(headers, "last-modified")) {
    response.modified = util::parseTimestamp(modified->c_str());
  }
  if (const auto expires = headerValue(headers, "expires")) {
    response.expires = util::parseTimestamp(expires->c_str());
  }
  if (const auto cacheControl = headerValue(headers, "cache-control")) {
    const auto parsed = http::CacheControl::parse(*cacheControl);
    response.expires.reset();
    if (parsed.maxAge) {
      response.expires = parsed.toTimePoint();
    }
    response.mustRevalidate = parsed.mustRevalidate;
  }
}

auto makeResponse(const Resource& resource, emscripten_fetch_t* fetch)
  -> Response {
  auto result = Response{};
  const auto headers = responseHeaders(fetch);
  applyCacheHeaders(result, headers);

  const auto code = fetch->status;
  if (code == 0) {
    const auto statusText = std::string{fetch->statusText};
    result.error = std::make_unique<Response::Error>(
      Response::Error::Reason::Connection,
      statusText.empty() ? "Browser HTTP request failed" : statusText
    );
    return result;
  }

  if (code == 200 || code == 206) {
    if (fetch->numBytes == 0) {
      result.data = std::make_shared<std::string>();
    } else if (fetch->data == nullptr) {
      result.error = std::make_unique<Response::Error>(
        Response::Error::Reason::Other, "HTTP response missing body data"
      );
    } else {
      result.data = std::make_shared<std::string>(
        reinterpret_cast<const char*>(fetch->data),
        static_cast<std::size_t>(fetch->numBytes)
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
        headerValue(headers, "retry-after"),
        headerValue(headers, "x-rate-limit-reset")
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

class FetchRequestState;

class FetchRequestQueue {
 public:
  void enqueue(const std::shared_ptr<FetchRequestState>& state);
  void release(uint64_t id, const FetchRequestState* state);

 private:
  void pump();

  std::mutex mutex_;
  std::deque<std::weak_ptr<FetchRequestState>> pending_;
  std::vector<std::shared_ptr<FetchRequestState>> active_;
  bool pumping_ = false;
  bool pumpRequested_ = false;
};

class FetchRequestState
    : public std::enable_shared_from_this<FetchRequestState> {
 public:
  FetchRequestState(
    Resource resource_, FileSource::Callback callback_,
    std::shared_ptr<FetchRequestQueue> queue_
  )
      : id_(nextRequestId.fetch_add(1, std::memory_order_relaxed)),
        resource_(std::move(resource_)),
        callback_(std::move(callback_)),
        queue_(std::move(queue_)) {}

  auto id() const -> uint64_t { return id_; }
  auto resourceURL() const -> const std::string& { return resource_.url; }
  auto isCanceled() const -> bool {
    std::scoped_lock lock(mutex_);
    return canceled_ || finished_;
  }

  void activate() {
    std::scoped_lock lock(mutex_);
    active_ = true;
  }

  void start();
  void cancel();
  void initializeAsync();

 private:
  static auto takeHolder(emscripten_fetch_t* fetch)
    -> std::unique_ptr<std::shared_ptr<FetchRequestState>>;
  static void onFetchComplete(emscripten_fetch_t* fetch);

  void complete(emscripten_fetch_t* completedFetch);
  void finish(std::optional<Response> response);
  void deliver();
  void releaseActiveSlot();
  void buildRequestHeaders();

  uint64_t id_;
  Resource resource_;
  FileSource::Callback callback_;
  Response response_;
  std::weak_ptr<FetchRequestQueue> queue_;
  std::unique_ptr<util::AsyncTask> async_;
  std::shared_ptr<FetchRequestState> pendingDelivery_;
  emscripten_fetch_t* fetch_ = nullptr;
  std::vector<std::pair<std::string, std::string>> requestHeaderStorage_;
  std::vector<const char*> requestHeaders_;
  mutable std::mutex mutex_;
  bool canceled_ = false;
  bool active_ = false;
  bool finished_ = false;
};

void FetchRequestState::initializeAsync() {
  async_ = std::make_unique<util::AsyncTask>([weak = weak_from_this()]() {
    if (const auto state = weak.lock()) {
      state->deliver();
    }
  });
}

void FetchRequestQueue::enqueue(
  const std::shared_ptr<FetchRequestState>& state
) {
  {
    std::scoped_lock lock(mutex_);
    pending_.push_back(state);
    traceHttp(
      "enqueue id=%llu active=%zu pending=%zu url=%s",
      static_cast<unsigned long long>(state->id()), active_.size(),
      pending_.size(), state->resourceURL().c_str()
    );
  }
  pump();
}

void FetchRequestQueue::release(uint64_t id, const FetchRequestState* state) {
  {
    std::scoped_lock lock(mutex_);
    const auto found = std::find_if(
      active_.begin(), active_.end(),
      [id, state](const std::shared_ptr<FetchRequestState>& active) {
        return active && active->id() == id && active.get() == state;
      }
    );
    if (found == active_.end()) {
      traceHttp("release missing id=%llu", static_cast<unsigned long long>(id));
    } else {
      active_.erase(found);
    }
    traceHttp(
      "release id=%llu active=%zu pending=%zu",
      static_cast<unsigned long long>(id), active_.size(), pending_.size()
    );
  }
  pump();
}

void FetchRequestQueue::pump() {
  {
    std::scoped_lock lock(mutex_);
    if (pumping_) {
      pumpRequested_ = true;
      return;
    }
    pumping_ = true;
  }

  while (true) {
    auto ready = std::vector<std::shared_ptr<FetchRequestState>>{};
    {
      std::scoped_lock lock(mutex_);
      pumpRequested_ = false;
      while (active_.size() < maxActiveFetches && !pending_.empty()) {
        auto state = pending_.front().lock();
        pending_.pop_front();
        if (!state || state->isCanceled()) {
          continue;
        }

        state->activate();
        active_.push_back(state);
        traceHttp(
          "activate id=%llu active=%zu pending=%zu",
          static_cast<unsigned long long>(state->id()), active_.size(),
          pending_.size()
        );
        ready.push_back(std::move(state));
      }
    }

    for (const auto& state : ready) {
      state->start();
    }

    {
      std::scoped_lock lock(mutex_);
      if (!pumpRequested_) {
        pumping_ = false;
        return;
      }
    }
  }
}

void FetchRequestState::buildRequestHeaders() {
  requestHeaderStorage_.clear();
  requestHeaders_.clear();

  if (resource_.dataRange) {
    requestHeaderStorage_.emplace_back(
      "Range", std::string{"bytes="} +
                 util::toString(resource_.dataRange->first) + "-" +
                 util::toString(resource_.dataRange->second)
    );
  }

  if (resource_.priorEtag) {
    requestHeaderStorage_.emplace_back("If-None-Match", *resource_.priorEtag);
  } else if (resource_.priorModified) {
    requestHeaderStorage_.emplace_back(
      "If-Modified-Since", util::rfc1123(*resource_.priorModified)
    );
  }

  requestHeaders_.reserve(requestHeaderStorage_.size() * 2 + 1);
  for (const auto& [name, value] : requestHeaderStorage_) {
    requestHeaders_.push_back(name.c_str());
    requestHeaders_.push_back(value.c_str());
  }
  requestHeaders_.push_back(nullptr);
}

void FetchRequestState::start() {
  auto* holder = new std::shared_ptr<FetchRequestState>(shared_from_this());

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
  attributes.userData = holder;

  auto canceledBeforeStart = false;
  {
    std::scoped_lock lock(mutex_);
    if (canceled_ || finished_) {
      canceledBeforeStart = true;
    } else {
      buildRequestHeaders();
      if (requestHeaders_.size() > 1) {
        attributes.requestHeaders = requestHeaders_.data();
      }
    }
  }
  if (canceledBeforeStart) {
    delete holder;
    finish(std::nullopt);
    return;
  }

  const auto url = resource_.url;
  traceHttp(
    "start id=%llu url=%s", static_cast<unsigned long long>(id_), url.c_str()
  );
  auto* startedFetch = emscripten_fetch(&attributes, url.c_str());
  if (startedFetch == nullptr) {
    delete holder;
    auto response = Response{};
    response.error = std::make_unique<Response::Error>(
      Response::Error::Reason::Connection,
      "emscripten_fetch failed to start request"
    );
    finish(std::move(response));
    return;
  }

  auto completedSynchronously = false;
  auto closeImmediately = false;
  {
    std::scoped_lock lock(mutex_);
    if (finished_) {
      completedSynchronously = true;
    } else if (canceled_) {
      closeImmediately = true;
    } else {
      fetch_ = startedFetch;
    }
  }

  if (completedSynchronously) {
    return;
  }
  if (closeImmediately) {
    auto canceledHolder = takeHolder(startedFetch);
    emscripten_fetch_close(startedFetch);
  }
}

void FetchRequestState::cancel() {
  auto* fetchToClose = static_cast<emscripten_fetch_t*>(nullptr);
  std::unique_ptr<std::shared_ptr<FetchRequestState>> holder;
  auto wasActive = false;
  {
    std::scoped_lock lock(mutex_);
    if (canceled_) {
      return;
    }
    canceled_ = true;
    callback_ = nullptr;
    wasActive = active_ && !finished_;
    fetchToClose = fetch_;
    fetch_ = nullptr;
    holder = takeHolder(fetchToClose);
  }

  traceHttp(
    "cancel id=%llu had_fetch=%d active=%d",
    static_cast<unsigned long long>(id_), fetchToClose != nullptr, wasActive
  );
  if (fetchToClose != nullptr) {
    emscripten_fetch_close(fetchToClose);
  }
  if (wasActive) {
    finish(std::nullopt);
  }
}

auto FetchRequestState::takeHolder(emscripten_fetch_t* fetch)
  -> std::unique_ptr<std::shared_ptr<FetchRequestState>> {
  if (fetch == nullptr || fetch->userData == nullptr) {
    return {};
  }
  auto* holderPtr =
    static_cast<std::shared_ptr<FetchRequestState>*>(fetch->userData);
  fetch->userData = nullptr;
  return std::unique_ptr<std::shared_ptr<FetchRequestState>>{holderPtr};
}

void FetchRequestState::onFetchComplete(emscripten_fetch_t* fetch) {
  auto holder = takeHolder(fetch);
  if (!holder) {
    return;
  }
  (*holder)->complete(fetch);
}

void FetchRequestState::complete(emscripten_fetch_t* completedFetch) {
  const auto response = makeResponse(resource_, completedFetch);
  traceHttp(
    "complete id=%llu status=%u bytes=%llu url=%s",
    static_cast<unsigned long long>(id_), completedFetch->status,
    static_cast<unsigned long long>(completedFetch->numBytes),
    resource_.url.c_str()
  );
  {
    std::scoped_lock lock(mutex_);
    if (fetch_ == completedFetch) {
      fetch_ = nullptr;
    }
  }
  emscripten_fetch_close(completedFetch);
  finish(response);
}

void FetchRequestState::finish(std::optional<Response> response) {
  auto shouldRelease = false;
  auto shouldDeliver = false;
  auto* async = static_cast<util::AsyncTask*>(nullptr);
  {
    std::scoped_lock lock(mutex_);
    if (finished_) {
      return;
    }
    finished_ = true;
    shouldRelease = active_;
    active_ = false;
    requestHeaderStorage_.clear();
    requestHeaders_.clear();
    if (response && !canceled_ && callback_) {
      response_ = std::move(*response);
      pendingDelivery_ = shared_from_this();
      shouldDeliver = true;
      async = async_.get();
    }
  }

  if (shouldRelease) {
    releaseActiveSlot();
  }
  if (shouldDeliver && async != nullptr) {
    async->send();
  }
}

void FetchRequestState::releaseActiveSlot() {
  if (const auto queue = queue_.lock()) {
    queue->release(id_, this);
  }
}

void FetchRequestState::deliver() {
  auto keepAlive = std::shared_ptr<FetchRequestState>{};
  auto callback = FileSource::Callback{};
  auto response = Response{};
  {
    std::scoped_lock lock(mutex_);
    keepAlive = std::move(pendingDelivery_);
    if (canceled_ || !callback_) {
      return;
    }
    callback = std::move(callback_);
    response = response_;
  }

  if (callback) {
    callback(response);
  }
}

class FetchRequest : public AsyncRequest {
 public:
  FetchRequest(
    Resource resource, FileSource::Callback callback,
    std::shared_ptr<FetchRequestQueue> queue
  )
      : queue_(std::move(queue)),
        state_(
          std::make_shared<FetchRequestState>(
            std::move(resource), std::move(callback), queue_
          )
        ) {
    state_->initializeAsync();
    queue_->enqueue(state_);
  }

  ~FetchRequest() override { state_->cancel(); }

 private:
  std::shared_ptr<FetchRequestQueue> queue_;
  std::shared_ptr<FetchRequestState> state_;
};

}  // namespace

class HTTPFileSource::Impl {
 public:
  Impl(
    const ResourceOptions& resourceOptions, const ClientOptions& clientOptions
  )
      : resourceOptions_(resourceOptions.clone()),
        clientOptions_(clientOptions.clone()) {}

  void setResourceOptions(ResourceOptions options) {
    std::scoped_lock lock(mutex_);
    resourceOptions_ = options.clone();
  }

  ResourceOptions getResourceOptions() {
    std::scoped_lock lock(mutex_);
    return resourceOptions_.clone();
  }

  void setClientOptions(ClientOptions options) {
    std::scoped_lock lock(mutex_);
    clientOptions_ = options.clone();
  }

  ClientOptions getClientOptions() {
    std::scoped_lock lock(mutex_);
    return clientOptions_.clone();
  }

 private:
  friend HTTPFileSource;

  mutable std::mutex mutex_;
  ResourceOptions resourceOptions_;
  ClientOptions clientOptions_;
  std::shared_ptr<FetchRequestQueue> queue_ =
    std::make_shared<FetchRequestQueue>();
};

HTTPFileSource::HTTPFileSource(
  const ResourceOptions& resourceOptions, const ClientOptions& clientOptions
)
    : impl(std::make_unique<Impl>(resourceOptions, clientOptions)) {}

HTTPFileSource::~HTTPFileSource() = default;

std::unique_ptr<AsyncRequest> HTTPFileSource::request(
  const Resource& resource, Callback callback
) {
  return std::make_unique<FetchRequest>(
    resource, std::move(callback), impl->queue_
  );
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

extern "C" {

EMSCRIPTEN_KEEPALIVE void mln_emscripten_http_trace_set(int enabled) {
  mbgl::traceHttpEnabled.store(enabled != 0, std::memory_order_relaxed);
}

}  // extern "C"
