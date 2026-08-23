package org.maplibre.nativeffi.gradle

enum class AndroidTarget(
  val cargoTarget: String,
  val kotlinNativeTarget: String,
  val targetPlatform: String,
  val cmakeArchitecture: String,
  val ndkAbi: String,
  val javaCppPlatform: String,
  val ndkTargetTriple: String,
  val taskSuffix: String,
) {
  ARM(
    cargoTarget = "armv7-linux-androideabi",
    kotlinNativeTarget = "androidNativeArm32",
    targetPlatform = "android-arm",
    cmakeArchitecture = "arm",
    ndkAbi = "armeabi-v7a",
    javaCppPlatform = "android-arm",
    ndkTargetTriple = "armv7a-linux-androideabi",
    taskSuffix = "Arm32",
  ),
  ARM64(
    cargoTarget = "aarch64-linux-android",
    kotlinNativeTarget = "androidNativeArm64",
    targetPlatform = "android-arm64",
    cmakeArchitecture = "arm64",
    ndkAbi = "arm64-v8a",
    javaCppPlatform = "android-arm64",
    ndkTargetTriple = "aarch64-linux-android",
    taskSuffix = "Arm64",
  ),
  X64(
    cargoTarget = "x86_64-linux-android",
    kotlinNativeTarget = "androidNativeX64",
    targetPlatform = "android-x64",
    cmakeArchitecture = "x64",
    ndkAbi = "x86_64",
    javaCppPlatform = "android-x86_64",
    ndkTargetTriple = "x86_64-linux-android",
    taskSuffix = "X86_64",
  );

  fun ndkCompilerName(apiLevel: Int): String = "$ndkTargetTriple$apiLevel-clang++"

  // Vulkan non-dispatchable handles are 64-bit integers on ARM32, while the C
  // API currently carries backend-native handles in pointer-width fields.
  fun supportsBackend(backend: String): Boolean = this != ARM || backend == "opengl"

  fun cmakePreset(backend: String): String {
    require(supportsBackend(backend)) {
      "Android ABI '$ndkAbi' does not support the $backend backend"
    }
    return "android-$cmakeArchitecture-${if (backend == "opengl") "egl" else "vulkan"}"
  }

  companion object {
    const val DEFAULT_BACKEND = "opengl"

    fun defaultAbis(backend: String): String =
      entries.filter { it.supportsBackend(backend) }.joinToString(",") { it.ndkAbi }

    fun parseAbis(value: String): List<AndroidTarget> {
      val abis = value.split(',').map(String::trim)
      require(abis.isNotEmpty() && abis.none(String::isEmpty)) {
        "maplibre.android.abis must be a comma-separated list of supported ABIs"
      }
      require(abis.distinct().size == abis.size) {
        "maplibre.android.abis contains duplicate ABIs: $value"
      }

      return abis.map { abi ->
        entries.firstOrNull { it.ndkAbi == abi }
          ?: error(
            "Unsupported Android ABI '$abi'; maplibre.android.abis supports " +
              entries.joinToString(",") { it.ndkAbi }
          )
      }
    }

    fun parseAbis(value: String, backend: String): List<AndroidTarget> {
      val targets = parseAbis(value)
      val unsupported = targets.filterNot { it.supportsBackend(backend) }
      require(unsupported.isEmpty()) {
        "Android backend '$backend' does not support ABIs: " +
          unsupported.joinToString(",") { it.ndkAbi }
      }
      return targets
    }

    fun compatibleAbis(value: String, backend: String): List<AndroidTarget> =
      parseAbis(value)
        .filter { it.supportsBackend(backend) }
        .ifEmpty { parseAbis(defaultAbis(backend)) }

    fun parseBackend(value: String): String {
      val backend = value.trim().lowercase()
      require(backend == "opengl" || backend == "vulkan") {
        "Unsupported Android backend '$value'; maplibre.android.backend supports opengl or vulkan"
      }
      return backend
    }
  }
}
