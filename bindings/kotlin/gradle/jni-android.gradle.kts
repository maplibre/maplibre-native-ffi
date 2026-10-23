import org.gradle.api.file.Directory
import org.gradle.api.provider.Provider
import org.gradle.api.tasks.Exec
import org.gradle.api.tasks.Sync
import org.gradle.api.tasks.bundling.Zip
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.catalogVersionInt
import org.maplibre.nativeffi.gradle.localBuild
import org.maplibre.nativeffi.gradle.localBuildPrefix
import org.maplibre.nativeffi.gradle.nativePreset
import org.maplibre.nativeffi.gradle.registerLocalBuildVerification
import org.maplibre.nativeffi.gradle.requiredEnvironmentVariable

val hostPlatform = HostPlatform.current()
val androidApiLevel = catalogVersionInt("android-minSdk")
val androidNdkVersion = requiredEnvironmentVariable("MLN_FFI_ANDROID_NDK_VERSION")
val androidBackend =
  AndroidTarget.parseBackend(
    providers.gradleProperty("maplibre.android.backend").getOrElse(AndroidTarget.DEFAULT_BACKEND),
  )
val androidTargets =
  AndroidTarget.parseAbis(
    providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS),
  )
@Suppress("UNCHECKED_CAST")
val androidSdkDirectory =
  extensions.extraProperties["maplibreAndroidSdkDirectory"] as Provider<Directory>
val androidNdkPrebuilt = androidSdkDirectory.map {
  it.dir("ndk/$androidNdkVersion/toolchains/llvm/prebuilt/${hostPlatform.androidNdkPrebuiltTag}")
}
// Resolve the NOTICE from the SDK Gradle selected. ANDROID_HOME can point at a different SDK.
val androidNdkNotice = androidSdkDirectory.map { it.file("ndk/$androidNdkVersion/NOTICE") }
val repositoryRoot = rootProject.layout.projectDirectory.asFile

val androidSdkPackages =
  tasks.register<Exec>("androidSdkPackages") {
    group = "build setup"
    description = "Installs the pinned packages into the SDK selected by Gradle."
    workingDir(repositoryRoot)
    commandLine(
      "mise",
      "run",
      "//:android-sdk-packages",
      "--sdk-root",
      androidSdkDirectory.get().asFile.absolutePath,
    )
  }

val checkedInCHeaders = rootProject.layout.projectDirectory.dir("include")
// plugin.h includes upstream's mln/plugin/plugin_api.h from the submodule.
val upstreamPluginHeaders =
  rootProject.layout.projectDirectory.dir("third_party/maplibre-native/include")
// The JNI shim: the hand-written runtime and the declarations tools/bindgen generates.
val jniSources = layout.projectDirectory.dir("src/androidMain/jni")
// The C API library ships in the backend runtime AARs; the JNI shim is private to this
// binding and ships in this module's AAR.
val packagedAndroidRuntimeLibs = layout.buildDirectory.dir("generated/jniLibs/runtime")
@Suppress("UNCHECKED_CAST")
val packagedAndroidBindingLibs =
  extensions.extraProperties["maplibreAndroidBindingLibsDirectory"] as Provider<Directory>
val packageAndroidRuntimeLibraries =
  tasks.register("packageAndroidRuntimeLibraries") {
    group = "build"
    description = "Packages the MapLibre C library for the Android runtime AARs."
  }

val packageAndroidBindingLibraries =
  tasks.register("packageAndroidBindingLibraries") {
    group = "build"
    description = "Packages the JNI shim for the Android binding AAR."
  }

val packageAndroidNativeLibraries =
  tasks.register("packageAndroidNativeLibraries") {
    group = "build"
    description = "Packages every Android native library this repository publishes."
    dependsOn(packageAndroidRuntimeLibraries, packageAndroidBindingLibraries)
  }

