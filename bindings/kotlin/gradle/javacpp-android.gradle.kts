import org.gradle.api.file.Directory
import org.gradle.api.provider.Provider
import org.gradle.api.tasks.Exec
import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.Sync
import org.gradle.api.tasks.bundling.Zip
import org.gradle.api.tasks.compile.JavaCompile
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.catalogVersionInt
import org.maplibre.nativeffi.gradle.requiredEnvironmentVariable

val hostPlatform = HostPlatform.current()
val javaCppToolClasspath = configurations.getByName("javaCppTool")
val androidApiLevel = catalogVersionInt("android-minSdk")
val androidNdkVersion = requiredEnvironmentVariable("MLN_FFI_ANDROID_NDK_VERSION")
val androidBackend =
  AndroidTarget.parseBackend(
    providers.gradleProperty("maplibre.android.backend").getOrElse(AndroidTarget.DEFAULT_BACKEND)
  )
val androidTargets =
  AndroidTarget.parseAbis(
    providers.gradleProperty("maplibre.android.abis").getOrElse(AndroidTarget.DEFAULT_ABIS)
  )
@Suppress("UNCHECKED_CAST")
val androidSdkDirectory =
  extensions.extraProperties["maplibreAndroidSdkDirectory"] as Provider<Directory>
val androidNdkPrebuilt = androidSdkDirectory.map {
  it.dir("ndk/$androidNdkVersion/toolchains/llvm/prebuilt/${hostPlatform.androidNdkPrebuiltTag}")
}
// Resolve the NOTICE from the SDK Gradle selected, which is the same SDK whose
// NDK supplies the statically linked libc++. Reading ANDROID_HOME directly would
// pair the bridge with a notice from a different NDK whenever local.properties
// or ANDROID_SDK_ROOT selects another SDK.
val androidNdkNotice = androidSdkDirectory.map { it.file("ndk/$androidNdkVersion/NOTICE") }
val repositoryRoot = rootProject.layout.projectDirectory.asFile

val javaCppConfigSources =
  listOf(
    "src/androidMain/java/org/maplibre/nativeffi/internal/javacpp/MaplibreNativeCConfig.java",
    "src/androidMain/java/org/maplibre/nativeffi/internal/javacpp/AndroidNativeBridge.java",
  )
val checkedInCHeaders = rootProject.layout.projectDirectory.dir("include")
val generatedJavaCppSources =
  layout.buildDirectory.dir("generated/sources/javacpp/androidMain/java")
val generatedJavaCppClasses = layout.buildDirectory.dir("classes/javacppGenerated")
val javaCppConfigClasses = layout.buildDirectory.dir("classes/javacppConfig")
val javaCppAndroidIncludes = layout.projectDirectory.dir("src/androidMain/javacpp")
val javaCppAndroidCompatHeader = javaCppAndroidIncludes.file("javacpp_android_compat.h")
// The C API library is the backend-specific payload of the runtime AARs, and any
// Android host packages it. The JavaCPP bridge is private to this binding, so it
// travels in this module's own AAR and stays out of a non-Kotlin host's APK.
val packagedAndroidRuntimeLibs = layout.buildDirectory.dir("generated/jniLibs/runtime")
@Suppress("UNCHECKED_CAST")
val packagedAndroidBindingLibs =
  extensions.extraProperties["maplibreAndroidBindingLibsDirectory"] as Provider<Directory>
// Snapshot publishing extracts the Android CMake packages produced by target CI here,
// allowing the publication job to reuse them instead of rebuilding MapLibre Native.
val prebuiltAndroidInstallRoot =
  providers.gradleProperty("maplibre.android.prebuiltInstallRoot").map(rootProject::file)
// Target CI round-trips packages into each CMake preset's install directory.
val prebuiltAndroidBuildRoot =
  providers.gradleProperty("maplibre.android.prebuiltBuildRoot").map(rootProject::file)

check(!(prebuiltAndroidInstallRoot.isPresent && prebuiltAndroidBuildRoot.isPresent)) {
  "Configure only one of maplibre.android.prebuiltInstallRoot and maplibre.android.prebuiltBuildRoot"
}

val compileJavaCppConfig =
  tasks.register<JavaCompile>("compileAndroidJavaCppConfig") {
    source(javaCppConfigSources)
    classpath = javaCppToolClasspath
    destinationDirectory = javaCppConfigClasses
    options.release = catalogVersionInt("java-android-release")
  }

