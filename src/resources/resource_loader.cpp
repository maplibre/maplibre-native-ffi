#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <mln/storage/database_file_source.hpp>
#include <mln/storage/file_source.hpp>
#include <mln/storage/file_source_request.hpp>
#include <mln/storage/main_resource_loader.hpp>
#include <mln/storage/online_file_source.hpp>
#include <mln/storage/resource.hpp>
#include <mln/storage/resource_options.hpp>
#include <mln/storage/resource_transform.hpp>
#include <mln/storage/response.hpp>
#include <mln/util/async_request.hpp>
#include <mln/util/client_options.hpp>
#include <mln/util/mapbox.hpp>
#include <mln/util/tile_server_options.hpp>

#include "resources/resource_loader.hpp"

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "resources/custom_resource_provider.hpp"
#include "runtime/runtime.hpp"

namespace mln::core {
namespace {

auto can_request_network(const mln::Resource& resource) -> bool {
  return resource.hasLoadingMethod(mln::Resource::LoadingMethod::Network);
}

auto equals_ignoring_case(std::string_view left, std::string_view right)
  -> bool {
  return std::ranges::equal(left, right, [](char left_char, char right_char) {
    return std::tolower(static_cast<unsigned char>(left_char)) ==
           std::tolower(static_cast<unsigned char>(right_char));
  });
}

// RFC 3986 schemes start with a letter and continue with letters, digits,
// '+', '-', or '.'. A colon in a path alone does not identify a scheme.
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

// Strips the query, fragment, and RFC 3986 userinfo so a diagnostic never
// carries credentials.
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
) -> mln::Response {
  auto response = mln::Response{};
  // Reason::Other makes this terminal; the transport and server reasons are
  // retried or absorbed by the tile and source paths.
  response.error = std::make_unique<mln::Response::Error>(
    mln::Response::Error::Reason::Other,
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
  const mln::Response& response, mln::FileSource::Callback callback
) -> std::unique_ptr<mln::AsyncRequest> {
  auto request = std::make_unique<mln::FileSourceRequest>(std::move(callback));
  request->actor().invoke(&mln::FileSourceRequest::setResponse, response);
  return request;
}

auto resource_kind_to_abi(mln::Resource::Kind kind) -> mln_resource_kind {
  switch (kind) {
    case mln::Resource::Kind::Style:
      return MLN_RESOURCE_KIND_STYLE;
    case mln::Resource::Kind::Source:
      return MLN_RESOURCE_KIND_SOURCE;
    case mln::Resource::Kind::Tile:
      return MLN_RESOURCE_KIND_TILE;
    case mln::Resource::Kind::Glyphs:
      return MLN_RESOURCE_KIND_GLYPHS;
    case mln::Resource::Kind::SpriteImage:
      return MLN_RESOURCE_KIND_SPRITE_IMAGE;
    case mln::Resource::Kind::SpriteJSON:
      return MLN_RESOURCE_KIND_SPRITE_JSON;
    case mln::Resource::Kind::Image:
      return MLN_RESOURCE_KIND_IMAGE;
    case mln::Resource::Kind::Unknown:
    default:
      return MLN_RESOURCE_KIND_UNKNOWN;
  }
}

// Applies the per-kind normalization mln::OnlineFileSource would apply, so a
// provider sees the URL the built-in path would have requested. Kinds with no
// canonical form, and URLs normalization rejects, fall back to the request URL
// so a provider stays reachable for requests the native path refuses.
auto resolve_resource_url(
  const mln::ResourceOptions& resource_options, const mln::Resource& resource
) -> std::string {
  const auto& options = resource_options.tileServerOptions();
  const auto& api_key = resource_options.apiKey();
  try {
    switch (resource.kind) {
      case mln::Resource::Kind::Style:
        return mln::util::mapbox::normalizeStyleURL(
          options, resource.url, api_key
        );
      case mln::Resource::Kind::Source:
        return mln::util::mapbox::normalizeSourceURL(
          options, resource.url, api_key
        );
      case mln::Resource::Kind::Glyphs:
        return mln::util::mapbox::normalizeGlyphsURL(
          options, resource.url, api_key
        );
      case mln::Resource::Kind::SpriteImage:
      case mln::Resource::Kind::SpriteJSON:
        return mln::util::mapbox::normalizeSpriteURL(
          options, resource.url, api_key
        );
      case mln::Resource::Kind::Tile:
        return mln::util::mapbox::normalizeTileURL(
          options, resource.url, api_key
        );
      case mln::Resource::Kind::Unknown:
      case mln::Resource::Kind::Image:
      default:
        return resource.url;
    }
  } catch (...) {
    return resource.url;
  }
}

auto make_resource_transform(void* platform_context) -> mln::ResourceTransform {
  if (platform_context == nullptr) {
    return mln::ResourceTransform{};
  }

  return mln::ResourceTransform{
    [platform_context](
      mln::Resource::Kind kind, const std::string& url,
      mln::ResourceTransform::FinishedCallback finished
    ) -> void {
      invoke_resource_transform(
        platform_context, resource_kind_to_abi(kind), url, std::move(finished)
      );
    }
  };
}

class AbiNetworkFileSource final : public mln::FileSource {
 public:
  AbiNetworkFileSource(
    const mln::ResourceOptions& resource_options,
    const mln::ClientOptions& client_options
  )
      : resource_options_(resource_options.clone()),
        client_options_(client_options.clone()),
        native_(
          std::make_shared<mln::OnlineFileSource>(
            resource_options, client_options
          )
        ) {
    apply_resource_transform(resource_options_.platformContext());
  }

  auto request(const mln::Resource& resource, Callback callback)
    -> std::unique_ptr<mln::AsyncRequest> override {
    const auto options = resource_options_snapshot();
    auto provider_declined = false;
    if (can_request_network(resource)) {
      // Release the lease before falling through to native loading: the native
      // source may invoke a transform that needs the runtime registry lock.
      const auto provider = acquire_resource_provider_for_platform_context(
        options.platformContext()
      );
      if (provider.has_value()) {
        provider_declined = true;
        auto resolved_url = resolve_resource_url(options, resource);
        // A request outside the provider's routes passes through, as one the
        // provider declined does.
        if (
          provider->matches(resource_kind_to_abi(resource.kind), resolved_url)
        ) {
          auto request = request_custom_resource(
            resource, std::move(resolved_url), provider->callback(),
            provider->user_data(), callback,
            [native = native_, resource](Callback next) {
              return request_native(
                native, native->getResourceOptions(), resource, true,
                std::move(next)
              );
            }
          );
          if (request != nullptr) {
            return request;
          }
        }
      }
    }
    return request_native(
      native_, options, resource, provider_declined, std::move(callback)
    );
  }

  [[nodiscard]] auto canRequest(const mln::Resource& resource) const
    -> bool override {
    const auto options = resource_options_snapshot();
    // A provider claims only the requests its routes match, as request() does,
    // so a request outside those routes stays available to another file source.
    if (can_request_network(resource)) {
      const auto provider = acquire_resource_provider_for_platform_context(
        options.platformContext()
      );
      if (
        provider.has_value() && provider->matches(
                                  resource_kind_to_abi(resource.kind),
                                  resolve_resource_url(options, resource)
                                )
      ) {
        return true;
      }
    }
    return native_->canRequest(resource);
  }

  void forward(
    const mln::Resource& resource, const mln::Response& response,
    std::function<void()> callback
  ) override {
    native_->forward(resource, response, std::move(callback));
  }

  [[nodiscard]] auto supportsCacheOnlyRequests() const -> bool override {
    return native_->supportsCacheOnlyRequests();
  }

  void pause() override { native_->pause(); }

  void resume() override { native_->resume(); }

  void setResourceTransform(mln::ResourceTransform transform) override {
    native_->setResourceTransform(std::move(transform));
  }

  void setResourceOptions(mln::ResourceOptions options) override {
    auto platform_context = options.platformContext();
    {
      const std::scoped_lock lock(resource_options_mutex_);
      resource_options_ = options.clone();
    }
    native_->setResourceOptions(std::move(options));
    apply_resource_transform(platform_context);
  }

  auto getResourceOptions() -> mln::ResourceOptions override {
    return resource_options_snapshot();
  }

  void setClientOptions(mln::ClientOptions options) override {
    client_options_ = options.clone();
    native_->setClientOptions(std::move(options));
  }

  auto getClientOptions() -> mln::ClientOptions override {
    return client_options_.clone();
  }

 private:
  static auto request_native(
    const std::shared_ptr<mln::FileSource>& native,
    const mln::ResourceOptions& options, const mln::Resource& resource,
    bool provider_declined, Callback callback
  ) -> std::unique_ptr<mln::AsyncRequest> {
    if (!native->canRequest(resource)) {
      auto result = mln::Response{};
      result.error = std::make_unique<mln::Response::Error>(
        mln::Response::Error::Reason::Other,
        "native loader cannot request this resource"
      );
      return respond_immediately(result, std::move(callback));
    }
    const auto unsupported = unsupported_network_scheme(options, resource.url);
    if (unsupported.has_value()) {
      return respond_immediately(
        unsupported_scheme_response(
          *unsupported, resource.url, provider_declined
        ),
        std::move(callback)
      );
    }
    return native->request(resource, std::move(callback));
  }

  // Snapshots the options so reads never race a replacement. The mutex guards
  // only this member and is released before any other lock is taken.
  [[nodiscard]] auto resource_options_snapshot() const -> mln::ResourceOptions {
    const std::scoped_lock lock(resource_options_mutex_);
    return resource_options_.clone();
  }

  // Reports the scheme of a URL the online source is unable to serve. A
  // registered resource transform rewrites URLs inside the online source, so it
  // keeps every scheme available.
  [[nodiscard]] static auto unsupported_network_scheme(
    const mln::ResourceOptions& options, const std::string& url
  ) -> std::optional<std::string_view> {
    if (
      has_resource_transform_for_platform_context(options.platformContext())
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
    const auto alias = options.tileServerOptions().uriSchemeAlias();
    if (!alias.empty() && equals_ignoring_case(*scheme, alias)) {
      return std::nullopt;
    }
    return scheme;
  }

  void apply_resource_transform(void* platform_context) {
    native_->setResourceTransform(make_resource_transform(platform_context));
  }

  mutable std::mutex resource_options_mutex_;
  mln::ResourceOptions resource_options_;
  mln::ClientOptions client_options_;
  std::shared_ptr<mln::FileSource> native_;
};

}  // namespace

auto make_network_file_source(
  const mln::ResourceOptions& resource_options,
  const mln::ClientOptions& client_options
) noexcept -> std::unique_ptr<mln::FileSource> {
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
  const mln::ResourceOptions& resource_options,
  const mln::ClientOptions& client_options
) noexcept -> std::unique_ptr<mln::FileSource> {
  try {
    auto source = std::make_unique<mln::DatabaseFileSource>(
      resource_options, client_options
    );
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
  const mln::ResourceOptions& resource_options,
  const mln::ClientOptions& client_options
) noexcept -> std::unique_ptr<mln::FileSource> {
  try {
    return std::make_unique<mln::MainResourceLoader>(
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
