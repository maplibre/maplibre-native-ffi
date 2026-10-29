package org.maplibre.nativeffi.gradle

enum class AndroidTarget(
  val cargoTarget: String,
  val kotlinNativeTarget: String,
  val targetPlatform: String,
  val ndkAbi: String,
  val ndkTargetTriple: String,
  val taskSuffix: String,
) {
  ARM(
    cargoTarget = "armv7-linux-androideabi",
    kotlinNativeTarget = "androidNativeArm32",
    targetPlatform = "android-arm",
    ndkAbi = "armeabi-v7a",
    ndkTargetTriple = "armv7a-linux-androideabi",
    taskSuffix = "Arm32",
  ),
  ARM64(
    cargoTarget = "aarch64-linux-android",
    kotlinNativeTarget = "androidNativeArm64",
    targetPlatform = "android-arm64",
    ndkAbi = "arm64-v8a",
    ndkTargetTriple = "aarch64-linux-android",
    taskSuffix = "Arm64",
  ),
  X86(
    cargoTarget = "i686-linux-android",
    kotlinNativeTarget = "androidNativeX86",
    targetPlatform = "android-x86",
    ndkAbi = "x86",
    ndkTargetTriple = "i686-linux-android",
    taskSuffix = "X86",
  ),
  X64(
    cargoTarget = "x86_64-linux-android",
    kotlinNativeTarget = "androidNativeX64",
    targetPlatform = "android-x64",
    ndkAbi = "x86_64",
    ndkTargetTriple = "x86_64-linux-android",
    taskSuffix = "X86_64",
  );

  /** The NDK C compiler driver for this ABI at [apiLevel]. */
  fun ndkCompilerName(apiLevel: Int): String = "$ndkTargetTriple$apiLevel-clang"

  companion object {
    const val DEFAULT_BACKEND = "opengl"

    val DEFAULT_ABIS: String = entries.joinToString(",") { it.ndkAbi }

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
              entries.joinToString(",") { it.ndkAbi },
          )
      }
    }

    fun parseBackend(value: String): String {
      val backend = value.trim().lowercase()
      require(backend == "opengl" || backend == "vulkan") {
        "Unsupported Android backend '$value'; maplibre.android.backend supports opengl or vulkan"
      }
      return backend
    }
  }
}
