package org.maplibre.nativeffi.gradle

data class AndroidTarget(
  val ndkAbi: String,
  val javaCppPlatform: String,
  val ndkCompilerTriple: String,
  val cargoTarget: String,
) {
  companion object {
    fun current(): AndroidTarget = fromCargoTarget(requiredCargoTarget())

    fun fromCargoTarget(cargoTarget: String): AndroidTarget =
      when (cargoTarget) {
        "aarch64-linux-android" ->
          AndroidTarget(
            ndkAbi = "arm64-v8a",
            javaCppPlatform = "android-arm64",
            ndkCompilerTriple = "aarch64-linux-android24",
            cargoTarget = cargoTarget,
          )
        "x86_64-linux-android" ->
          AndroidTarget(
            ndkAbi = "x86_64",
            javaCppPlatform = "android-x86_64",
            ndkCompilerTriple = "x86_64-linux-android24",
            cargoTarget = cargoTarget,
          )
        else ->
          error(
            "Unsupported Android cargo target: $cargoTarget " +
              "(expected aarch64-linux-android or x86_64-linux-android)"
          )
      }

    private fun requiredCargoTarget(): String =
      System.getenv("CARGO_BUILD_TARGET")
        ?: error("CARGO_BUILD_TARGET must be set for Android Gradle builds")
  }
}
