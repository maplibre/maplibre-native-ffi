# The Hermes adapter and the C library, for React Native apps on iOS, which
# install this pod through autolinking. The host still installs
# mln::hermes::Module itself; see native/hermes/module.h.
require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))
root = File.realpath(__dir__)
native = File.join(root, "native")
# The npm package carries these headers, and a workspace checkout has them
# elsewhere in the repository.
first_directory = lambda do |*candidates|
  candidates.find { |path| File.directory?(path) } || candidates.first
end
c_headers = first_directory.call(
  File.join(native, "include"), File.expand_path("../../include", root),
)
node_api_headers = first_directory.call(
  File.join(native, "node-api-headers"),
  File.join(root, "node_modules/node-api-headers/include"),
)
symbols = "-include \"#{native}/hermes/symbols.h\""

Pod::Spec.new do |s|
  s.name = "MaplibreNativeFfi"
  s.version = package["version"]
  s.summary = "The Hermes adapter of MapLibre Native FFI's TypeScript binding."
  s.homepage = package["homepage"] || "https://maplibre.org/maplibre-native-ffi/"
  s.license = package["license"]
  s.authors = "MapLibre contributors"
  s.platforms = { :ios => min_ios_version_supported }
  s.source = {
    :git => "https://github.com/maplibre/maplibre-native-ffi.git",
    :tag => "v#{s.version}",
  }

  s.source_files = [
    "native/{generated_addon,generated_values,realm,runtime}.c",
    "native/{generated_addon,runtime,mutex}.h",
    "native/hermes/*.{h,cpp}",
  ]
  s.public_header_files = "native/hermes/module.h"
  # The npm package carries the XCFramework. In a workspace checkout, put one
  # there yourself.
  s.vendored_frameworks = "native/MaplibreNativeC.xcframework"

  s.pod_target_xcconfig = {
    "HEADER_SEARCH_PATHS" => [native, c_headers, node_api_headers]
      .map { |path| "\"#{path}\"" }.join(" "),
    "GCC_PREPROCESSOR_DEFINITIONS" => "$(inherited) NAPI_VERSION=8",
    # The adapter's Node-API functions stay private to this library.
    "OTHER_CFLAGS" => "$(inherited) #{symbols}",
    "OTHER_CPLUSPLUSFLAGS" => "$(inherited) #{symbols}",
  }

  # React Native's helper adds the JSI and the rest of React Native's pods.
  install_modules_dependencies(s)
end
