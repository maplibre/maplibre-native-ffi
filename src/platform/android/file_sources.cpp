#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <android/asset_manager.h>
#include <mln/actor/actor.hpp>
#include <mln/platform/settings.hpp>
#include <mln/storage/asset_file_source.hpp>
#include <mln/storage/file_source_request.hpp>
#include <mln/storage/local_file_request.hpp>
#include <mln/storage/local_file_source.hpp>
#include <mln/storage/resource.hpp>
#include <mln/storage/resource_options.hpp>
#include <mln/storage/response.hpp>
#include <mln/util/async_request.hpp>
#include <mln/util/client_options.hpp>
#include <mln/util/constants.hpp>
#include <mln/util/thread.hpp>
#include <mln/util/url.hpp>
#include <sys/types.h>

#include "platform/android/asset_manager.hpp"

namespace {

constexpr auto android_asset_path_prefix = std::string_view{"/android_asset/"};

auto path_from_url(std::string_view url, std::string_view protocol)
  -> std::string {
  auto rest = url.substr(protocol.size());
  rest = rest.substr(0, rest.find_first_of("?#"));
  return mln::util::percentDecode(std::string{rest});
}

struct AssetCloser {
  void operator()(AAsset* asset) const noexcept {
    if (asset != nullptr) {
      AAsset_close(asset);
    }
  }
};

auto read_asset(
  AAssetManager* manager, const std::string& path,
  const std::optional<std::pair<std::uint64_t, std::uint64_t>>& data_range
) -> mln::Response {
  auto response = mln::Response{};
  if (manager == nullptr) {
    response.error = std::make_unique<mln::Response::Error>(
      mln::Response::Error::Reason::Other,
      "Android AssetManager is not initialized; call mln_android_init before "
      "asset requests"
    );
    return response;
  }

  auto asset = std::unique_ptr<AAsset, AssetCloser>{
    AAssetManager_open(manager, path.c_str(), AASSET_MODE_RANDOM)
  };
  if (!asset) {
    response.error = std::make_unique<mln::Response::Error>(
      mln::Response::Error::Reason::NotFound, "Could not read asset"
    );
    return response;
  }

  const auto total = AAsset_getLength64(asset.get());
  auto offset = off64_t{0};
  auto length = total;
  if (data_range.has_value()) {
    offset = static_cast<off64_t>(data_range->first);
    const auto last = static_cast<off64_t>(data_range->second);
    if (last < offset) {
      response.data = std::make_shared<std::string>();
      return response;
    }
    length = last - offset + 1;
  }

  if (offset > total) {
    response.data = std::make_shared<std::string>();
    return response;
  }
  const auto remaining = total - offset;
  if (length > remaining) {
    length = remaining;
  }

  if (offset != 0 && AAsset_seek64(asset.get(), offset, SEEK_SET) < 0) {
    response.error = std::make_unique<mln::Response::Error>(
      mln::Response::Error::Reason::Other, "Cannot seek asset " + path
    );
    return response;
  }

  auto data = std::string(static_cast<std::size_t>(length), '\0');
  auto filled = std::size_t{0};
  while (filled < data.size()) {
    const auto got =
      AAsset_read(asset.get(), data.data() + filled, data.size() - filled);
    if (got < 0) {
      response.error = std::make_unique<mln::Response::Error>(
        mln::Response::Error::Reason::Other, "Cannot read asset " + path
      );
      return response;
    }
    if (got == 0) {
      break;
    }
    filled += static_cast<std::size_t>(got);
  }
  if (filled != data.size()) {
    response.error = std::make_unique<mln::Response::Error>(
      mln::Response::Error::Reason::Other, "Cannot read asset " + path
    );
    return response;
  }
  response.data = std::make_shared<std::string>(std::move(data));
  return response;
}

class AndroidFileSourceImpl {
 public:
  AndroidFileSourceImpl(
    std::string_view protocol_, const mln::ResourceOptions& resource_options_,
    const mln::ClientOptions& client_options_
  )
      : protocol(protocol_),
        resource_options(resource_options_.clone()),
        client_options(client_options_.clone()) {}

  void request(
    const mln::Resource& resource,
    const mln::ActorRef<mln::FileSourceRequest>& req
  ) {
    if (!resource.url.starts_with(protocol)) {
      auto response = mln::Response{};
      response.error = std::make_unique<mln::Response::Error>(
        mln::Response::Error::Reason::Other, "Invalid local resource URL"
      );
      req.invoke(&mln::FileSourceRequest::setResponse, response);
      return;
    }

    auto path = path_from_url(resource.url, protocol);
    if (protocol == mln::util::ASSET_PROTOCOL) {
      path.erase(0, path.find_first_not_of('/'));
      auto root = getResourceOptions().assetPath();
      if (!root.ends_with('/')) {
        root += '/';
      }
      path = root + path;
    }

    if (path.starts_with(android_asset_path_prefix)) {
      auto asset_path = path.substr(android_asset_path_prefix.size());
      asset_path.erase(0, asset_path.find_first_not_of('/'));
      req.invoke(
        &mln::FileSourceRequest::setResponse,
        read_asset(
          mln::platform::android_asset_manager(), asset_path, resource.dataRange
        )
      );
    } else {
      mln::requestLocalFile(path, req, resource.dataRange);
    }
  }