val generateJavaCppBindings =
  tasks.register<JavaExec>("generateAndroidJavaCppBindings") {
    group = "build"
    description = "Generates JavaCPP declarations for the Android MapLibre Native C ABI."
    dependsOn(compileJavaCppConfig)
    classpath = files(javaCppConfigClasses) + javaCppToolClasspath
    mainClass = "org.bytedeco.javacpp.tools.Builder"
    args(
      "-classpath",
      classpath.asPath,
      "-Dplatform.includepath=${checkedInCHeaders.asFile.absolutePath}",
      "-d",
      generatedJavaCppSources.get().asFile.absolutePath,
      "-nogenerate",
      "org.maplibre.nativeffi.internal.javacpp.MaplibreNativeCConfig",
      "org.maplibre.nativeffi.internal.javacpp.AndroidNativeBridge",
    )
    inputs.files(javaCppConfigSources.map(::file))
    inputs.dir(checkedInCHeaders).withPropertyName("maplibreNativeCHeaders")
    inputs.dir(javaCppAndroidIncludes)
    outputs.file(
      generatedJavaCppSources.map {
        it.file("org/maplibre/nativeffi/internal/javacpp/MaplibreNativeC.java")
      }
    )
  }

val compileGeneratedJavaCppBindings =
  tasks.register<JavaCompile>("compileGeneratedAndroidJavaCppBindings") {
    dependsOn(generateJavaCppBindings)
    source(generatedJavaCppSources)
    classpath = files(javaCppConfigClasses) + javaCppToolClasspath
    destinationDirectory = generatedJavaCppClasses
    options.release = catalogVersionInt("java-android-release")
  }

val packageAndroidRuntimeLibraries =
  tasks.register("packageAndroidRuntimeLibraries") {
    group = "build"
    description = "Packages the MapLibre C library for the Android runtime AARs."
  }

val packageAndroidBindingLibraries =
  tasks.register("packageAndroidBindingLibraries") {
    group = "build"
    description = "Packages the JavaCPP JNI bridge for the Android binding AAR."
  }

val packageAndroidNativeLibraries =
  tasks.register("packageAndroidNativeLibraries") {
    group = "build"
    description = "Packages every Android native library this repository publishes."
    dependsOn(packageAndroidRuntimeLibraries, packageAndroidBindingLibraries)
  }

// The bridge links the NDK C++ runtime statically, so this AAR redistributes
// NDK-licensed code on its own and carries the notice with it rather than
// leaning on a runtime AAR that a consumer resolves separately.
tasks.withType<Zip>().configureEach {
  if (name == "bundleAndroidMainAar") {
    from(androidNdkNotice) { into("META-INF/licenses/android-ndk") }
  }
}

