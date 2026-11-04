# The React Native module and surface view. The binding's own pod,
# MaplibreNativeFfi from @maplibre/native-ffi, carries the Hermes adapter and
# the C library.
require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

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

  s.source_files = ["ios/*.{h,mm}", "cpp/*.{h,cpp}"]
  s.pod_target_xcconfig = { "HEADER_SEARCH_PATHS" => "\"#{__dir__}/cpp\"" }

  s.dependency "MaplibreNativeFfi"
  install_modules_dependencies(s)
end
