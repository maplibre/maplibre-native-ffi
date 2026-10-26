include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/platform/apple.cmake")

# Apple Metal presets also build the C API as MaplibreNativeC.framework, which
# `package-xcframework` combines into MaplibreNativeC.xcframework.
set(MLN_FFI_APPLE_FRAMEWORK_NAME MaplibreNativeC)

function(mln_ffi_add_c_api_framework target implementation_target)
  add_library(${target} SHARED)
  mln_ffi_configure_shared_c_api_wrapper(${target} ${implementation_target})

  # App Store processing needs the platform, and MinimumOSVersion on iOS,
  # tvOS, and visionOS. The publish step replaces the project version with the
  # release's.
  mln_ffi_apple_is_embedded(embedded)
  mln_ffi_apple_is_simulator(simulator)
  if(NOT embedded)
    set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM MacOSX)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM iPhoneOS)
    if(simulator)
      set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM iPhoneSimulator)
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
    set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM XROS)
    if(simulator)
      set(MLN_FFI_APPLE_FRAMEWORK_PLATFORM XRSimulator)
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

# The install adds the bundle's headers and resources, because Ninja copies
# MACOSX_PACKAGE_LOCATION files only for a target that compiles its own sources.
# Swift Package Manager consumers get only the bundle, so it carries the license
# notices too.
function(mln_ffi_install_c_api_framework target framework_target component)
  install(
    TARGETS ${framework_target}
    FRAMEWORK DESTINATION Frameworks COMPONENT "${component}")
  mln_ffi_install_apple_dsym(${framework_target} Frameworks "${component}")

  # CMake gives an embedded Apple framework a flat bundle, and a macOS or Mac
  # Catalyst one a versioned bundle.
  set(framework "Frameworks/${MLN_FFI_APPLE_FRAMEWORK_NAME}.framework")
  mln_ffi_apple_is_embedded(embedded)
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
    PATTERN "*.h"
    PATTERN "binding_support.h" EXCLUDE)
  install(
    FILES "${MLN_FFI_SOURCE_DIR}/include/mln/plugin/plugin_api.h"
    DESTINATION "${content}/Headers/mln/plugin"
    COMPONENT "${component}")
  install(
    FILES "${PROJECT_SOURCE_DIR}/cmake/apple/module.modulemap"
    DESTINATION "${content}/Modules"
    COMPONENT "${component}")
  # Upstream's iOS privacy manifest covers every Apple SDK.
  install(
    FILES "${MLN_FFI_SOURCE_DIR}/platform/ios/PrivacyInfo.xcprivacy"
    DESTINATION "${resources}"
    COMPONENT "${component}")
  install(
    FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${resources}"
    COMPONENT "${component}")
  mln_ffi_install_licenses(${target} "${component}" "${resources}/licenses")

  # CMake links only the binary and Resources at a versioned bundle's root.
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
