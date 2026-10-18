include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/platform/apple.cmake")

# Apple Metal presets also link the C API as MaplibreNativeC.framework, the
# bundle form that Xcode and Swift Package Manager embed. The
# `package-xcframework` task combines the framework from each Apple Metal
# package into MaplibreNativeC.xcframework.
set(MLN_FFI_APPLE_FRAMEWORK_NAME MaplibreNativeC)

# Whether the framework targets iOS or tvOS rather than macOS or Mac Catalyst.
# CMake gives these frameworks a flat bundle, and the macOS family a versioned
# one.
function(mln_ffi_apple_framework_embedded out_var)
  mln_ffi_apple_is_maccatalyst(maccatalyst)
  if(CMAKE_SYSTEM_NAME MATCHES "^(iOS|tvOS)$" AND NOT maccatalyst)
    set(${out_var} TRUE PARENT_SCOPE)
  else()
    set(${out_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(mln_ffi_add_c_api_framework target implementation_target)
  add_library(${target} SHARED)
  mln_ffi_configure_shared_c_api_wrapper(${target} ${implementation_target})

  # App Store processing reads the supported platform from every embedded
  # framework, and MinimumOSVersion from an iOS or tvOS one. The version is the
  # project's until the publish step relabels it with the release version.
  mln_ffi_apple_framework_embedded(embedded)
  mln_ffi_apple_is_simulator(simulator)
  if(NOT embedded)
    set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM MacOSX)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM iPhoneOS)
    if(simulator)
      set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM iPhoneSimulator)
    endif()
  else()
    set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM AppleTVOS)
    if(simulator)
      set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM AppleTVSimulator)
    endif()
  endif()
  set(MLN_FFI_APPLE_FRAMEWORK_MINIMUM_OS "")
  if(embedded)
    set(MLN_FFI_APPLE_FRAMEWORK_MINIMUM_OS
        "<key>MinimumOSVersion</key>\n  <string>${CMAKE_OSX_DEPLOYMENT_TARGET}</string>")
  endif()
  set(info_plist
      "${CMAKE_CURRENT_BINARY_DIR}/${MLN_FFI_APPLE_FRAMEWORK_NAME}-Info.plist")
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/apple/Info.plist.in" "${info_plist}"
    @ONLY)

  set_target_properties(
    ${target}
    PROPERTIES
      FRAMEWORK
      TRUE
      FRAMEWORK_VERSION
      A
      OUTPUT_NAME
      "${MLN_FFI_APPLE_FRAMEWORK_NAME}"
      MACOSX_FRAMEWORK_INFO_PLIST
      "${info_plist}")
endfunction()

# CMake links the framework and writes its Info.plist, and the install adds the
# rest of the bundle. The Ninja generator copies MACOSX_PACKAGE_LOCATION content
# only for a target that compiles its own sources, and this target links the
# shared objects, so headers and resources go in here instead. A consumer that
# takes the framework through Swift Package Manager receives the bundle alone,
# so the bundle carries the prefix's license notices too.
function(mln_ffi_install_c_api_framework target framework_target component)
  install(
    TARGETS ${framework_target}
    FRAMEWORK DESTINATION Frameworks COMPONENT "${component}")

  set(framework "Frameworks/${MLN_FFI_APPLE_FRAMEWORK_NAME}.framework")
  mln_ffi_apple_framework_embedded(embedded)
  if(embedded)
    set(content "${framework}")
    set(resources "${content}")
  else()
    set(content "${framework}/Versions/A")
    set(resources "${content}/Resources")
  endif()

  install(
    FILES "${PROJECT_SOURCE_DIR}/include/maplibre_native_c.h"
    DESTINATION "${content}/Headers"
    COMPONENT "${component}")
  install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/maplibre_native_c"
    DESTINATION "${content}/Headers"
    COMPONENT "${component}"
    FILES_MATCHING
    PATTERN "*.h")
  install(
    FILES "${MLN_FFI_SOURCE_DIR}/include/mln/plugin/plugin_api.h"
    DESTINATION "${content}/Headers/mln/plugin"
    COMPONENT "${component}")
  install(
    FILES "${PROJECT_SOURCE_DIR}/cmake/apple/module.modulemap"
    DESTINATION "${content}/Modules"
    COMPONENT "${component}")
  # The core makes the same required-reason API calls under every Apple SDK, so
  # upstream's iOS SDK manifest describes this library too.
  install(
    FILES "${MLN_FFI_SOURCE_DIR}/platform/ios/PrivacyInfo.xcprivacy"
    DESTINATION "${resources}"
    COMPONENT "${component}")
  install(
    FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${resources}"
    COMPONENT "${component}")
  mln_ffi_install_licenses(${target} "${component}" "${resources}/licenses")

  # CMake links the binary and Resources at the root of a versioned bundle,
  # but not the directories that the install adds.
  if(NOT embedded)
    foreach(directory Headers Modules)
      install(
        CODE
        "file(CREATE_LINK \"Versions/Current/${directory}\"
          \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${framework}/${directory}\"
          SYMBOLIC)"
        COMPONENT "${component}")
    endforeach()
  endif()
endfunction()
