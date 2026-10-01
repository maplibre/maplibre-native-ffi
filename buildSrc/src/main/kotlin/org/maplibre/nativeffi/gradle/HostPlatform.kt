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

  val lwjglNativeClassifier: String
    get() =
      when {
        isMac && isArm64 -> "natives-macos-arm64"
        isLinux && isArm64 -> "natives-linux-arm64"
        isLinux -> "natives-linux"
        isWindows && isArm64 -> "natives-windows-arm64"
        isWindows -> "natives-windows"
        else -> throw IllegalStateException("Unsupported LWJGL native platform: $osName/$arch")
      }

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
    fun current(): HostPlatform =
      HostPlatform(
        System.getProperty("os.name").lowercase(),
        System.getProperty("os.arch").lowercase(),
      )
  }
}