// The shim is built with the NDK and links its runtime objects, so this AAR carries the
// NDK's notice.
tasks.withType<Zip>().configureEach {
  if (name == "bundleAndroidMainAar") {
    dependsOn(androidSdkPackages)
    from(androidNdkNotice) { into("META-INF/licenses/android-ndk") }
  }
}

androidTargets.forEach { target ->
  val verifyLocalBuild =
    registerLocalBuildVerification(
      "verifyAndroid${target.taskSuffix}LocalBuild",
      target.targetPlatform,
      androidBackend,
    )
  packageAndroidRuntimeLibraries.configure { dependsOn(verifyLocalBuild) }
  packageAndroidBindingLibraries.configure { dependsOn(verifyLocalBuild) }
  val installDir =
    localBuild?.let { localBuildPrefix(it, nativePreset(target.targetPlatform, androidBackend)) }
      ?: return@forEach
  val targetRoot = layout.buildDirectory.dir("android-native/$androidBackend/${target.cargoTarget}")
  val runtimePackageDir = packagedAndroidRuntimeLibs.map {
    it.dir("$androidBackend/${target.cargoTarget}")
  }
  val bindingPackageDir = packagedAndroidBindingLibs.map { it.dir(target.cargoTarget) }
  val nativeLibrary = installDir.resolve("lib/libmaplibre-native-c.so")
  val jniLibrary = targetRoot.map { it.file("jni/libmaplibre-native-ffi-jni.so") }
  val ndkCompiler = androidNdkPrebuilt.map {
    it.file("bin/${target.ndkCompilerName(androidApiLevel)}${hostPlatform.androidNdkCommandSuffix}")
  }

  val buildJni =
    tasks.register<Exec>("buildAndroidJni${target.taskSuffix}") {
      group = "build"
      description = "Builds the Android ${target.ndkAbi} JNI shim."
      dependsOn(androidSdkPackages, verifyLocalBuild)
      inputs.dir(jniSources).withPropertyName("jniSources")
      inputs.dir(checkedInCHeaders).withPropertyName("maplibreNativeCHeaders")
      inputs.file(nativeLibrary).withPropertyName("maplibreNativeCLibrary")
      inputs.file(ndkCompiler).withPropertyName("androidNdkCompiler")
      outputs.file(jniLibrary)
      doFirst { jniLibrary.get().asFile.parentFile.mkdirs() }
      executable(ndkCompiler.get().asFile)
      args(
        "-std=c11",
        "-O2",
        "-shared",
        "-fPIC",
        "-fvisibility=hidden",
        "-Wall",
        "-Werror",
        "-I${checkedInCHeaders.asFile}",
        "-I${upstreamPluginHeaders.asFile}",
        "-I${jniSources.asFile}",
        jniSources.file("mln_jni.c").asFile,
        jniSources.file("mln_jni_generated.c").asFile,
        "-L${installDir.resolve("lib")}",
        "-lmaplibre-native-c",
        "-llog",
        // Android 15 devices may use 16 KB pages.
        "-Wl,-z,max-page-size=16384",
        "-o",
        jniLibrary.get().asFile,
      )
    }

  val packageRuntimeTarget =
    tasks.register<Sync>("packageAndroidRuntimeLibraries${target.taskSuffix}") {
      description = "Packages the Android ${target.ndkAbi} MapLibre C library."
      dependsOn(verifyLocalBuild)
      into(runtimePackageDir)
      from(nativeLibrary) { into(target.ndkAbi) }
    }

  val packageBindingTarget =
    tasks.register<Sync>("packageAndroidBindingLibraries${target.taskSuffix}") {
      description = "Packages the Android ${target.ndkAbi} JNI shim."
      dependsOn(buildJni)
      into(bindingPackageDir)
      from(jniLibrary) { into(target.ndkAbi) }
    }

  packageAndroidRuntimeLibraries.configure { dependsOn(packageRuntimeTarget) }
  packageAndroidBindingLibraries.configure { dependsOn(packageBindingTarget) }
}
