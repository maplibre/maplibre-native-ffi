#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <mbgl/storage/database_file_source.hpp>
#include <mbgl/storage/file_source.hpp>
#include <mbgl/storage/file_source_manager.hpp>
#if defined(__EMSCRIPTEN__)
#include <mbgl/storage/http_file_source.hpp>
#endif
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

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "resources/custom_resource_provider.hpp"
#include "resources/resource_loader.hpp"
#include "runtime/runtime.hpp"

namespace mln::core {
namespace {

auto can_request_network(const mbgl::Resource& resource) -> bool {
  return resource.hasLoadingMethod(mbgl::Resource::LoadingMethod::Network);
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
#if defined(__EMSCRIPTEN__)
          std::make_unique<mbgl::HTTPFileSource>(
            resource_options, client_options
          )
#else
          std::make_unique<mbgl::OnlineFileSource>(
            resource_options, client_options
          )
#endif
        ) {
    apply_resource_transform();
  }

  auto request(const mbgl::Resource& resource, Callback callback)
    -> std::unique_ptr<mbgl::AsyncRequest> override {
    const auto provider =
      runtime_resource_provider(resource_options_.platformContext());
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
    return native_->request(resource, std::move(callback));
  }

  [[nodiscard]] auto canRequest(const mbgl::Resource& resource) const
    -> bool override {
    const auto has_provider =
      runtime_resource_provider(resource_options_.platformContext())
        .has_value();
    return (can_request_network(resource) && has_provider) ||
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
  static auto runtime_resource_provider(void* platform_context)
    -> std::optional<ResourceProvider> {
    const auto* runtime = find_runtime_for_platform_context(platform_context);
    if (runtime == nullptr || !runtime->has_resource_provider) {
      return std::nullopt;
    }
    return runtime->resource_provider;
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

#if defined(__EMSCRIPTEN__)
class CompletedRequest final : public mbgl::AsyncRequest {};

class BrowserResourceLoader final : public mbgl::FileSource {
 public:
  BrowserResourceLoader(
    const mbgl::ResourceOptions& resource_options,
    const mbgl::ClientOptions& client_options
  )
      : resource_options_(resource_options.clone()),
        client_options_(client_options.clone()) {
    refresh_sources();
  }

  auto request(const mbgl::Resource& resource, Callback callback)
    -> std::unique_ptr<mbgl::AsyncRequest> override {
    if (network_source_ != nullptr && network_source_->canRequest(resource)) {
      return network_source_->request(resource, std::move(callback));
    }

    auto response = mbgl::Response{};
    response.noContent = true;
    response.error = std::make_unique<mbgl::Response::Error>(
      mbgl::Response::Error::Reason::Other, "Unsupported resource request."
    );
    callback(response);
    return std::make_unique<CompletedRequest>();
  }

  [[nodiscard]] auto canRequest(const mbgl::Resource& resource) const
    -> bool override {
    return network_source_ != nullptr && network_source_->canRequest(resource);
  }

  void forward(
    const mbgl::Resource&, const mbgl::Response&, std::function<void()> callback
  ) override {
    if (callback) {
      callback();
    }
  }

  [[nodiscard]] auto supportsCacheOnlyRequests() const -> bool override {
    return false;
  }

  void pause() override {
    for_each_source([](mbgl::FileSource& source) { source.pause(); });
  }

  void resume() override {
    for_each_source([](mbgl::FileSource& source) { source.resume(); });
  }

  void setResourceTransform(mbgl::ResourceTransform transform) override {
    if (network_source_ != nullptr) {
      network_source_->setResourceTransform(std::move(transform));
    }
  }

  void setResourceOptions(mbgl::ResourceOptions options) override {
    resource_options_ = options.clone();
    for_each_source([&](mbgl::FileSource& source) {
      source.setResourceOptions(options.clone());
    });
  }

  auto getResourceOptions() -> mbgl::ResourceOptions override {
    return resource_options_.clone();
  }

  void setClientOptions(mbgl::ClientOptions options) override {
    client_options_ = options.clone();
    for_each_source([&](mbgl::FileSource& source) {
      source.setClientOptions(options.clone());
    });
  }

  auto getClientOptions() -> mbgl::ClientOptions override {
    return client_options_.clone();
  }

 private:
  using SourceFn = std::function<void(mbgl::FileSource&)>;

  void refresh_sources() {
    auto* manager = mbgl::FileSourceManager::get();
    network_source_ = manager->getFileSource(
      mbgl::FileSourceType::Network, resource_options_, client_options_
    );
  }

  void for_each_source(const SourceFn& fn) {
    if (network_source_ != nullptr) {
      fn(*network_source_);
    }
  }

  mbgl::ResourceOptions resource_options_;
  mbgl::ClientOptions client_options_;
  std::shared_ptr<mbgl::FileSource> network_source_;
};
#endif

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
    const auto* runtime =
      find_runtime_for_platform_context(resource_options.platformContext());
    if (runtime != nullptr && runtime->has_maximum_cache_size) {
      source->setMaximumAmbientCacheSize(
        runtime->maximum_cache_size, [](std::exception_ptr exception) -> void {
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
#if defined(__EMSCRIPTEN__)
    return std::make_unique<BrowserResourceLoader>(
      resource_options, client_options
    );
#else
    return std::make_unique<mbgl::MainResourceLoader>(
      resource_options, client_options
    );
#endif
  } catch (const std::exception& exception) {
    set_thread_error(exception);
    return nullptr;
  } catch (...) {
    set_thread_error("main resource loader construction failed");
    return nullptr;
  }
}

}  // namespace mln::core
