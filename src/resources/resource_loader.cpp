#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <mbgl/storage/database_file_source.hpp>
#include <mbgl/storage/file_source.hpp>
#include <mbgl/storage/file_source_request.hpp>
#include <mbgl/storage/main_resource_loader.hpp>
#include <mbgl/storage/online_file_source.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/storage/resource_transform.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/async_request.hpp>
#include <mbgl/util/client_options.hpp>
#include <mbgl/util/event.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/tile_server_options.hpp>

#include "resources/resource_loader.hpp"

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "resources/custom_resource_provider.hpp"
#include "runtime/runtime.hpp"

namespace mln::core {
namespace {

auto can_request_network(const mbgl::Resource& resource) -> bool {
  return resource.hasLoadingMethod(mbgl::Resource::LoadingMethod::Network);
}

auto equals_ignoring_case(std::string_view left, std::string_view right)
  -> bool {
  return std::ranges::equal(left, right, [](char left_char, char right_char) {
    return std::tolower(static_cast<unsigned char>(left_char)) ==
           std::tolower(static_cast<unsigned char>(right_char));
  });
}

// Returns the RFC 3986 scheme of a URL, or nullopt when the URL carries none.
auto url_scheme(std::string_view url) -> std::optional<std::string_view> {
  const auto separator = url.find(':');
  if (separator == std::string_view::npos || separator == 0) {
    return std::nullopt;
  }
  const auto scheme = url.substr(0, separator);
  if (std::isalpha(static_cast<unsigned char>(scheme.front())) == 0) {
    return std::nullopt;
  }
  const auto is_scheme_character = [](char character) -> bool {
    return std::isalnum(static_cast<unsigned char>(character)) != 0 ||
           character == '+' || character == '-' || character == '.';
  };
  if (!std::ranges::all_of(scheme, is_scheme_character)) {
    return std::nullopt;
  }
  return scheme;
}

// Keeps enough URL context to identify the failing resource without copying
// credentials into public diagnostics. Query and fragment values are omitted,
// and RFC 3986 userinfo is removed from hierarchical URLs.
auto diagnostic_url(std::string_view url) -> std::string {
  const auto query = url.find('?');
  const auto fragment = url.find('#');
  const auto suffix = std::min(query, fragment);
  auto sanitized = std::string{url.substr(0, suffix)};

  const auto scheme_separator = sanitized.find(':');
  if (
    scheme_separator == std::string::npos ||
    sanitized.substr(scheme_separator + 1, 2) != "//"
  ) {
    return sanitized;
  }

  const auto authority_start = scheme_separator + 3;
  const auto authority_end = sanitized.find('/', authority_start);
  const auto at = sanitized.rfind(
    '@', authority_end == std::string::npos ? sanitized.size() : authority_end
  );
  if (at != std::string::npos && at >= authority_start) {
    sanitized.erase(authority_start, at - authority_start + 1);
  }
  return sanitized;
}

auto unsupported_scheme_response(
  std::string_view scheme, const std::string& url, bool provider_declined
) -> mbgl::Response {
  auto response = mbgl::Response{};
  // Reason::Other keeps this a terminal configuration error: it surfaces as
  // MLN_RESOURCE_ERROR_REASON_OTHER, and the tile and source paths retry or
  // silently absorb the reasons that describe transport and server state.
  response.error = std::make_unique<mbgl::Response::Error>(
    mbgl::Response::Error::Reason::Other,
    "unsupported URL scheme \"" + std::string{scheme} +
      "\" for network request \"" + diagnostic_url(url) +
      (provider_declined
         ? "\"; the registered resource provider declined this request; "
           "update it to serve this scheme"
         : "\"; register a resource provider with "
           "mln_runtime_set_resource_provider() to serve this scheme")
  );
  return response;
}

auto respond_immediately(
  const mbgl::Response& response, mbgl::FileSource::Callback callback
) -> std::unique_ptr<mbgl::AsyncRequest> {
  auto request = std::make_unique<mbgl::FileSourceRequest>(std::move(callback));
  request->actor().invoke(&mbgl::FileSourceRequest::setResponse, response);
  return request;
}

auto resource_kind_to_abi(mbgl::Resource::Kind kind) -> uint32_t {
  switch (kind) {
    case mbgl::Resource::Kind::Style:
      return MLN_RESOURCE_KIND_STYLE;
    case mbgl::Resource::Kind::Source:
      return MLN_RESOURCE_KIND_SOURCE;
    case mbgl::Resource::Kind::Tile:
      return MLN_RESOURCE_KIND_TILE;
    case mbgl::Resource::Kind::Glyphs:
      return MLN_RESOURCE_KIND_GLYPHS;
    case mbgl::Resource::Kind::SpriteImage:
      return MLN_RESOURCE_KIND_SPRITE_IMAGE;
    case mbgl::Resource::Kind::SpriteJSON:
      return MLN_RESOURCE_KIND_SPRITE_JSON;
    case mbgl::Resource::Kind::Image:
      return MLN_RESOURCE_KIND_IMAGE;
    case mbgl::Resource::Kind::Unknown:
    default:
      return MLN_RESOURCE_KIND_UNKNOWN;
  }
}

auto make_resource_transform(void* platform_context)
  -> mbgl::ResourceTransform {
  if (platform_context == nullptr) {
    return mbgl::ResourceTransform{};
  }

  return mbgl::ResourceTransform{
    [platform_context](
      mbgl::Resource::Kind kind, const std::string& url,
      mbgl::ResourceTransform::FinishedCallback finished
    ) -> void {
      std::string replacement_url;
      const auto status = invoke_resource_transform(
        platform_context, resource_kind_to_abi(kind), url.c_str(),
        replacement_url
      );
      if (status == MLN_STATUS_OK && !replacement_url.empty()) {
        finished(replacement_url);
        return;
      }
      finished(url);
    }
  };
}

class AbiNetworkFileSource final : public mbgl::FileSource {
 public:
  AbiNetworkFileSource(
    const mbgl::ResourceOptions& resource_options,
    const mbgl::ClientOptions& client_options
  )
      : resource_options_(resource_options.clone()),
        client_options_(client_options.clone()),
        native_(
          std::make_unique<mbgl::OnlineFileSource>(
            resource_options, client_options
          )
        ) {
    apply_resource_transform();
  }

