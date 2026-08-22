// Browser HTTP via emscripten_fetch, so requests respect CORS, cookies, and the
// page cache.

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <iterator>
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
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/client_options.hpp>
#include <mbgl/util/http_header.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/util.hpp>

#include <emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <pthread.h>

#include "../offline_url.hpp"

namespace mln {
namespace {

// FETCH_LOAD_TO_MEMORY copies each completed XHR body into the fixed pthread
// wasm heap before C++ can drop obsolete responses, so bound active transport
// requests below OnlineFileSource's platform-wide concurrency limit.
constexpr auto maxActiveFetches = std::size_t{16};

// Deadline for one fetch, after which the request hands its slot back with a
// connection error that MapLibre retries. Without it a stalled server holds its
// slot forever. An XHR exposes only a whole-request deadline, so this also
// bounds a transfer that is still making progress.
constexpr auto fetchTimeout = Seconds{30};

std::atomic_uint64_t nextRequestId = 1;

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

// Request headers in the order emscripten_fetch wants them, before they are
// flattened into its null-terminated name/value array.
using RequestHeaders = std::vector<std::pair<std::string, std::string>>;

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

// The thread every fetch is issued on.
//
// emscripten_fetch reports through the calling thread's event loop, and every
// MapLibre thread parks in its run loop instead of returning to one, so a fetch
// issued from one is never reported. This thread's entry takes a runtime
// keepalive and returns, leaving a worker that services JavaScript; work
// reaches it through the proxying queue. The first request queue starts it and
// the last releases the keepalive. Responses travel back through
// util::AsyncTask, which needs no event loop on the receiving side.
class TransportThread {
 public:
  static auto instance() -> TransportThread& {
    static TransportThread thread;
    return thread;
  }

  void acquire() {
    std::scoped_lock lock(mutex_);
    users_ += 1;
    if (users_ != 1) {
      return;
    }
    started_ = pthread_create(&thread_, nullptr, &live, nullptr) == 0;
    if (started_) {
      pthread_detach(thread_);
    }
  }

  void release() {
    auto thread = pthread_t{};
    {
      std::scoped_lock lock(mutex_);
      if (users_ == 0) {
        return;
      }
      users_ -= 1;
      if (users_ != 0 || !started_) {
        return;
      }
      thread = thread_;
      started_ = false;
    }

    // Queue destruction means no request can enqueue more transport work. A
    // stop proxy therefore runs after everything already queued and drops the
    // keepalive once that work is complete.
    static_cast<void>(emscripten_proxy_async(
      emscripten_proxy_get_system_queue(), thread, &stop, nullptr
    ));
  }

  // Reports false when the thread is unavailable, which leaves the caller to
  // fail the request rather than wait for a response nothing will produce.
  [[nodiscard]] auto run(std::function<void()> work) -> bool {
    std::scoped_lock lock(mutex_);
    if (!started_) {
      return false;
    }
    return runOn(thread_, std::move(work));
  }

  // Queues work on a specific transport generation. A fetch belongs to the
  // pthread that created its browser XHR, which can differ from `thread_` once
  // the last source releases one generation and another source starts the
  // next.
  [[nodiscard]] static auto runOn(pthread_t thread, std::function<void()> work)
    -> bool {
    auto task = std::make_unique<std::function<void()>>(std::move(work));
    if (
      emscripten_proxy_async(
        emscripten_proxy_get_system_queue(), thread, &execute, task.get()
      ) == 0
    ) {
      return false;
    }
    // The queue owns it from here; execute() takes it back.
    static_cast<void>(task.release());
    return true;
  }

 private:
  TransportThread() = default;

  static auto live(void* /*unused*/) -> void* {
    emscripten_runtime_keepalive_push();
    return nullptr;
  }

  static void stop(void* /*unused*/) { emscripten_runtime_keepalive_pop(); }

  static void execute(void* raw) {
    const auto task = std::unique_ptr<std::function<void()>>{
      static_cast<std::function<void()>*>(raw)
    };
    (*task)();
  }

