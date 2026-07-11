import org.gradle.api.file.Directory
import org.gradle.api.provider.Provider
import org.gradle.api.tasks.Exec
import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.Sync
import org.gradle.api.tasks.compile.JavaCompile
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.catalogVersion
import org.maplibre.nativeffi.gradle.catalogVersionInt

val hostPlatform = HostPlatform.current()
val javaCppToolClasspath = configurations.getByName("javaCppTool")
val androidApiLevel = catalogVersionInt("android-minSdk")
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
  (extensions.extraProperties["maplibreAndroidSdkDirectory"] as Provider<Directory>).get().asFile
val androidNdkDirectory = androidSdkDirectory.resolve("ndk/${catalogVersion("android-ndk")}")
val androidNdkPrebuilt =
  androidNdkDirectory.resolve("toolchains/llvm/prebuilt/${hostPlatform.androidNdkPrebuiltTag}")
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
val packagedAndroidNativeLibs = layout.buildDirectory.dir("generated/jniLibs/androidMain")

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

val packageAndroidNativeLibraries =
  tasks.register("packageAndroidNativeLibraries") {
    group = "build"
    description = "Packages MapLibre C, JavaCPP JNI, and libc++ libraries for Android."
  }

androidTargets.forEach { target ->
  val targetRoot = layout.buildDirectory.dir("android-native/$androidBackend/${target.cargoTarget}")
  val cmakePreset = target.cmakePreset(androidBackend)
  val installDir = rootProject.layout.projectDirectory.dir("build/$cmakePreset/install")
  val javaCppNativeBuild = targetRoot.map { it.dir("javacpp") }
  val packageDir = packagedAndroidNativeLibs.map { it.dir("$androidBackend/${target.cargoTarget}") }
  val nativeLibrary = installDir.file("lib/libmaplibre-native-c.so")
  val javaCppNativeLibrary = javaCppNativeBuild.map { it.file("libjniMaplibreNativeC.so") }
  val ndkCompiler =
    androidNdkPrebuilt.resolve(
      "bin/${target.ndkCompilerName(androidApiLevel)}${hostPlatform.androidNdkCommandSuffix}"
    )
  val libcxxShared =
    androidNdkPrebuilt.resolve("sysroot/usr/lib/${target.ndkTargetTriple}/libc++_shared.so")

  val buildNative =
    tasks.register<Exec>("buildMaplibreNativeCAndroid${target.taskSuffix}") {
      group = "build"
      description = "Builds and installs MapLibre Native C for Android ${target.ndkAbi}."
      doNotTrackState("CMake owns native incremental build state")
      workingDir(repositoryRoot)
      environment("ANDROID_HOME", androidSdkDirectory.absolutePath)
      executable("cmake")
      args("--workflow", "--preset", cmakePreset)
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
        "-Dplatform.compiler=${ndkCompiler.absolutePath}",
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
        "org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC",
        "org.maplibre.nativeffi.internal.javacpp.AndroidNativeBridge",
      )
      inputs.files(javaCppConfigSources.map(::file))
      inputs.dir(javaCppAndroidIncludes)
      inputs.dir(checkedInCHeaders).withPropertyName("maplibreNativeCHeaders")
      inputs.file(nativeLibrary).withPropertyName("maplibreNativeCLibrary")
      inputs.file(ndkCompiler).withPropertyName("androidNdkCompiler")
      outputs.file(javaCppNativeLibrary)
    }

  val packageTarget =
    tasks.register<Sync>("packageAndroidNativeLibraries${target.taskSuffix}") {
      description = "Packages Android ${target.ndkAbi} native libraries."
      dependsOn(generateJavaCppNativeLibrary)
      into(packageDir)
      from(nativeLibrary) { into(target.ndkAbi) }
      from(javaCppNativeLibrary) { into(target.ndkAbi) }
      from(libcxxShared) { into(target.ndkAbi) }
    }

  packageAndroidNativeLibraries.configure { dependsOn(packageTarget) }
}
