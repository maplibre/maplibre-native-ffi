package org.maplibre.nativeffi.gradle

enum class AndroidTarget(
  val cargoTarget: String,
  val ndkAbi: String,
  val javaCppPlatform: String,
  val ndkCompilerTriple: String,
) {
  ARM64(
    cargoTarget = "aarch64-linux-android",
    ndkAbi = "arm64-v8a",
    javaCppPlatform = "android-arm64",
    ndkCompilerTriple = "aarch64-linux-android24",
  ),
  X64(
    cargoTarget = "x86_64-linux-android",
    ndkAbi = "x86_64",
    javaCppPlatform = "android-x86_64",
    ndkCompilerTriple = "x86_64-linux-android24",
  );

  companion object {
    fun current(): AndroidTarget = fromCargoTarget(requiredCargoTarget())

    fun isAndroidCargoTarget(cargoTarget: String): Boolean =
      entries.any { it.cargoTarget == cargoTarget }

    /** True when [CARGO_BUILD_TARGET] is set to a supported Android triple. */
    fun hasAndroidCargoTarget(): Boolean {
      val cargoTarget = System.getenv("CARGO_BUILD_TARGET") ?: return false
      return isAndroidCargoTarget(cargoTarget)
    }

    /**
     * Resolves the Android rustls Maven layout when Android repos are configured off-Android envs.
     * [CARGO_BUILD_TARGET] may point at the host desktop triple during Linux native builds; only
     * honor it when it matches a supported Android cargo target.
     */
    fun cargoTargetForRustlsMetadata(): String {
      val cargoTarget = System.getenv("CARGO_BUILD_TARGET")
      if (cargoTarget != null) {
        entries.firstOrNull { it.cargoTarget == cargoTarget }?.let { return cargoTarget }
      }
      return ARM64.cargoTarget
    }

    /**
     * Resolves ndk ABI filters during Gradle configuration when [CARGO_BUILD_TARGET] may be unset.
     */
    fun ndkAbiForGradleConfiguration(): String =
      fromCargoTarget(cargoTargetForRustlsMetadata()).ndkAbi

    fun fromCargoTarget(cargoTarget: String): AndroidTarget =
      entries.firstOrNull { it.cargoTarget == cargoTarget }
        ?: error(
          "Unsupported Android cargo target: $cargoTarget " +
            "(expected ${entries.joinToString(" or ") { it.cargoTarget }})"
        )

    private fun requiredCargoTarget(): String =
      System.getenv("CARGO_BUILD_TARGET")
        ?: error("CARGO_BUILD_TARGET must be set for Android Gradle builds")
  }
}
