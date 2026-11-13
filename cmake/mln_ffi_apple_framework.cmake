include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/platform/apple.cmake")

# Apple Metal presets also install the C API as MaplibreNativeC.framework,
# which `package-xcframework` combines into MaplibreNativeC.xcframework. The
# framework's binary is the shared library under the framework's name and
# install name. Swift Package Manager consumers get only the bundle, so it
# carries the headers, the license notices, and the SBOM too.
set(MLN_FFI_APPLE_FRAMEWORK_NAME MaplibreNativeC)

function(mln_ffi_install_c_api_framework target shared component)
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
        "<key>MinimumOSVersion</key>\n  <string>${CMAKE_OSX_DEPLOYMENT_TARGET}</string>"
    )
  endif()
  set(info_plist
      "${CMAKE_CURRENT_BINARY_DIR}/${MLN_FFI_APPLE_FRAMEWORK_NAME}-Info.plist")
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/apple/Info.plist.in" "${info_plist}"
    @ONLY)

  # An embedded Apple framework is flat, and a macOS or Mac Catalyst one is
  # versioned.
  set(framework "Frameworks/${MLN_FFI_APPLE_FRAMEWORK_NAME}.framework")
  if(embedded)
    set(content "${framework}")
    set(resources "${content}")
    set(install_name
        "@rpath/${MLN_FFI_APPLE_FRAMEWORK_NAME}.framework/${MLN_FFI_APPLE_FRAMEWORK_NAME}"
    )
  else()
    set(content "${framework}/Versions/A")
    set(resources "${content}/Resources")
    set(install_name
        "@rpath/${MLN_FFI_APPLE_FRAMEWORK_NAME}.framework/Versions/A/${MLN_FFI_APPLE_FRAMEWORK_NAME}"
    )
  endif()

  # The framework's binary is the shared library under another install name,
  # so it is a copy that the build renames and signs again.
  set(binary
      "${CMAKE_CURRENT_BINARY_DIR}/framework/${MLN_FFI_APPLE_FRAMEWORK_NAME}")
  add_custom_command(
    OUTPUT "${binary}"
    COMMAND "${CMAKE_COMMAND}" -E copy "${shared}" "${binary}"
    COMMAND install_name_tool -id "${install_name}" "${binary}"
    COMMAND codesign --force --sign - "${binary}"
    DEPENDS "${shared}"
    VERBATIM)
  add_custom_target(${target}_framework ALL DEPENDS "${binary}")
  install(
    PROGRAMS "${binary}"
    DESTINATION "${content}"
    COMPONENT "${component}")
  mln_ffi_install_apple_dsym(
    "${binary}" "${content}/${MLN_FFI_APPLE_FRAMEWORK_NAME}" "${component}")
  install(
    FILES "${info_plist}"
    DESTINATION "${resources}"
    RENAME Info.plist
    COMPONENT "${component}")

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
    FILES "${PROJECT_SOURCE_DIR}/include/mln/plugin/plugin_api.h"
    DESTINATION "${content}/Headers/mln/plugin"
    COMPONENT "${component}")
  install(
    FILES "${PROJECT_SOURCE_DIR}/cmake/apple/module.modulemap"
    DESTINATION "${content}/Modules"
    COMPONENT "${component}")
  # MapLibre Native's iOS privacy manifest, which covers every Apple SDK.
  install(
    FILES "${PROJECT_SOURCE_DIR}/cmake/apple/PrivacyInfo.xcprivacy"
    DESTINATION "${resources}"
    COMPONENT "${component}")
  install(
    FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION "${resources}"
    COMPONENT "${component}")
  get_target_property(components ${target} MLN_FFI_COMPONENT_DIRECTORY)
  install(
    DIRECTORY "${components}/licenses/"
    DESTINATION "${resources}/licenses"
    COMPONENT "${component}")
  install(
    FILES "${components}/sbom.cdx.json"
    DESTINATION "${resources}"
    COMPONENT "${component}")

  # A versioned bundle links its binary and directories at its root.
  if(NOT embedded)
    install(
      CODE
      "file(CREATE_LINK \"A\"
        \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${framework}/Versions/Current\"
        SYMBOLIC)"
      COMPONENT "${component}")
    foreach(entry ${MLN_FFI_APPLE_FRAMEWORK_NAME} Headers Modules Resources)
      install(
        CODE
        "file(CREATE_LINK \"Versions/Current/${entry}\"
          \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${framework}/${entry}\"
          SYMBOLIC)"
        COMPONENT "${component}")
    endforeach()
  endif()
endfunction()
