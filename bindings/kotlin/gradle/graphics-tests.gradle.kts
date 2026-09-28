import org.gradle.api.file.Directory
import org.gradle.api.provider.Provider
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.catalogVersionInt
import org.maplibre.nativeffi.gradle.requiredEnvironmentVariable

val host = HostPlatform.current()
val backend =
  AndroidTarget.parseBackend(
    providers.gradleProperty("maplibre.android.backend").getOrElse(AndroidTarget.DEFAULT_BACKEND)
  )
val targets =
  AndroidTarget.parseAbis(
    providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.defaultAbis(backend)),
    backend,
  )
@Suppress("UNCHECKED_CAST")
val sdk = extensions.extraProperties["maplibreAndroidSdkDirectory"] as Provider<Directory>
val ndkVersion = requiredEnvironmentVariable("MLN_FFI_ANDROID_NDK_VERSION")
val apiLevel = catalogVersionInt("android-minSdk")
val fixture = rootProject.file("tests/graphics")
val vulkanHeaders = rootProject.file("third_party/maplibre-native/vendor/Vulkan-Headers/include")
val jni = file("src/androidDeviceTest/cpp/graphics_jni.c")
val output = layout.buildDirectory.dir("generated/jniLibs/graphicsTest")

val builds = targets.map { target ->
  val library = output.map {
    it.file("${target.cargoTarget}/${target.ndkAbi}/libbinding_test_graphics.so")
  }
  val compiler = sdk.map {
    it.file(
      "ndk/$ndkVersion/toolchains/llvm/prebuilt/${host.androidNdkPrebuiltTag}/bin/${target.ndkCompilerName(apiLevel).replace("clang++", "clang")}${host.androidNdkCommandSuffix}"
    )
  }
  tasks.register<Exec>("buildAndroidTestGraphics${target.taskSuffix}") {
    inputs.dir(fixture)
    inputs.file(jni)
    inputs.dir(vulkanHeaders)
    outputs.file(library)
    doFirst { library.get().asFile.parentFile.mkdirs() }
    commandLine(
      compiler.get().asFile,
      "-std=c11",
      "-shared",
      "-fPIC",
      "-I${fixture.resolve("include")}",
      "-I$vulkanHeaders",
      fixture.resolve("graphics.c"),
      jni,
      "-ldl",
      "-o",
      library.get().asFile,
    )
  }
}

tasks
  .matching { it.name == "mergeAndroidDeviceTestJniLibFolders" }
  .configureEach { dependsOn(builds) }
