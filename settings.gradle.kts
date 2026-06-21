pluginManagement {
  repositories {
    google()
    mavenCentral()
    gradlePluginPortal()
  }
}

rootProject.name = "maplibre-native-ffi"

include(":bindings:kotlin")

if (androidSdkDir() != null) {
  include(":examples:android-map")
}

include(":examples:lwjgl-map")

fun androidSdkDir(): File? {
  val sdkDir =
    sequenceOf(
        localAndroidSdkDir(),
        System.getenv("ANDROID_HOME"),
        System.getenv("ANDROID_SDK_ROOT"),
      )
      .filterNotNull()
      .firstOrNull { File(it).isDirectory }

  return sdkDir?.let(::File)
}

fun localAndroidSdkDir(): String? {
  val localProperties = File(rootDir, "local.properties")
  if (!localProperties.isFile) {
    return null
  }

  return localProperties.inputStream().use { input ->
    java.util.Properties().apply { load(input) }.getProperty("sdk.dir")
  }
}
