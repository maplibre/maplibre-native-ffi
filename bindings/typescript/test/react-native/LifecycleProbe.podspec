Pod::Spec.new do |s|
  s.name = 'LifecycleProbe'
  s.version = '0.0.0'
  s.summary = 'React Native reload teardown spike for MapLibre Native FFI'
  s.homepage = 'https://github.com/maplibre/maplibre-native-ffi/issues/1316'
  s.license = 'BSD-2-Clause'
  s.author = 'MapLibre'
  s.source = { :git => 'https://github.com/maplibre/maplibre-native-ffi.git' }
  s.platforms = { :ios => '15.1' }
  s.source_files = 'ios/*.mm', 'native/Probe.{h,cpp}', 'native/shim_test.c', 'native/test_{api,graphics}.c', 'support/native_probe.c', 'graphics/graphics.c'
  s.header_mappings_dir = 'native'
  s.dependency 'MaplibreNativeFfi'
  s.pod_target_xcconfig = {
    'CLANG_C_LANGUAGE_STANDARD' => 'c11',
    'HEADER_SEARCH_PATHS' => '"$(PODS_TARGET_SRCROOT)/support" "$(PODS_TARGET_SRCROOT)/graphics/include" "$(PODS_TARGET_SRCROOT)/vulkan" "$(PODS_ROOT)/../../node_modules/@maplibre/native-ffi/native/react-native" "$(PODS_ROOT)/../../node_modules/@maplibre/native-ffi/native/node-api-headers"',
    'OTHER_CFLAGS' => '$(inherited) -include "$(PODS_ROOT)/../../node_modules/@maplibre/native-ffi/native/react-native/symbols.h"',
  }
  install_modules_dependencies(s)
end