  auto request(const mbgl::Resource& resource, Callback callback)
    -> std::unique_ptr<mbgl::AsyncRequest> override {
    const auto provider = find_resource_provider_for_platform_context(
      resource_options_.platformContext()
    );
    if (can_request_network(resource) && provider.has_value()) {
      auto request = request_custom_resource(
        resource, provider->callback, provider->user_data, callback
      );
      if (request != nullptr) {
        return request;
      }
    }
    if (!native_->canRequest(resource)) {
      return nullptr;
    }
    const auto unsupported = unsupported_network_scheme(resource.url);
    if (unsupported.has_value()) {
      return respond_immediately(
        unsupported_scheme_response(
          *unsupported, resource.url, provider.has_value()
        ),
        std::move(callback)
      );
    }
    return native_->request(resource, std::move(callback));
  }

  [[nodiscard]] auto canRequest(const mbgl::Resource& resource) const
    -> bool override {
    const auto provider = find_resource_provider_for_platform_context(
      resource_options_.platformContext()
    );
    return (can_request_network(resource) && provider.has_value()) ||
           native_->canRequest(resource);
  }

  void forward(
    const mbgl::Resource& resource, const mbgl::Response& response,
    std::function<void()> callback
  ) override {
    native_->forward(resource, response, std::move(callback));
  }

  [[nodiscard]] auto supportsCacheOnlyRequests() const -> bool override {
    return native_->supportsCacheOnlyRequests();
  }

  void pause() override { native_->pause(); }

  void resume() override { native_->resume(); }

  void setResourceTransform(mbgl::ResourceTransform transform) override {
    native_->setResourceTransform(std::move(transform));
  }

