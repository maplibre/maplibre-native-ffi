import org.gradle.api.tasks.Delete
import org.gradle.api.tasks.JavaExec
import org.gradle.api.tasks.Sync
import org.gradle.api.tasks.compile.JavaCompile
import org.maplibre.nativeffi.gradle.AndroidTarget
import org.maplibre.nativeffi.gradle.HostPlatform
import org.maplibre.nativeffi.gradle.MaplibreNativeCArtifact
import org.maplibre.nativeffi.gradle.catalogVersionInt

val hostPlatform = HostPlatform.current()
val maplibreNativeC = extensions.getByType<MaplibreNativeCArtifact>()
val javaCppToolClasspath = configurations.getByName("javaCppTool")

val javaCppConfigSources =
  listOf(
    "src/androidMain/java/org/maplibre/nativeffi/internal/javacpp/MaplibreNativeCConfig.java",
    "src/androidMain/java/org/maplibre/nativeffi/internal/javacpp/AndroidNativeBridge.java",
  )

val generatedJavaCppSources =
  layout.buildDirectory.dir("generated/sources/javacpp/androidMain/java")
val generatedJavaCppClasses = layout.buildDirectory.dir("classes/javacppGenerated")
val generatedJavaCppNativeBuild =
  layout.buildDirectory.dir("generated/sources/javacpp/androidMain/native")
val packagedAndroidNativeLibs = layout.buildDirectory.dir("generated/jniLibs/androidMain")
val javaCppAndroidIncludes = layout.projectDirectory.dir("src/androidMain/javacpp")
val javaCppConfigClasses = layout.buildDirectory.dir("classes/javacppConfig")
val javaCppAndroidCompatHeader = javaCppAndroidIncludes.file("javacpp_android_compat.h")

val androidNdkHome = providers.environmentVariable("ANDROID_NDK_HOME")

fun registerDisabledAndroidJavaCppTask(name: String) {
  tasks.register(name) {
    group = "build"
    enabled = false
    onlyIf { false }
  }
}

if (AndroidTarget.fromEnv() != null) {
  val androidTarget = AndroidTarget.current()

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
        "-Dplatform.includepath=${maplibreNativeC.includeDirs.joinToString(File.pathSeparator)}",
        "-d",
        generatedJavaCppSources.get().asFile.absolutePath,
        "-nogenerate",
        "org.maplibre.nativeffi.internal.javacpp.MaplibreNativeCConfig",
        "org.maplibre.nativeffi.internal.javacpp.AndroidNativeBridge",
      )
      inputs.files(javaCppConfigSources.map { file(it) })
      inputs.files(maplibreNativeC.includeDirs).withPropertyName("maplibreNativeCIncludeDirs")
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

  val generateJavaCppNativeLibrary =
    tasks.register<JavaExec>("generateAndroidJavaCppNativeLibrary") {
      group = "build"
      description = "Generates the Android JavaCPP JNI library for the MapLibre Native C ABI."
      dependsOn(compileGeneratedJavaCppBindings)
      classpath = files(generatedJavaCppClasses, javaCppConfigClasses) + javaCppToolClasspath
      mainClass = "org.bytedeco.javacpp.tools.Builder"
      args(
        "-classpath",
        classpath.asPath,
        "-properties",
        androidTarget.javaCppPlatform,
        "-Dplatform.compiler=${androidNdkHome.get()}/toolchains/llvm/prebuilt/${hostPlatform.androidNdkPrebuiltTag}/bin/${androidTarget.ndkCompilerTriple}-clang++",
        "-Dplatform.includepath=${(maplibreNativeC.includeDirs + javaCppAndroidIncludes.asFile).joinToString(File.pathSeparator)}",
        "-Dplatform.linkpath=${maplibreNativeC.linkDirs.joinToString(File.pathSeparator)}",
        "-clean",
        "-d",
        generatedJavaCppNativeBuild.get().asFile.absolutePath,
        "-o",
        "jniMaplibreNativeC",
        "-Xcompiler",
        "-include",
        "-Xcompiler",
        javaCppAndroidCompatHeader.asFile.absolutePath,
        "org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC",
        "org.maplibre.nativeffi.internal.javacpp.AndroidNativeBridge",
      )
      inputs.files(javaCppConfigSources.map { file(it) })
      inputs.dir(javaCppAndroidIncludes)
      inputs.files(maplibreNativeC.includeDirs).withPropertyName("maplibreNativeCIncludeDirs")
      inputs.files(maplibreNativeC.linkDirs).withPropertyName("maplibreNativeCLinkDirs")
      inputs.file(maplibreNativeC.libraryPath).withPropertyName("maplibreNativeCLibrary")
      outputs.file(generatedJavaCppNativeBuild.map { it.file("libjniMaplibreNativeC.so") })
    }

  val cleanPackagedAndroidNativeLibs =
    tasks.register<Delete>("cleanPackagedAndroidNativeLibs") { delete(packagedAndroidNativeLibs) }

  tasks.register<Sync>("packageAndroidNativeLibraries") {
    dependsOn(generateJavaCppNativeLibrary, cleanPackagedAndroidNativeLibs)
    from(generatedJavaCppNativeBuild.map { it.file("libjniMaplibreNativeC.so") })
    from(maplibreNativeC.libraryPath)
    into(packagedAndroidNativeLibs.map { it.dir(androidTarget.ndkAbi) })
  }
} else {
  registerDisabledAndroidJavaCppTask("compileAndroidJavaCppConfig")
  registerDisabledAndroidJavaCppTask("generateAndroidJavaCppBindings")
  registerDisabledAndroidJavaCppTask("compileGeneratedAndroidJavaCppBindings")
  registerDisabledAndroidJavaCppTask("generateAndroidJavaCppNativeLibrary")
  registerDisabledAndroidJavaCppTask("packageAndroidNativeLibraries")
}
