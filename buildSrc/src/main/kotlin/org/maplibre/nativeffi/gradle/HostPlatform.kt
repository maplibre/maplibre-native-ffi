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
        isMac -> "natives-macos"
        isLinux && isArm64 -> "natives-linux-arm64"
        isLinux -> "natives-linux"
        isWindows -> "natives-windows"
        else -> throw IllegalStateException("Unsupported LWJGL native platform: $osName/$arch")
      }

  val jextractDistribution: JextractDistribution
    get() =
      when {
        isMac && isArm64 ->
          JextractDistribution(
            "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_macos-aarch64_bin.tar.gz",
            "6783d2ba7f686ee636b9542525ee06b7bd096dfca294538613b877a4b5a057da",
          )
        isMac ->
          JextractDistribution(
            "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_macos-x64_bin.tar.gz",
            "62fd0453349b8eb48f083d2fb9c5f2ab255f894eaa8c658221366f363c7e91b9",
          )
        isLinux && isArm64 ->
          JextractDistribution(
            "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_linux-aarch64_bin.tar.gz",
            "75a199a05e5edade798600a175f8897e711330338f7d8d2da5fff18d707d665e",
          )
        isLinux ->
          JextractDistribution(
            "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_linux-x64_bin.tar.gz",
            "d826d366b5db8edbed9cfef3779e45e43ba496ca2166b8f70cdaf81ee90c0b1e",
          )
        isWindows ->
          JextractDistribution(
            "https://download.java.net/java/early_access/jextract/25/1/openjdk-25-jextract+1-1_windows-x64_bin.tar.gz",
            "22a853168512d5909c4dfccb1b9e8b2bffb1187b0cf98458aef989d41de995ac",
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

  val kotlinNativeTargetPresetName: String?
    get() =
      when {
        isMac && isArm64 -> "macosArm64"
        isMac -> "macosX64"
        isLinux && isArm64 -> "linuxArm64"
        isLinux -> "linuxX64"
        else -> null
      }

  companion object {
    fun current(): HostPlatform =
      HostPlatform(
        System.getProperty("os.name").lowercase(),
        System.getProperty("os.arch").lowercase(),
      )
  }
}
