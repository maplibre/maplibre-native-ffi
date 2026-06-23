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
    /** Resolves when [CARGO_BUILD_TARGET] names a supported Android triple. */
    fun fromEnv(): AndroidTarget? {
      val cargoTarget = System.getenv("CARGO_BUILD_TARGET") ?: return null
      return entries.firstOrNull { it.cargoTarget == cargoTarget }
    }

    fun current(): AndroidTarget =
      fromEnv()
        ?: error(
          "CARGO_BUILD_TARGET must be set to a supported Android triple " +
            "(expected ${entries.joinToString(" or ") { it.cargoTarget }})"
        )

    /**
     * Cargo `--filter-platform` for rustls-platform-verifier-android Maven metadata. The published
     * artifact is ABI-agnostic; [ARM64] is the fallback when the env names a host desktop triple
     * during Linux native builds.
     */
    fun rustlsMetadataCargoTarget(): String = fromEnv()?.cargoTarget ?: ARM64.cargoTarget

    /**
     * NDK ABI filter for Android Gradle when [CARGO_BUILD_TARGET] is unset or names a host triple.
     */
    fun ndkAbiForGradleConfiguration(): String = fromEnv()?.ndkAbi ?: ARM64.ndkAbi
  }
}
