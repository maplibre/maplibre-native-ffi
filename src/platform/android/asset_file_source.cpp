#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <mln/actor/actor.hpp>
#include <mln/platform/settings.hpp>
#include <mln/storage/asset_file_source.hpp>
#include <mln/storage/file_source_request.hpp>
#include <mln/storage/resource.hpp>
#include <mln/storage/resource_options.hpp>
#include <mln/storage/response.hpp>
#include <mln/util/async_request.hpp>
#include <mln/util/client_options.hpp>
#include <mln/util/constants.hpp>
#include <mln/util/thread.hpp>
#include <mln/util/url.hpp>

#include <android/asset_manager.h>
#include <sys/types.h>

#include "platform/android/asset_manager.hpp"

namespace {

constexpr auto android_asset_file_prefix =
  std::string_view{"file:///android_asset"};

auto accepts_url(std::string_view url) -> bool {
  if (url.starts_with(mln::util::ASSET_PROTOCOL)) {
    return true;
  }
  if (!url.starts_with(android_asset_file_prefix)) {
    return false;
  }
  const auto rest = url.substr(android_asset_file_prefix.size());
  return rest.empty() || rest.front() == '/' || rest.front() == '?' ||
         rest.front() == '#';
}

auto asset_path_from_url(std::string_view url) -> std::string {
  auto rest = std::string_view{};
  if (url.starts_with(mln::util::ASSET_PROTOCOL)) {
    rest =
      url.substr(std::char_traits<char>::length(mln::util::ASSET_PROTOCOL));
  } else {
    rest = url.substr(android_asset_file_prefix.size());
    if (!rest.empty() && rest.front() == '/') {
      rest.remove_prefix(1);
    }
  }

  const auto suffix = rest.find_first_of("?#");
  if (suffix != std::string_view::npos) {
    rest = rest.substr(0, suffix);
  }

  auto path = mln::util::percentDecode(std::string{rest});
  while (path.starts_with('/')) {
    path.erase(0, 1);
  }
  return path;
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

}  // namespace

namespace mln {

class AssetFileSource::Impl {
 public:
  Impl(
    const ActorRef<Impl>&, const ResourceOptions& resource_options_,
    const ClientOptions& client_options_
  )
      : resource_options(resource_options_.clone()),
        client_options(client_options_.clone()) {}

  void request(
    const Resource& resource, const ActorRef<FileSourceRequest>& req
  ) {
    if (!accepts_url(resource.url)) {
      auto response = Response{};
      response.error = std::make_unique<Response::Error>(
        Response::Error::Reason::Other, "Invalid asset URL"
      );
      req.invoke(&FileSourceRequest::setResponse, response);
      return;
    }

    req.invoke(
      &FileSourceRequest::setResponse,
      read_asset(
        mln::platform::android_asset_manager(),
        asset_path_from_url(resource.url), resource.dataRange
      )
    );
  }

  void setResourceOptions(ResourceOptions options) {
    const std::scoped_lock lock(resource_options_mutex);
    resource_options = options;
  }

  auto getResourceOptions() -> ResourceOptions {
    const std::scoped_lock lock(resource_options_mutex);
    return resource_options.clone();
  }

  void setClientOptions(ClientOptions options) {
    const std::scoped_lock lock(client_options_mutex);
    client_options = options;
  }

  auto getClientOptions() -> ClientOptions {
    const std::scoped_lock lock(client_options_mutex);
    return client_options.clone();
  }

 private:
  mutable std::mutex resource_options_mutex;
  mutable std::mutex client_options_mutex;
  ResourceOptions resource_options;
  ClientOptions client_options;
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
  return accepts_url(resource.url);
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

}  // namespace mln