  std::mutex mutex_;
  pthread_t thread_{};
  std::size_t users_ = 0;
  bool started_ = false;
};

class FetchRequestState;

class FetchRequestQueue {
 public:
  FetchRequestQueue() { TransportThread::instance().acquire(); }
  ~FetchRequestQueue() { TransportThread::instance().release(); }

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

// A request outlives the call that made it, and cancellation, fetch completion,
// and delivery race freely. The rule: everything a request holds — the queue's
// transport slot, the fetch handle, the caller's callback — is claimed by
// taking it out of its member under `mutex_`, and whoever claims it is the only
// one that releases it.
//
// A request never holds a reference to itself. The caller's AsyncRequest owns
// it until cancellation, the queue owns every request it activated, an
// in-flight fetch owns the one it reports to, and the delivery task refers to
// it weakly.
class FetchRequestState
    : public std::enable_shared_from_this<FetchRequestState> {
 public:
  FetchRequestState(
    Resource resource, FileSource::Callback callback,
    std::shared_ptr<FetchRequestQueue> queue
  )
      : id_(nextRequestId.fetch_add(1, std::memory_order_relaxed)),
        resource_(std::move(resource)),
        url_(mln::platform::offline_url(resource_)),
        queue_(std::move(queue)),
        callback_(std::move(callback)) {}

  auto id() const -> uint64_t { return id_; }

  // Whether the request still has somewhere to put a response. The queue skips
  // one that does not rather than spending a slot on it.
  auto wantsTransport() const -> bool {
    std::scoped_lock lock(mutex_);
    return !canceled_ && !finished_;
  }

  // Records the transport slot the queue just handed out. The request gives it
  // back exactly once, however it ends.
  void takeSlot() {
    std::scoped_lock lock(mutex_);
    slotHeld_ = true;
  }

  void initializeDelivery();
  void startOnTransport();
  void cancel();

 private:
  // Who owns the fetch handle, and therefore who closes it.
  enum class Transport : std::uint8_t {
    // No handle outstanding.
    None,
    // A fetch is in flight and its terminal callback closes the handle. Set
    // before emscripten_fetch() so the handle has an owner even before the
    // request stores it.
    Callback,
    // The request took the handle away from the callback and closes it itself.
    Request,
  };

  static auto takeHolder(emscripten_fetch_t* fetch)
    -> std::unique_ptr<std::shared_ptr<FetchRequestState>>;
  static void onFetchComplete(emscripten_fetch_t* fetch);

  void start();

  void closeClaimedFetch(
    emscripten_fetch_t* claimedFetch, pthread_t transportThread
  );
  void completeFetch(emscripten_fetch_t* completedFetch);
  void finish(std::optional<Response> response);
  void deliver();
  void releaseSlot();
  auto buildRequestHeaders() const -> RequestHeaders;

  const uint64_t id_;
  const Resource resource_;
  // The URL the transport requests, which is the resource's own unless it is an
  // offline download. Resolved once so every path sees the same URL.
  const std::string url_;
  const std::weak_ptr<FetchRequestQueue> queue_;

  mutable std::mutex mutex_;
  std::shared_ptr<util::AsyncTask> delivery_;
  FileSource::Callback callback_;
  Response response_;
  emscripten_fetch_t* fetch_ = nullptr;
  pthread_t transportThread_{};
  Transport transport_ = Transport::None;
  bool canceled_ = false;
  bool slotHeld_ = false;
  bool finished_ = false;
};

void FetchRequestState::initializeDelivery() {
  // Runs on the thread that owns the request, before the queue can see it: the
  // task delivers on that thread's run loop, and it holds a weak reference so a
  // queued delivery never keeps the request alive past cancellation.
  delivery_ = std::make_shared<util::AsyncTask>([weak = weak_from_this()]() {
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
    if (found != active_.end()) {
      active_.erase(found);
    }
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
        if (!state || !state->wantsTransport()) {
          continue;
        }

        state->takeSlot();
        active_.push_back(state);
        ready.push_back(std::move(state));
      }
    }

    for (const auto& state : ready) {
      state->startOnTransport();
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

auto FetchRequestState::buildRequestHeaders() const -> RequestHeaders {
  // Every input is fixed at construction, so this runs without `mutex_`.
  auto headers = RequestHeaders{};
  if (resource_.dataRange) {
    headers.emplace_back(
      "Range", std::string{"bytes="} +
                 util::toString(resource_.dataRange->first) + "-" +
                 util::toString(resource_.dataRange->second)
    );
  }

  if (resource_.priorEtag) {
    headers.emplace_back("If-None-Match", *resource_.priorEtag);
  } else if (resource_.priorModified) {
    headers.emplace_back(
      "If-Modified-Since", util::rfc1123(*resource_.priorModified)
    );
  }

  // Unlike the native sources, this one sends no User-Agent: the browser sets
  // it and an XHR rejects the header.
  return headers;
}

// Hands the request to the transport thread, the only one a fetch can report
// to. Everything from here runs there, including the bookkeeping after
// emscripten_fetch() returns.
void FetchRequestState::startOnTransport() {
  auto self = shared_from_this();
  if (TransportThread::instance().run([self]() { self->start(); })) {
    return;
  }

  auto response = Response{};
  response.error = std::make_unique<Response::Error>(
    Response::Error::Reason::Connection,
    "the browser transport thread is unavailable"
  );
  finish(std::move(response));
}

void FetchRequestState::start() {
  const auto transportThread = pthread_self();
  auto wantsFetch = false;
  {
    std::scoped_lock lock(mutex_);
    wantsFetch = !canceled_ && !finished_;
    if (wantsFetch) {
      transport_ = Transport::Callback;
      transportThread_ = transportThread;
    }
  }
  if (!wantsFetch) {
    // Cancellation beat the queue to this request, so the slot it was just
    // handed comes straight back.
    releaseSlot();
    return;
  }

  emscripten_fetch_attr_t attributes{};
  emscripten_fetch_attr_init(&attributes);
  std::strcpy(attributes.requestMethod, "GET");
  attributes.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attributes.timeoutMSecs =
    static_cast<std::uint32_t>(Milliseconds{fetchTimeout}.count());
  attributes.onsuccess = onFetchComplete;
  attributes.onerror = onFetchComplete;

  // Both outlive the emscripten_fetch() call below, which copies out the
  // strings the pointers refer to.
  const auto headers = buildRequestHeaders();
  auto headerPointers = std::vector<const char*>{};
  headerPointers.reserve(headers.size() * 2 + 1);
  for (const auto& [name, value] : headers) {
    headerPointers.push_back(name.c_str());
    headerPointers.push_back(value.c_str());
  }
  headerPointers.push_back(nullptr);
  if (!headers.empty()) {
    attributes.requestHeaders = headerPointers.data();
  }

  // The fetch owns a reference to the request until the terminal callback
  // releases it. Every fetch reports exactly once, including when a close
  // aborts it, so `holder` must not be touched once the fetch has it.
  auto* holder = new std::shared_ptr<FetchRequestState>(shared_from_this());
  attributes.userData = holder;

  auto* startedFetch = emscripten_fetch(&attributes, url_.c_str());
  if (startedFetch == nullptr) {
    // Nothing started, so nothing will report: the reference and the handle
    // ownership both come back here.
    delete holder;
    {
      std::scoped_lock lock(mutex_);
      transport_ = Transport::None;
      transportThread_ = pthread_t{};
    }
    auto response = Response{};
    response.error = std::make_unique<Response::Error>(
      Response::Error::Reason::Connection,
      "emscripten_fetch failed to start request"
    );
    finish(std::move(response));
    return;
  }

  auto* fetchToClose = static_cast<emscripten_fetch_t*>(nullptr);
  {
    std::scoped_lock lock(mutex_);
    if (transport_ != Transport::Callback) {
      // The fetch already reported from inside emscripten_fetch() and its
      // callback closed the handle, which freed `startedFetch`.
    } else if (canceled_) {
      // Cancellation could not claim a handle the request had not stored yet,
      // so ending the fetch is this call's job.
      transport_ = Transport::Request;
      fetchToClose = startedFetch;
    } else {
      fetch_ = startedFetch;
    }
  }

  if (fetchToClose != nullptr) {
    closeClaimedFetch(fetchToClose, transportThread);
    finish(std::nullopt);
  }
}

void FetchRequestState::closeClaimedFetch(
  emscripten_fetch_t* claimedFetch, pthread_t transportThread
) {
  // Aborts the transfer on the thread that owns the handle. The XHR is local to
  // the transport generation that created it, so `transportThread` stays its
  // target even after another generation has started. Emscripten reports the
  // abort to onFetchComplete() from inside this call, which finds the handle
  // claimed and leaves it here.
  if (pthread_equal(pthread_self(), transportThread)) {
    emscripten_fetch_close(claimedFetch);
  } else {
    // If proxying fails, leaking the handle beats touching worker-local XHR
    // state from another pthread.
    static_cast<void>(TransportThread::runOn(transportThread, [claimedFetch]() {
      emscripten_fetch_close(claimedFetch);
    }));
  }
  {
    std::scoped_lock lock(mutex_);
    transport_ = Transport::None;
    transportThread_ = pthread_t{};
  }
}

void FetchRequestState::cancel() {
  auto delivery = std::shared_ptr<util::AsyncTask>{};
  auto* fetchToClose = static_cast<emscripten_fetch_t*>(nullptr);
  auto transportThread = pthread_t{};
  auto closePending = false;
  {
    std::scoped_lock lock(mutex_);
    if (canceled_) {
      return;
    }
    canceled_ = true;
    // Dropping the task also removes any delivery it already queued from the
    // run loop, so a response cannot remain in the wasm heap after
    // cancellation.
    callback_ = nullptr;
    delivery = std::move(delivery_);
    if (transport_ == Transport::Callback) {
      if (fetch_ != nullptr) {
        transport_ = Transport::Request;
        fetchToClose = std::exchange(fetch_, nullptr);
        transportThread = transportThread_;
      } else {
        // start() has not stored the handle yet; it closes and finishes.
        closePending = true;
      }
    }
  }

  if (fetchToClose != nullptr) {
    closeClaimedFetch(fetchToClose, transportThread);
  }
  if (!closePending) {
    finish(std::nullopt);
  }
  // `delivery` is destroyed here, with `mutex_` released.
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
  // Releases the fetch's reference to the request once this returns.
  const auto holder = takeHolder(fetch);
  if (!holder) {
    return;
  }
  (*holder)->completeFetch(fetch);
}

void FetchRequestState::completeFetch(emscripten_fetch_t* completedFetch) {
  {
    std::scoped_lock lock(mutex_);
    if (transport_ != Transport::Callback) {
      // Cancellation claimed the handle and is inside emscripten_fetch_close(),
      // which reported this abort. It closes the handle and finishes the
      // request, and `completedFetch` holds nothing left to read.
      return;
    }
    // Claimed before reading the response, so a cancellation that arrives while
    // this runs cannot free the handle underneath it.
    transport_ = Transport::None;
    fetch_ = nullptr;
    transportThread_ = pthread_t{};
  }

  auto response = makeResponse(resource_, completedFetch);
  emscripten_fetch_close(completedFetch);
  finish(std::move(response));
}

void FetchRequestState::finish(std::optional<Response> response) {
  auto delivery = std::shared_ptr<util::AsyncTask>{};
  {
    std::scoped_lock lock(mutex_);
    if (finished_) {
      return;
    }
    finished_ = true;
    if (response && !canceled_ && callback_) {
      response_ = std::move(*response);
      delivery = delivery_;
    }
  }

  releaseSlot();
  if (delivery) {
    delivery->send();
  }
}

void FetchRequestState::releaseSlot() {
  // The queue holds a reference to every request it activated, so handing the
  // slot back can be what destroys this one.
  const auto self = shared_from_this();
  auto held = false;
  {
    std::scoped_lock lock(mutex_);
    held = std::exchange(slotHeld_, false);
  }
  if (!held) {
    return;
  }
  if (const auto queue = queue_.lock()) {
    queue->release(id_, this);
  }
}

void FetchRequestState::deliver() {
  auto callback = FileSource::Callback{};
  auto response = Response{};
  {
    std::scoped_lock lock(mutex_);
    if (canceled_ || !callback_) {
      return;
    }
    // Claimed, so a later wake cannot deliver the same response twice.
    callback = std::move(callback_);
    response = std::move(response_);
  }

  callback(response);
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
    state_->initializeDelivery();
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

}  // namespace mln