  void setResourceOptions(mln::ResourceOptions options) {
    const std::scoped_lock lock(resource_options_mutex);
    resource_options = options;
  }

  auto getResourceOptions() -> mln::ResourceOptions {
    const std::scoped_lock lock(resource_options_mutex);
    return resource_options.clone();
  }

  void setClientOptions(mln::ClientOptions options) {
    const std::scoped_lock lock(client_options_mutex);
    client_options = options;
  }

  auto getClientOptions() -> mln::ClientOptions {
    const std::scoped_lock lock(client_options_mutex);
    return client_options.clone();
  }

 private:
  mutable std::mutex resource_options_mutex;
  mutable std::mutex client_options_mutex;
  const std::string_view protocol;
  mln::ResourceOptions resource_options;
  mln::ClientOptions client_options;
};

}  // namespace

namespace mln {

class AssetFileSource::Impl : public AndroidFileSourceImpl {
 public:
  Impl(
    const ActorRef<Impl>&, const ResourceOptions& options,
    const ClientOptions& client_options
  )
      : AndroidFileSourceImpl(util::ASSET_PROTOCOL, options, client_options) {}
};

class LocalFileSource::Impl : public AndroidFileSourceImpl {
 public:
  Impl(
    const ActorRef<Impl>&, const ResourceOptions& options,
    const ClientOptions& client_options
  )
      : AndroidFileSourceImpl(util::FILE_PROTOCOL, options, client_options) {}
};

AssetFileSource::AssetFileSource(
  const ResourceOptions& resourceOptions, const ClientOptions& clientOptions
)
    : impl(
        std::make_unique<util::Thread<Impl>>(
          util::makeThreadPrioritySetter(
            platform::EXPERIMENTAL_THREAD_PRIORITY_FILE
          ),
          "AssetFileSource", resourceOptions.clone(), clientOptions.clone()
        )
      ) {}

AssetFileSource::~AssetFileSource() = default;

auto AssetFileSource::request(const Resource& resource, Callback callback)
  -> std::unique_ptr<AsyncRequest> {
  auto req = std::make_unique<FileSourceRequest>(std::move(callback));
  impl->actor().invoke(&Impl::request, resource, req->actor());
  return req;
}

auto AssetFileSource::canRequest(const Resource& resource) const -> bool {
  return resource.url.starts_with(util::ASSET_PROTOCOL);
}

void AssetFileSource::pause() { impl->pause(); }

void AssetFileSource::resume() { impl->resume(); }

void AssetFileSource::setResourceOptions(ResourceOptions options) {
  impl->actor().invoke(&Impl::setResourceOptions, options.clone());
}

auto AssetFileSource::getResourceOptions() -> ResourceOptions {
  return impl->actor().ask(&Impl::getResourceOptions).get();
}

void AssetFileSource::setClientOptions(ClientOptions options) {
  impl->actor().invoke(&Impl::setClientOptions, options.clone());
}

auto AssetFileSource::getClientOptions() -> ClientOptions {
  return impl->actor().ask(&Impl::getClientOptions).get();
}

LocalFileSource::LocalFileSource(
  const ResourceOptions& resourceOptions, const ClientOptions& clientOptions
)
    : impl(
        std::make_unique<util::Thread<Impl>>(
          util::makeThreadPrioritySetter(
            platform::EXPERIMENTAL_THREAD_PRIORITY_FILE
          ),
          "LocalFileSource", resourceOptions.clone(), clientOptions.clone()
        )
      ) {}

LocalFileSource::~LocalFileSource() = default;

auto LocalFileSource::request(const Resource& resource, Callback callback)
  -> std::unique_ptr<AsyncRequest> {
  auto req = std::make_unique<FileSourceRequest>(std::move(callback));
  impl->actor().invoke(&Impl::request, resource, req->actor());
  return req;
}

auto LocalFileSource::canRequest(const Resource& resource) const -> bool {
  return resource.url.starts_with(util::FILE_PROTOCOL);
}

void LocalFileSource::pause() { impl->pause(); }

void LocalFileSource::resume() { impl->resume(); }

void LocalFileSource::setResourceOptions(ResourceOptions options) {
  impl->actor().invoke(&Impl::setResourceOptions, options.clone());
}

auto LocalFileSource::getResourceOptions() -> ResourceOptions {
  return impl->actor().ask(&Impl::getResourceOptions).get();
}

void LocalFileSource::setClientOptions(ClientOptions options) {
  impl->actor().invoke(&Impl::setClientOptions, options.clone());
}

auto LocalFileSource::getClientOptions() -> ClientOptions {
  return impl->actor().ask(&Impl::getClientOptions).get();
}

}  // namespace mln
