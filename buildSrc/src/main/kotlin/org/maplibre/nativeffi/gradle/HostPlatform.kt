package org.maplibre.nativeffi.gradle

class HostPlatform private constructor(osName: String, arch: String) {
  val osName: String = osName
  val arch: String = arch

  val isMac: Boolean
    get() = osName.contains("mac")

  val isLinux: Boolean
    get() = osName.contains("linux")

  val isWindows: Boolean
    get() = osName.contains("windows")

  val isArm64: Boolean
    get() = arch == "aarch64" || arch == "arm64"

  val maplibreNativeClassifier: String
    get() =
      when {
        isMac && isArm64 -> "natives-macos-arm64"
        isLinux && isArm64 -> "natives-linux-arm64"
        isLinux -> "natives-linux-x64"
        isWindows && isArm64 -> "natives-windows-arm64"
        isWindows -> "natives-windows-x64"
        else -> throw IllegalStateException("Unsupported MapLibre native platform: $osName/$arch")
      }

  /** The native target platform that the JVM on this host loads. */
  val targetPlatform: String
    get() = JVM_TARGET_PLATFORMS.getValue(maplibreNativeClassifier)

  val androidNdkPrebuiltTag: String
    get() =
      when {
        isMac -> "darwin-x86_64"
        isLinux && isArm64 -> "linux-aarch64"
        isLinux -> "linux-x86_64"
        isWindows -> "windows-x86_64"
        else -> throw IllegalStateException("Unsupported Android NDK host: $osName/$arch")
      }

  val executableSuffix: String
    get() = if (isWindows) ".exe" else ""

  val androidNdkCommandSuffix: String
    get() = if (isWindows) ".cmd" else ""

  companion object {
    /** The native target platform of each JVM runtime JAR classifier. */
    val JVM_TARGET_PLATFORMS: Map<String, String> =
      linkedMapOf(
        "natives-linux-x64" to "linux-gnu-x64",
        "natives-linux-arm64" to "linux-gnu-arm64",
        "natives-linux-musl-x64" to "linux-musl-x64",
        "natives-linux-musl-arm64" to "linux-musl-arm64",
        "natives-freebsd-x64" to "freebsd-x64",
        "natives-macos-arm64" to "macos-arm64",
        "natives-windows-x64" to "windows-x64",
        "natives-windows-arm64" to "windows-arm64",
      )

    fun current(): HostPlatform =
      HostPlatform(
        System.getProperty("os.name").lowercase(),
        System.getProperty("os.arch").lowercase(),
      )
  }
}
