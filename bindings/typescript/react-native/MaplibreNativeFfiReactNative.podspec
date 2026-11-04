# The React Native module and surface view, with the binding's Hermes adapter
# and the C library. The npm package carries the binding's native sources and
# the XCFramework. In a workspace checkout, scripts/stage.sh copies them in.
require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))
symbols = "-include \"${PODS_TARGET_SRCROOT}/native/hermes/symbols.h\""

Pod::Spec.new do |s|
  s.name = "MaplibreNativeFfiReactNative"
  s.version = package["version"]
  s.summary = package["description"]
  s.homepage = "https://maplibre.org/maplibre-native-ffi/"
  s.license = package["license"]
  s.authors = "MapLibre contributors"
  s.platforms = { :ios => min_ios_version_supported }
  s.source = {
    :git => "https://github.com/maplibre/maplibre-native-ffi.git",
    :tag => "v#{s.version}",
  }

  s.source_files = [
    "ios/*.{h,mm}",
    "cpp/*.{h,cpp}",
    "native/{generated_addon,generated_values,realm,runtime}.c",
    "native/{generated_addon,runtime,mutex}.h",
    "native/hermes/*.{h,cpp}",
  ]
  s.public_header_files = "ios/*.h"
  s.vendored_frameworks = "ios/MaplibreNativeC.xcframework"

  s.pod_target_xcconfig = {
    "HEADER_SEARCH_PATHS" => %w[cpp native native/include native/node-api-headers]
      .map { |path| "\"${PODS_TARGET_SRCROOT}/#{path}\"" }.join(" "),
    "GCC_PREPROCESSOR_DEFINITIONS" => "$(inherited) NAPI_VERSION=8",
    # The adapter's Node-API functions stay private to this library.
    "OTHER_CFLAGS" => "$(inherited) #{symbols}",
    "OTHER_CPLUSPLUSFLAGS" => "$(inherited) #{symbols}",
  }

  # React Native's helper adds the JSI and the rest of React Native's pods.
  install_modules_dependencies(s)
end
