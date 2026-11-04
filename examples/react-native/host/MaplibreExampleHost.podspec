require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))
root = File.realpath(File.join(__dir__, "../../.."))
# The workspace's binding, linked into this package's node_modules.
binding = File.realpath(File.join(__dir__, "node_modules/@maplibre/native-ffi"))
symbols = "-include \"#{binding}/native/hermes/symbols.h\""

Pod::Spec.new do |s|
  s.name = "MaplibreExampleHost"
  s.version = "0.0.0"
  s.summary = package["description"]
  s.homepage = "https://github.com/maplibre/maplibre-native-ffi"
  s.license = "BSD-2-Clause"
  s.authors = "MapLibre contributors"
  s.platforms = { :ios => min_ios_version_supported }
  s.source = { :path => "." }

  # The host, and the binding's Hermes adapter through ios/binding.
  s.source_files = ["ios/**/*.{h,mm,c,cpp}", "cpp/*.{h,cpp}"]
  # The C library, from scripts/xcframework.sh.
  s.vendored_frameworks = "build/MaplibreNativeC.xcframework"

  s.pod_target_xcconfig = {
    "HEADER_SEARCH_PATHS" => [
      "\"#{__dir__}/cpp\"",
      "\"#{binding}/native\"",
      "\"#{binding}/native/hermes\"",
      "\"#{binding}/node_modules/node-api-headers/include\"",
      "\"#{root}/include\"",
    ].join(" "),
    "GCC_PREPROCESSOR_DEFINITIONS" => "$(inherited) NAPI_VERSION=8",
    # The adapter's Node-API functions stay private to this library.
    "OTHER_CFLAGS" => "$(inherited) #{symbols}",
    "OTHER_CPLUSPLUSFLAGS" => "$(inherited) #{symbols}",
  }

  install_modules_dependencies(s)
end
