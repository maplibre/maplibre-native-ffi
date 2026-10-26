// Android HTTP through java.net.HttpURLConnection, which needs no Java code of
// the library's own. It blocks, so a pool of threads attached to the Java VM
// runs the requests.

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <mln/storage/file_source.hpp>
#include <mln/storage/http_file_source.hpp>
#include <mln/storage/resource.hpp>
#include <mln/storage/resource_options.hpp>
#include <mln/storage/response.hpp>
#include <mln/util/async_request.hpp>
#include <mln/util/client_options.hpp>
#include <mln/util/platform.hpp>

#include "../http_transport.hpp"
#include "../offline_url.hpp"
#include "platform/android/i18n.hpp"
#include "platform/android/java_vm.hpp"

namespace {

using mln::Response;
using Reason = mln::Response::Error::Reason;
namespace http = mln::platform::http;

// Matches `mln::util::DEFAULT_MAXIMUM_CONCURRENT_REQUESTS` enforced by
// `OnlineFileSource`.
constexpr auto worker_count = std::size_t{20};
// Bounds connecting and each wait for more of the reply. HttpURLConnection runs
// the TLS handshake under the read timeout, so a stalled peer cannot occupy a
// worker, while a slow but progressing download still completes.
constexpr auto timeout_ms = jni::jint{30'000};
// libcurl 8's default redirect limit.
constexpr auto max_redirects = 30;
constexpr auto read_chunk_size = jni::jsize{64 * 1024};

struct JavaURL {
  static constexpr auto Name() { return "java/net/URL"; }
};

struct URLConnection {
  static constexpr auto Name() { return "java/net/URLConnection"; }
};

struct HttpURLConnection {
  static constexpr auto Name() { return "java/net/HttpURLConnection"; }
};

struct InputStream {
  static constexpr auto Name() { return "java/io/InputStream"; }
};

struct Throwable {
  static constexpr auto Name() { return "java/lang/Throwable"; }
};

struct IOException {
  static constexpr auto Name() { return "java/io/IOException"; }
};

struct SSLException {
  static constexpr auto Name() { return "javax/net/ssl/SSLException"; }
};

struct ProtocolException {
  static constexpr auto Name() { return "java/net/ProtocolException"; }
};

struct MalformedURLException {
  static constexpr auto Name() { return "java/net/MalformedURLException"; }
};

struct UnknownServiceException {
  static constexpr auto Name() { return "java/net/UnknownServiceException"; }
};

using Connection = jni::Object<HttpURLConnection>;

auto equals_ignoring_ascii_case(std::string_view left, std::string_view right)
  -> bool {
  if (left.size() != right.size()) return false;
  for (std::size_t index = 0; index < left.size(); ++index) {
    auto lower = [](char value) {
      return value >= 'A' && value <= 'Z' ? static_cast<char>(value + 32)
                                          : value;
    };
    if (lower(left[index]) != lower(right[index])) return false;
  }
  return true;
}

auto has_header(
  const std::vector<http::RequestHeader>& headers, std::string_view name
) -> bool {
  for (const auto& header : headers) {
    if (equals_ignoring_ascii_case(header.name, name)) return true;
  }
  return false;
}

// Whether `throwable` is one that a retry cannot fix: a TLS failure, a
// malformed reply or URL, or a request that the application's network security
// policy refuses. Any other I/O failure is the connection's.
auto failure_reason(JNIEnv& env, const jni::Object<Throwable>& throwable)
  -> Reason {
  if (
    throwable.IsInstanceOf(env, jni::Class<SSLException>::Singleton(env)) ||
    throwable.IsInstanceOf(
      env, jni::Class<ProtocolException>::Singleton(env)
    ) ||
    throwable.IsInstanceOf(
      env, jni::Class<MalformedURLException>::Singleton(env)
    ) ||
    throwable.IsInstanceOf(
      env, jni::Class<UnknownServiceException>::Singleton(env)
    )
  ) {
    return Reason::Other;
  }
  return throwable.IsInstanceOf(env, jni::Class<IOException>::Singleton(env))
           ? Reason::Connection
           : Reason::Other;
}

// Takes the pending Java exception as the request's failure.
auto failure_response(JNIEnv& env) -> Response {
  auto throwable =
    jni::Local<jni::Object<Throwable>>(env, jni::ExceptionOccurred(env));
  jni::ExceptionClear(env);
  try {
    const auto& cls = jni::Class<Throwable>::Singleton(env);
    static const auto to_string = cls.GetMethod<jni::String()>(env, "toString");
    auto message = jni::Make<std::string>(env, throwable.Call(env, to_string));
    return http::error_response(
      failure_reason(env, throwable), std::move(message)
    );
  } catch (const jni::PendingJavaException&) {
    jni::ExceptionClear(env);
    return http::error_response(Reason::Other, "Android HTTP request failed");
  }
}

auto make_url(JNIEnv& env, const std::string& spec)
  -> jni::Local<jni::Object<JavaURL>> {
  const auto& cls = jni::Class<JavaURL>::Singleton(env);
  static const auto constructor = cls.GetConstructor<jni::String>(env);
  return cls.New(env, constructor, jni::Make<jni::String>(env, spec));
}

// Resolves a redirect's Location against the URL that answered with it, or
// returns null when the result is not an HTTP URL.
auto resolve_redirect(
  JNIEnv& env, const jni::Object<JavaURL>& base, const std::string& location
) -> jni::Local<jni::Object<JavaURL>> {
  const auto& cls = jni::Class<JavaURL>::Singleton(env);
  static const auto constructor =
    cls.GetConstructor<jni::Object<JavaURL>, jni::String>(env);
  static const auto get_protocol =
    cls.GetMethod<jni::String()>(env, "getProtocol");
  try {
    auto resolved =
      cls.New(env, constructor, base, jni::Make<jni::String>(env, location));
    const auto protocol =
      jni::Make<std::string>(env, resolved.Call(env, get_protocol));
    if (protocol == "http" || protocol == "https") {
      return resolved;
    }
  } catch (const jni::PendingJavaException&) {
    jni::ExceptionClear(env);
  }
  return jni::Local<jni::Object<JavaURL>>{};
}

auto same_origin(
  JNIEnv& env, const jni::Object<JavaURL>& left,
  const jni::Object<JavaURL>& right
) -> bool {
  const auto& cls = jni::Class<JavaURL>::Singleton(env);
  static const auto get_protocol =
    cls.GetMethod<jni::String()>(env, "getProtocol");
  static const auto get_host = cls.GetMethod<jni::String()>(env, "getHost");
  static const auto get_port = cls.GetMethod<jni::jint()>(env, "getPort");
  static const auto get_default_port =
    cls.GetMethod<jni::jint()>(env, "getDefaultPort");
  auto port = [&](const jni::Object<JavaURL>& url) {
    const auto explicit_port = url.Call(env, get_port);
    return explicit_port == -1 ? url.Call(env, get_default_port)
                               : explicit_port;
  };
  return jni::Make<std::string>(env, left.Call(env, get_protocol)) ==
           jni::Make<std::string>(env, right.Call(env, get_protocol)) &&
         equals_ignoring_ascii_case(
           jni::Make<std::string>(env, left.Call(env, get_host)),
           jni::Make<std::string>(env, right.Call(env, get_host))
         ) &&
         port(left) == port(right);
}

auto header_field(JNIEnv& env, const Connection& connection, const char* name)
  -> std::optional<std::string> {
  const auto& cls = jni::Class<HttpURLConnection>::Singleton(env);
  static const auto get_header_field =
    cls.GetMethod<jni::String(jni::String)>(env, "getHeaderField");
  const auto value = connection.Call(
    env, get_header_field, jni::Make<jni::String>(env, std::string{name})
  );
  if (!value) {
    return std::nullopt;
  }
  return jni::Make<std::string>(env, value);
}

auto read_body(JNIEnv& env, const Connection& connection)
  -> std::shared_ptr<const std::string> {
  const auto& connection_cls = jni::Class<HttpURLConnection>::Singleton(env);
  static const auto get_input_stream =
    connection_cls.GetMethod<jni::Object<InputStream>()>(env, "getInputStream");
  const auto& stream_cls = jni::Class<InputStream>::Singleton(env);
  static const auto read =
    stream_cls.GetMethod<jni::jint(jni::Array<jni::jbyte>)>(env, "read");
  static const auto close = stream_cls.GetMethod<void()>(env, "close");

  const auto stream = connection.Call(env, get_input_stream);
  const auto buffer = jni::Array<jni::jbyte>::New(env, read_chunk_size);
  auto body = std::make_shared<std::string>();
  for (;;) {
    const auto count = stream.Call(env, read, buffer);
    if (count < 0) {
      break;
    }
    const auto offset = body->size();
    body->resize(offset + static_cast<std::size_t>(count));
    jni::GetArrayRegion(
      env, *buffer, 0, count,
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      reinterpret_cast<jni::jbyte*>(body->data() + offset)
    );
  }
  stream.Call(env, close);
  return body;
}

// One request's connection, which cancel() disconnects from the requesting
// thread to end a transfer that is still in progress.
class Transfer {
 public:
  // Returns false once the request is canceled.
  auto track(JNIEnv& env, const Connection& connection) -> bool {
    std::scoped_lock lock(mutex);
    if (canceled) {
      return false;
    }
    current = jni::NewGlobal<jni::EnvAttachingDeleter>(env, connection);
    return true;
  }

  void untrack() {
    std::scoped_lock lock(mutex);
    current.reset();
  }

  [[nodiscard]] auto is_canceled() -> bool {
    std::scoped_lock lock(mutex);
    return canceled;
  }

  void cancel() {
    std::scoped_lock lock(mutex);
    canceled = true;
    auto* const jvm = mln::platform::android::java_vm();
    if (!current || jvm == nullptr) {
      return;
    }
    const auto environment = jni::GetAttachedEnv(*jvm);
    auto& env = *environment;
    try {
      const auto& cls = jni::Class<HttpURLConnection>::Singleton(env);
      static const auto disconnect = cls.GetMethod<void()>(env, "disconnect");
      current.Call(env, disconnect);
    } catch (const jni::PendingJavaException&) {
      jni::ExceptionClear(env);
    }
  }

 private:
  std::mutex mutex;
  bool canceled = false;
  jni::Global<Connection, jni::EnvAttachingDeleter> current;
};

auto open_connection(
  JNIEnv& env, const jni::Object<JavaURL>& url,
  const std::vector<http::RequestHeader>& headers
) -> jni::Local<Connection> {
  const auto& url_cls = jni::Class<JavaURL>::Singleton(env);
  static const auto open =
    url_cls.GetMethod<jni::Object<URLConnection>()>(env, "openConnection");
  const auto& cls = jni::Class<HttpURLConnection>::Singleton(env);
  static const auto set_follow_redirects =
    cls.GetMethod<void(jni::jboolean)>(env, "setInstanceFollowRedirects");
  static const auto set_use_caches =
    cls.GetMethod<void(jni::jboolean)>(env, "setUseCaches");
  static const auto set_connect_timeout =
    cls.GetMethod<void(jni::jint)>(env, "setConnectTimeout");
  static const auto set_read_timeout =
    cls.GetMethod<void(jni::jint)>(env, "setReadTimeout");
  static const auto add_request_property =
    cls.GetMethod<void(jni::String, jni::String)>(env, "addRequestProperty");

  auto connection = jni::Cast(env, cls, url.Call(env, open));
  // Redirects are followed here, so that each hop can drop the sensitive
  // headers on leaving the origin and can move between HTTP and HTTPS.
  connection.Call(env, set_follow_redirects, jni::jni_false);
  // The file source caches, and revalidates through its own headers.
  connection.Call(env, set_use_caches, jni::jni_false);
  connection.Call(env, set_connect_timeout, timeout_ms);
  connection.Call(env, set_read_timeout, timeout_ms);
  for (const auto& header : headers) {
    connection.Call(
      env, add_request_property, jni::Make<jni::String>(env, header.name),
      jni::Make<jni::String>(env, header.value)
    );
  }
  // HttpURLConnection otherwise asks for gzip and decodes it, which a byte
  // range of the encoded body cannot survive.
  if (has_header(headers, "range") && !has_header(headers, "accept-encoding")) {
    connection.Call(
      env, add_request_property,
      jni::Make<jni::String>(env, std::string{"Accept-Encoding"}),
      jni::Make<jni::String>(env, std::string{"identity"})
    );
  }
  return connection;
}

auto is_redirect_status(jni::jint status) -> bool {
  return status == 301 || status == 302 || status == 303 || status == 307 ||
         status == 308;
}

// Runs the request on a worker's thread, following redirects. Returns nothing
// once the request is canceled.
auto fetch(
  JNIEnv& env, Transfer& transfer, const mln::Resource& resource,
  const std::string& url_spec, std::vector<http::RequestHeader> headers
) -> std::optional<Response> {
  const auto& cls = jni::Class<HttpURLConnection>::Singleton(env);
  static const auto get_response_code =
    cls.GetMethod<jni::jint()>(env, "getResponseCode");
  static const auto disconnect = cls.GetMethod<void()>(env, "disconnect");
  const auto ranged = has_header(headers, "range");

  auto url = make_url(env, url_spec);
  for (auto hop = 0; hop <= max_redirects; ++hop) {
    const auto connection = open_connection(env, url, headers);
    if (!transfer.track(env, connection)) {
      return std::nullopt;
    }

    const auto status = connection.Call(env, get_response_code);
    // disconnect() does nothing before the connection opens, so a request
    // canceled while it opened ends here instead.
    if (transfer.is_canceled()) {
      connection.Call(env, disconnect);
      transfer.untrack();
      return std::nullopt;
    }
    if (is_redirect_status(status)) {
      const auto location = header_field(env, connection, "Location");
      connection.Call(env, disconnect);
      transfer.untrack();
      if (!location) {
        return http::error_response(
          Reason::Other, "HTTP redirect missing Location header"
        );
      }
      if (location->empty()) {
        return http::error_response(
          Reason::Other, "HTTP redirect Location header is empty"
        );
      }
      auto next = resolve_redirect(env, url, *location);
      if (!next) {
        return http::error_response(
          Reason::Other, "unsupported HTTP redirect URL"
        );
      }
      if (!same_origin(env, url, next)) {
        std::erase_if(headers, [](const auto& header) {
          return header.sensitive;
        });
      }
      url = std::move(next);
      continue;
    }

    if (ranged && (status == 200 || status == 206)) {
      const auto encoding = header_field(env, connection, "Content-Encoding");
      if (encoding && !equals_ignoring_ascii_case(*encoding, "identity")) {
        connection.Call(env, disconnect);
        transfer.untrack();
        return http::error_response(
          Reason::Other, "unsupported HTTP content encoding: " + *encoding
        );
      }
    }

    const auto reply_headers = http::ReplyHeaders{
      .etag = header_field(env, connection, "ETag"),
      .modified = header_field(env, connection, "Last-Modified"),
      .cache_control = header_field(env, connection, "Cache-Control"),
      .expires = header_field(env, connection, "Expires"),
      .retry_after = header_field(env, connection, "Retry-After"),
      .x_rate_limit_reset = header_field(env, connection, "x-rate-limit-reset"),
    };
    auto body = std::shared_ptr<const std::string>{};
    if (status == 200 || status == 206) {
      try {
        body = read_body(env, connection);
      } catch (const jni::PendingJavaException&) {
        const auto failure = failure_response(env);
        connection.Call(env, disconnect);
        transfer.untrack();
        // A body that stops partway is the connection's, whatever ended it.
        return http::error_response(Reason::Connection, failure.error->message);
      }
    } else {
      connection.Call(env, disconnect);
    }
    transfer.untrack();
    return http::make_response(
      resource, static_cast<std::uint16_t>(status), reply_headers,
      std::move(body)
    );
  }
  return http::error_response(Reason::Other, "too many HTTP redirects");
}

// The threads that run requests, each attached to the Java VM for its whole
// life. All of them start with the first request and never end, so a request
// that stalls never keeps a queued one from a free thread.
class Workers {
 public:
  using Job = std::function<void(JNIEnv&)>;

  static auto instance(JavaVM& vm) -> Workers& {
    static Workers workers{vm};
    return workers;
  }

  void post(Job job) {
    {
      std::scoped_lock lock(mutex);
      jobs.push_back(std::move(job));
    }
    ready.notify_one();
  }

 private:
  explicit Workers(JavaVM& vm) {
    for (std::size_t index = 0; index < worker_count; ++index) {
      std::thread([this, &vm] { run(vm); }).detach();
    }
  }

  void run(JavaVM& vm) {
    mln::platform::setCurrentThreadName("MLN HTTP");
    const auto environment = jni::GetAttachedEnv(vm);
    auto lock = std::unique_lock{mutex};
    for (;;) {
      ready.wait(lock, [this] { return !jobs.empty(); });
      auto job = std::move(jobs.front());
      jobs.pop_front();
      lock.unlock();
      job(*environment);
      lock.lock();
    }
  }

  std::mutex mutex;
  std::condition_variable ready;
  std::deque<Job> jobs;
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
        ),
        transfer(std::make_shared<Transfer>()) {
    delivery->start();

    // The host's header transform runs for every request, as on the other
    // targets, even one that then fails.
    auto url = platform::offline_url(resource);
    auto headers =
      platform::http::request_headers(resource, url, platformContext);
    auto* const jvm = platform::android::java_vm();
    if (jvm == nullptr) {
      delivery->complete(
        platform::http::error_response(
          Reason::Other,
          "Android platform services are not initialized; call "
          "mln_android_init before HTTP requests"
        )
      );
      return;
    }

    Workers::instance(*jvm).post(
      [delivery = delivery, transfer = transfer, url = std::move(url),
       headers = std::move(headers)](JNIEnv& env) mutable {
        if (delivery->is_canceled()) {
          return;
        }
        auto response = std::optional<Response>{};
        try {
          response =
            fetch(env, *transfer, delivery->resource, url, std::move(headers));
        } catch (const jni::PendingJavaException&) {
          response = failure_response(env);
          transfer->untrack();
        }
        if (response) {
          delivery->complete(std::move(*response));
        }
      }
    );
  }

  HTTPRequest(const HTTPRequest&) = delete;
  auto operator=(const HTTPRequest&) -> HTTPRequest& = delete;
  HTTPRequest(HTTPRequest&&) = delete;
  auto operator=(HTTPRequest&&) -> HTTPRequest& = delete;

  ~HTTPRequest() override {
    delivery->cancel();
    transfer->cancel();
  }

 private:
  std::shared_ptr<platform::http::Delivery> delivery;
  std::shared_ptr<Transfer> transfer;
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
