package org.maplibre.nativeffi.gradle

class HostPlatform private constructor(osName: String, arch: String) {
  data class JextractDistribution(val url: String, val sha256: String)

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

  val jextractDistribution: JextractDistribution
    get() =
      when {
        isMac && isArm64 ->
          JextractDistribution(
            "https://download.java.net/java/early_access/jextract/25/2/openjdk-25-jextract+2-4_macos-aarch64_bin.tar.gz",
            "3dd1dd1bde059d271739e2cc2290c64f93f85488c86c01e566c0e374eece798f",
          )
        isLinux && isArm64 ->
          JextractDistribution(
            "https://download.java.net/java/early_access/jextract/25/2/openjdk-25-jextract+2-4_linux-aarch64_bin.tar.gz",
            "0e25e6f6efa042f8758eaec65a873887fd2247fcf2e3e22dcfd7e4179fc8b0ae",
          )
        isLinux ->
          JextractDistribution(
            "https://download.java.net/java/early_access/jextract/25/2/openjdk-25-jextract+2-4_linux-x64_bin.tar.gz",
            "d0cc481abc1adb16fb9514e1c5e0bfc08d38c29228bece667fb5054ceaffaa42",
          )
        isWindows ->
          JextractDistribution(
            "https://download.java.net/java/early_access/jextract/25/2/openjdk-25-jextract+2-4_windows-x64_bin.tar.gz",
            "b03533eb6b249a154752b7c7bf93cdb8c147cf2f9699422e615e84b7fb652872",
          )
        else -> throw IllegalStateException("Unsupported jextract platform: $osName/$arch")
      }

  val jextractExecutableFileName: String
    get() = if (isWindows) "jextract.bat" else "jextract"

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
