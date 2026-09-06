// Marks a request as serving an offline download, which Mapbox endpoints bill
// separately from ordinary map use. Every HTTP file source the C API ships
// resolves the URL through here, so the requests match across builds.

#pragma once

#include <string>

#include <mln/storage/resource.hpp>
#include <mln/util/url.hpp>

namespace mln::platform {

inline auto is_mapbox_endpoint(const std::string& url) -> bool {
  const auto parsed = mln::util::URL{url};
  const auto host = url.substr(parsed.domain.first, parsed.domain.second);
  return host == "mapbox.com" || host == "mapbox.cn" ||
         host.ends_with(".mapbox.com") || host.ends_with(".mapbox.cn");
}

// Returns the URL to request, which is the resource's own unless it is an
// offline download from a Mapbox endpoint.
inline auto offline_url(const mln::Resource& resource) -> std::string {
  auto url = resource.url;
  if (
    resource.usage != mln::Resource::Usage::Offline || !is_mapbox_endpoint(url)
  ) {
    return url;
  }

  // The marker goes before any fragment, so it stays part of the query.
  const auto fragment = url.find('#');
  const auto query = url.find('?');
  const auto separator = query == std::string::npos ||
                             (fragment != std::string::npos && fragment < query)
                           ? '?'
                           : '&';
  url.insert(
    fragment == std::string::npos ? url.size() : fragment,
    std::string{separator} + "offline=true"
  );
  return url;
}

}  // namespace mln::platform
