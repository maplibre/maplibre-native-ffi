require 'json'
manifest = JSON.parse(File.read(File.join(__dir__, 'package.json')))

Pod::Spec.new do |s|
  s.name = 'MaplibreNativeFfi'
  s.version = manifest['version']
  s.summary = manifest['description']
  s.homepage = 'https://maplibre.org/maplibre-native-ffi/'
  s.license = 'BSD-2-Clause'
  s.author = 'MapLibre'
  s.source = { :git => 'https://github.com/maplibre/maplibre-native-ffi.git', :tag => "v#{s.version}" }
  s.platforms = { :ios => '15.1' }
  s.source_files = 'native/*.{c,h}', 'native/react-native/*.{cpp,h}', 'native/react-native/ios/*.mm'
  s.private_header_files = 'native/**/*.h'
  s.vendored_frameworks = 'native/MaplibreNativeC.xcframework'
  s.pod_target_xcconfig = {
    'GCC_C_LANGUAGE_STANDARD' => 'c11',
    'HEADER_SEARCH_PATHS' => '"$(PODS_TARGET_SRCROOT)/native/react-native" "$(PODS_TARGET_SRCROOT)/native/include" "$(PODS_TARGET_SRCROOT)/native/node-api-headers"',
    'OTHER_CFLAGS' => '$(inherited) -include "$(PODS_TARGET_SRCROOT)/native/react-native/symbols.h"',
  }
  install_modules_dependencies(s)
end
