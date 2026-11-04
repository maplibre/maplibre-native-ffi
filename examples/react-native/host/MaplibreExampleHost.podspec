require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name = "MaplibreExampleHost"
  s.version = "0.0.0"
  s.summary = package["description"]
  s.homepage = "https://github.com/maplibre/maplibre-native-ffi"
  s.license = "BSD-2-Clause"
  s.authors = "MapLibre contributors"
  s.platforms = { :ios => min_ios_version_supported }
  s.source = { :path => "." }

  s.source_files = ["ios/*.{h,mm}", "cpp/*.{h,cpp}"]
  s.pod_target_xcconfig = { "HEADER_SEARCH_PATHS" => "\"#{__dir__}/cpp\"" }

  # The binding's pod, which autolinking installs from @maplibre/native-ffi.
  s.dependency "MaplibreNativeFfi"
  install_modules_dependencies(s)
end
