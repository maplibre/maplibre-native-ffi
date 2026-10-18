include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/platform/apple.cmake")

# Apple Metal presets also link the C API as MaplibreNativeC.framework, the
# bundle form that Xcode and Swift Package Manager embed. The
# `package-xcframework` task combines the framework from each Apple Metal
# package into MaplibreNativeC.xcframework.
set(MLN_FFI_APPLE_FRAMEWORK_NAME MaplibreNativeC)

# The Info.plist keys that name the bundle's platform and its minimum OS. A Mac
# Catalyst bundle states its minimum as the macOS release that runs the iOS
# deployment target, which the SDK's version map gives.
function(mln_ffi_apple_framework_platform platform_var minimum_key_var
         minimum_var)
  mln_ffi_apple_is_simulator(simulator)
  mln_ffi_apple_is_maccatalyst(maccatalyst)
  set(minimum "${CMAKE_OSX_DEPLOYMENT_TARGET}")
  set(minimum_key MinimumOSVersion)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(platform MacOSX)
    set(minimum_key LSMinimumSystemVersion)
  elseif(maccatalyst)
    set(platform MacOSX)
    set(minimum_key LSMinimumSystemVersion)
    mln_ffi_apple_sdk_path(sdk_path)
    file(READ "${sdk_path}/SDKSettings.json" sdk_settings)
    string(
      JSON minimum
      ERROR_VARIABLE missing
      GET
        "${sdk_settings}" VersionMap iOSMac_macOS
        "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    if(missing)
      message(
        FATAL_ERROR
          "${sdk_path} maps no macOS release to iOS ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    if(simulator)
      set(platform iPhoneSimulator)
    else()
      set(platform iPhoneOS)
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
    if(simulator)
      set(platform AppleTVSimulator)
    else()
      set(platform AppleTVOS)
    endif()
  else()
    message(FATAL_ERROR "Unsupported Apple platform: ${CMAKE_SYSTEM_NAME}")
  endif()
  set(${platform_var} "${platform}" PARENT_SCOPE)
  set(${minimum_key_var} "${minimum_key}" PARENT_SCOPE)
  set(${minimum_var} "${minimum}" PARENT_SCOPE)
endfunction()

# Whether the framework uses the versioned bundle layout. macOS and Mac Catalyst
# frameworks do, and iOS and tvOS frameworks keep their content at the bundle
# root. CMake applies the same rule when it lays out the bundle.
function(mln_ffi_apple_framework_versioned out_var)
  mln_ffi_apple_is_maccatalyst(maccatalyst)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR maccatalyst)
    set(${out_var} TRUE PARENT_SCOPE)
  else()
    set(${out_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(mln_ffi_add_c_api_framework target implementation_target)
  add_library(${target} SHARED)
  mln_ffi_configure_shared_c_api_wrapper(${target} ${implementation_target})

  # CMake's default template leaves out the platform and minimum OS keys that
  # App Store validation reads from an embedded framework. The version is the
  # project's until the publish step relabels it with the release version.
  mln_ffi_apple_framework_platform(
    MLN_FFI_APPLE_FRAMEWORK_PLATFORM MLN_FFI_APPLE_FRAMEWORK_MINIMUM_KEY
    MLN_FFI_APPLE_FRAMEWORK_MINIMUM)
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
  mln_ffi_apple_framework_versioned(versioned)
  if(versioned)
    set(content "${framework}/Versions/A")
    set(resources "${content}/Resources")
  else()
    set(content "${framework}")
    set(resources "${content}")
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
  if(versioned)
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