  void setResourceOptions(mbgl::ResourceOptions options) override {
    resource_options_ = options.clone();
    native_->setResourceOptions(std::move(options));
    apply_resource_transform();
  }

  auto getResourceOptions() -> mbgl::ResourceOptions override {
    return resource_options_.clone();
  }

  void setClientOptions(mbgl::ClientOptions options) override {
    client_options_ = options.clone();
    native_->setClientOptions(std::move(options));
  }

  auto getClientOptions() -> mbgl::ClientOptions override {
    return client_options_.clone();
  }

 private:
  // Reports the scheme of a URL the online source is unable to serve. The
  // resource loader routes file, asset, mbtiles, and pmtiles URLs to their own
  // sources ahead of the network, so anything left here reaches an HTTP
  // client. A registered resource transform rewrites URLs inside the online
  // source, so it keeps every scheme available.
  [[nodiscard]] auto unsupported_network_scheme(const std::string& url) const
    -> std::optional<std::string_view> {
    if (
      has_resource_transform_for_platform_context(
        resource_options_.platformContext()
      )
    ) {
      return std::nullopt;
    }
    const auto scheme = url_scheme(url);
    if (!scheme.has_value()) {
      return std::nullopt;
    }
    if (
      equals_ignoring_case(*scheme, "http") ||
      equals_ignoring_case(*scheme, "https")
    ) {
      return std::nullopt;
    }
    // Tile server options name a canonical scheme the online source expands
    // into its configured base URL.
    const auto alias = resource_options_.tileServerOptions().uriSchemeAlias();
    if (!alias.empty() && equals_ignoring_case(*scheme, alias)) {
      return std::nullopt;
    }
    return scheme;
  }

  void apply_resource_transform() {
    native_->setResourceTransform(
      make_resource_transform(resource_options_.platformContext())
    );
  }

  mbgl::ResourceOptions resource_options_;
  mbgl::ClientOptions client_options_;
  std::unique_ptr<mbgl::FileSource> native_;
};

}  // namespace

auto make_network_file_source(
  const mbgl::ResourceOptions& resource_options,
  const mbgl::ClientOptions& client_options
) noexcept -> std::unique_ptr<mbgl::FileSource> {
  try {
    return std::make_unique<AbiNetworkFileSource>(
      resource_options, client_options
    );
  } catch (const std::exception& exception) {
    set_thread_error(exception);
    return nullptr;
  } catch (...) {
    set_thread_error("network file source construction failed");
    return nullptr;
  }
}

auto make_database_file_source(
  const mbgl::ResourceOptions& resource_options,
  const mbgl::ClientOptions& client_options
) noexcept -> std::unique_ptr<mbgl::FileSource> {
  try {
    auto source = std::make_unique<mbgl::DatabaseFileSource>(
      resource_options, client_options
    );
    const auto maximum_cache_size =
      find_maximum_cache_size_for_platform_context(
        resource_options.platformContext()
      );
    if (maximum_cache_size.has_value()) {
      source->setMaximumAmbientCacheSize(
        *maximum_cache_size, [](std::exception_ptr exception) -> void {
          if (exception != nullptr) {
            mbgl::Log::Error(
              mbgl::Event::Database,
              "Failed to apply maximum ambient cache size: " +
                mbgl::util::toString(exception)
            );
          }
        }
      );
    }
    return source;
  } catch (const std::exception& exception) {
    set_thread_error(exception);
    return nullptr;
  } catch (...) {
    set_thread_error("database file source construction failed");
    return nullptr;
  }
}

auto make_main_resource_loader(
  const mbgl::ResourceOptions& resource_options,
  const mbgl::ClientOptions& client_options
) noexcept -> std::unique_ptr<mbgl::FileSource> {
  try {
    return std::make_unique<mbgl::MainResourceLoader>(
      resource_options, client_options
    );
  } catch (const std::exception& exception) {
    set_thread_error(exception);
    return nullptr;
  } catch (...) {
    set_thread_error("main resource loader construction failed");
    return nullptr;
  }
}

}  // namespace mln::core
