package org.maplibre.nativeffi.gradle

enum class AndroidTarget(
  val cargoTarget: String,
  val targetPlatform: String,
  val cmakeArchitecture: String,
  val ndkAbi: String,
  val javaCppPlatform: String,
  val ndkTargetTriple: String,
  val taskSuffix: String,
) {
  ARM64(
    cargoTarget = "aarch64-linux-android",
    targetPlatform = "android-arm64",
    cmakeArchitecture = "arm64",
    ndkAbi = "arm64-v8a",
    javaCppPlatform = "android-arm64",
    ndkTargetTriple = "aarch64-linux-android",
    taskSuffix = "Arm64",
  ),
  X64(
    cargoTarget = "x86_64-linux-android",
    targetPlatform = "android-x64",
    cmakeArchitecture = "x64",
    ndkAbi = "x86_64",
    javaCppPlatform = "android-x86_64",
    ndkTargetTriple = "x86_64-linux-android",
    taskSuffix = "X86_64",
  );

  fun ndkCompilerName(apiLevel: Int): String = "$ndkTargetTriple$apiLevel-clang++"

  fun cmakePreset(backend: String): String =
    "android-$cmakeArchitecture-${if (backend == "opengl") "egl" else "vulkan"}"

  companion object {
    const val DEFAULT_ABIS = "arm64-v8a,x86_64"
    const val DEFAULT_BACKEND = "opengl"

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

    fun parseBackend(value: String): String {
      val backend = value.trim().lowercase()
      require(backend == "opengl" || backend == "vulkan") {
        "Unsupported Android backend '$value'; maplibre.android.backend supports opengl or vulkan"
      }
      return backend
    }
  }
}
