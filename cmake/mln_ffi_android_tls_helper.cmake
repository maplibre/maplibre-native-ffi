# Builds the dex that carries the Rustls platform verifier's Kotlin helper. The
# Android library embeds it and loads it in a class loader of its own, so a host
# packages no Java code and keeps no R8 rule for it.
function(mln_ffi_android_tls_helper_dex out_var)
  find_program(MLN_FFI_JAVA_EXECUTABLE java REQUIRED)
  # find_program tries only .com and .exe on Windows, where kotlinc is a batch
  # file.
  if(CMAKE_HOST_WIN32)
    find_program(MLN_FFI_KOTLINC_EXECUTABLE kotlinc.bat REQUIRED)
  else()
    find_program(MLN_FFI_KOTLINC_EXECUTABLE kotlinc REQUIRED)
  endif()

  # The commands name the tools by their install paths, and kotlinc finds its
  # JVM through JAVA_HOME, so a later `cmake --build` from an environment
  # without either on its PATH still rebuilds the dex.
  file(REAL_PATH "${MLN_FFI_JAVA_EXECUTABLE}" java_executable)
  cmake_path(GET java_executable PARENT_PATH java_home)
  cmake_path(GET java_home PARENT_PATH java_home)
  file(REAL_PATH "${MLN_FFI_KOTLINC_EXECUTABLE}" kotlinc_executable)
  cmake_path(GET kotlinc_executable PARENT_PATH kotlin_home)
  cmake_path(GET kotlin_home PARENT_PATH kotlin_home)
  set(kotlin_stdlib "${kotlin_home}/lib/kotlin-stdlib.jar")

  file(
    STRINGS "${PROJECT_SOURCE_DIR}/gradle/libs.versions.toml" compile_sdk
    REGEX "^android-compileSdk = ")
  string(REGEX REPLACE "^android-compileSdk = \"([0-9]+)\"$" "\\1" compile_sdk
         "${compile_sdk}")
  set(android_jar
      "$ENV{ANDROID_HOME}/platforms/android-${compile_sdk}/android.jar")
  set(r8_jar
      "$ENV{ANDROID_HOME}/build-tools/$ENV{MLN_FFI_ANDROID_BUILD_TOOLS_VERSION}/lib/d8.jar")
  foreach(input IN ITEMS "${kotlin_stdlib}" "${android_jar}" "${r8_jar}")
    if(NOT EXISTS "${input}")
      message(
        FATAL_ERROR
          "The Android TLS helper needs ${input}; run `mise run android-sdk-packages --native-only`")
    endif()
  endforeach()

  set(verifier_source
      "${PROJECT_SOURCE_DIR}/build/dependencies/rustls-platform-verifier/android/rustls-platform-verifier/src/main/java/org/rustls/platformverifier/CertificateVerifier.kt")
  set(helper_dir "${PROJECT_SOURCE_DIR}/src/platform/rust/tls-helper")
  set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/tls-helper")
  set(classes "${output_dir}/classes.jar")
  set(dex "${output_dir}/dex/classes.dex")

  # R8 shrinks the helper together with the parts of the Kotlin standard library
  # that it uses, which leaves one dex that needs nothing from the host.
  add_custom_command(
    OUTPUT "${dex}"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${output_dir}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_dir}/dex"
    COMMAND
      "${CMAKE_COMMAND}"
      -E
      env
      "JAVA_HOME=${java_home}"
      "${kotlinc_executable}"
      -jvm-target
      11
      -nowarn
      -classpath
      "${android_jar}"
      -d
      "${classes}"
      "${verifier_source}"
      "${helper_dir}/BuildConfig.kt"
    COMMAND
      "${java_executable}"
      -cp
      "${r8_jar}"
      com.android.tools.r8.R8
      --release
      --min-api
      "${ANDROID_PLATFORM_LEVEL}"
      --lib
      "${android_jar}"
      --pg-conf
      "${helper_dir}/rules.pro"
      --output
      "${output_dir}/dex"
      "${classes}"
      "${kotlin_stdlib}"
    DEPENDS
      "${verifier_source}" "${helper_dir}/BuildConfig.kt"
      "${helper_dir}/rules.pro"
    VERBATIM)
  set(${out_var} "${dex}" PARENT_SCOPE)
endfunction()

# Registers the notices for what the dex carries: the Kotlin standard library,
# under the Kotlin distribution's license and NOTICE, and our modifications to
# the verifier's helper. rust.md already holds the verifier's own license.
function(mln_ffi_android_tls_helper_licenses target)
  file(REAL_PATH "${MLN_FFI_KOTLINC_EXECUTABLE}" kotlinc_executable)
  cmake_path(GET kotlinc_executable PARENT_PATH kotlin_home)
  cmake_path(GET kotlin_home PARENT_PATH kotlin_home)
  mln_ffi_add_license(${target} "${kotlin_home}/license/LICENSE.txt"
                      "kotlin.txt")
  mln_ffi_add_license(${target} "${kotlin_home}/license/NOTICE.txt"
                      "kotlin-notice.txt")
  mln_ffi_add_license(
    ${target} "${PROJECT_SOURCE_DIR}/patches/rustls-platform-verifier/NOTICE"
    "rustls-platform-verifier-notice.txt")
endfunction()