androidTargets.forEach { target ->
  val targetRoot = layout.buildDirectory.dir("android-native/$androidBackend/${target.cargoTarget}")
  val cmakePreset = target.cmakePreset(androidBackend)
  val configuredInstallDir =
    prebuiltAndroidBuildRoot
      .map { it.resolve(cmakePreset).resolve("install") }
      .orElse(prebuiltAndroidInstallRoot.map { it.resolve(cmakePreset) })
  val installDir =
    rootProject.layout
      .dir(configuredInstallDir)
      .orElse(rootProject.layout.projectDirectory.dir("build/$cmakePreset/install"))
      .get()
  val javaCppNativeBuild = targetRoot.map { it.dir("javacpp") }
  val runtimePackageDir = packagedAndroidRuntimeLibs.map {
    it.dir("$androidBackend/${target.cargoTarget}")
  }
  val bindingPackageDir = packagedAndroidBindingLibs.map { it.dir(target.cargoTarget) }
  val nativeLibrary = installDir.file("lib/libmaplibre-native-c.so")
  val javaCppNativeLibrary = javaCppNativeBuild.map { it.file("libjniMaplibreNativeC.so") }
  val ndkCompiler = androidNdkPrebuilt.map {
    it.file("bin/${target.ndkCompilerName(androidApiLevel)}${hostPlatform.androidNdkCommandSuffix}")
  }

  val buildNative =
    tasks.register<Exec>("buildMaplibreNativeCAndroid${target.taskSuffix}") {
      group = "build"
      description = "Builds and installs MapLibre Native C for Android ${target.ndkAbi}."
      doNotTrackState("CMake owns native incremental build state")
      workingDir(repositoryRoot)
      executable("cmake")
      args("--workflow", "--preset", cmakePreset)
      environment("ANDROID_HOME", androidSdkDirectory.get().asFile.absolutePath)
      enabled = !configuredInstallDir.isPresent
    }

  val generateJavaCppNativeLibrary =
    tasks.register<JavaExec>("generateAndroidJavaCppNativeLibrary${target.taskSuffix}") {
      group = "build"
      description = "Generates the Android ${target.ndkAbi} JavaCPP JNI library."
      dependsOn(buildNative, compileGeneratedJavaCppBindings)
      classpath = files(generatedJavaCppClasses, javaCppConfigClasses) + javaCppToolClasspath
      mainClass = "org.bytedeco.javacpp.tools.Builder"
      args(
        "-classpath",
        classpath.asPath,
        "-properties",
        target.javaCppPlatform,
        "-Dplatform.compiler=${ndkCompiler.get().asFile.absolutePath}",
        "-Dplatform.includepath=${listOf(checkedInCHeaders.asFile, javaCppAndroidIncludes.asFile).joinToString(File.pathSeparator)}",
        "-Dplatform.linkpath=${installDir.dir("lib").asFile.absolutePath}",
        "-d",
        javaCppNativeBuild.get().asFile.absolutePath,
        "-o",
        "jniMaplibreNativeC",
        "-Xcompiler",
        "-include",
        "-Xcompiler",
        javaCppAndroidCompatHeader.asFile.absolutePath,
        // The C API library links libc++ statically, so the bridge does too and
        // the AARs redistribute no libc++_shared.so. --exclude-libs keeps the
        // static runtime's symbols out of the dynamic table, which leaves the
        // JNI entry points as the only symbols an app can bind to.
        "-Xcompiler",
        "-static-libstdc++",
        "-Xcompiler",
        "-Wl,--exclude-libs,ALL",
        "org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC",
        "org.maplibre.nativeffi.internal.javacpp.AndroidNativeBridge",
      )
      inputs.files(javaCppConfigSources.map(::file))
      inputs.dir(javaCppAndroidIncludes)
      inputs.dir(checkedInCHeaders).withPropertyName("maplibreNativeCHeaders")
      inputs.file(nativeLibrary).withPropertyName("maplibreNativeCLibrary")
      inputs.file(ndkCompiler).withPropertyName("androidNdkCompiler")
      outputs.file(javaCppNativeLibrary)
      doLast {
        val asLatin1 = javaCppNativeLibrary.get().asFile.readBytes().toString(Charsets.ISO_8859_1)
        check("libc++_shared.so" !in asLatin1) {
          "The Android ${target.ndkAbi} JavaCPP bridge depends on libc++_shared.so; it links " +
            "libc++ statically so that every packaged library stands on its own"
        }
      }
    }

  val packageRuntimeTarget =
    tasks.register<Sync>("packageAndroidRuntimeLibraries${target.taskSuffix}") {
      description = "Packages the Android ${target.ndkAbi} MapLibre C library."
      dependsOn(buildNative)
      into(runtimePackageDir)
      from(nativeLibrary) { into(target.ndkAbi) }
      doLast {
        val packaged = runtimePackageDir.get().file("${target.ndkAbi}/libmaplibre-native-c.so")
        val asLatin1 = packaged.asFile.readBytes().toString(Charsets.ISO_8859_1)
        check("libc++_shared.so" !in asLatin1) {
          "The Android ${target.ndkAbi} MapLibre C library depends on libc++_shared.so; the " +
            "Android presets set ANDROID_STL=c++_static so the AAR carries one library per ABI"
        }
      }
    }

  val packageBindingTarget =
    tasks.register<Sync>("packageAndroidBindingLibraries${target.taskSuffix}") {
      description = "Packages the Android ${target.ndkAbi} JavaCPP JNI bridge."
      dependsOn(generateJavaCppNativeLibrary)
      into(bindingPackageDir)
      from(javaCppNativeLibrary) { into(target.ndkAbi) }
    }

  packageAndroidRuntimeLibraries.configure { dependsOn(packageRuntimeTarget) }
  packageAndroidBindingLibraries.configure { dependsOn(packageBindingTarget) }
}
