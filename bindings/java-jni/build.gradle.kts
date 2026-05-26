import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.testing.Test
import org.gradle.jvm.toolchain.JavaLanguageVersion
import org.gradle.jvm.toolchain.JavaToolchainService

plugins { `java-library` }

repositories { mavenCentral() }

dependencies {
  implementation("org.bytedeco:javacpp:1.5.11")

  testImplementation(platform("org.junit:junit-bom:6.0.3"))
  testImplementation("org.junit.jupiter:junit-jupiter")
  testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

val javaToolchains = project.extensions.getByType<JavaToolchainService>()
val java25Compiler = javaToolchains.compilerFor { languageVersion = JavaLanguageVersion.of(25) }

tasks.withType<JavaCompile>().configureEach {
  javaCompiler = java25Compiler
  options.release = 25
}

fun javaCppPlatform(): String {
  var osName = System.getProperty("os.name").lowercase()
  var osArch = System.getProperty("os.arch").lowercase()
  osName =
    when {
      osName.startsWith("mac") || osName.startsWith("darwin") -> "macosx"
      osName.startsWith("win") -> "windows"
      osName.startsWith("linux") -> "linux"
      else -> throw GradleException("Unsupported JavaCPP platform: $osName")
    }
  osArch =
    when (osArch) {
      "amd64",
      "x86_64" -> "x86_64"
      "aarch64",
      "arm64" -> "arm64"
      else -> throw GradleException("Unsupported JavaCPP architecture: $osArch")
    }
  return "$osName-$osArch"
}

val generatedJavaCppSources = layout.buildDirectory.dir("generated/sources/javacpp/main/java")
val javaCppConfigClasses = layout.buildDirectory.dir("classes/javacppConfig")

sourceSets.named("main") { java.srcDir(generatedJavaCppSources) }

val compileJavaCppConfig =
  tasks.register<JavaCompile>("compileJavaCppConfig") {
    source("src/main/java/org/maplibre/nativejni/internal/javacpp/MaplibreNativeCConfig.java")
    classpath = configurations.compileClasspath.get()
    destinationDirectory = javaCppConfigClasses
    javaCompiler = java25Compiler
    options.release = 25
  }

val generateJavaCppBindings =
  tasks.register<JavaExec>("generateJavaCppBindings") {
    group = "build"
    description = "Generates JavaCPP declarations for the MapLibre Native C ABI."
    dependsOn(compileJavaCppConfig)
    classpath = files(javaCppConfigClasses) + configurations.compileClasspath.get()
    mainClass = "org.bytedeco.javacpp.tools.Builder"
    args(
      "-classpath",
      classpath.asPath,
      "-Dplatform.includepath=${rootProject.layout.projectDirectory.dir("include").asFile.absolutePath}",
      "-d",
      generatedJavaCppSources.get().asFile.absolutePath,
      "-nogenerate",
      "org.maplibre.nativejni.internal.javacpp.MaplibreNativeCConfig",
    )
    inputs.file("src/main/java/org/maplibre/nativejni/internal/javacpp/MaplibreNativeCConfig.java")
    inputs.dir(rootProject.layout.projectDirectory.dir("include"))
    outputs.file(
      generatedJavaCppSources.map {
        it.file("org/maplibre/nativejni/internal/javacpp/MaplibreNativeC.java")
      }
    )
  }

tasks.named<JavaCompile>("compileJava") { dependsOn(generateJavaCppBindings) }

val nativeBuildDir = providers.environmentVariable("MLN_FFI_BUILD_DIR")
val javaCppPlatformName = javaCppPlatform()
val jniBridgeLibrary =
  layout.buildDirectory.file(
    "classes/java/main/org/maplibre/nativejni/internal/javacpp/$javaCppPlatformName/${System.mapLibraryName("jniMaplibreNativeC")}"
  )

val buildJavaCppNative =
  tasks.register<JavaExec>("buildJavaCppNative") {
    group = "build"
    description = "Builds the JavaCPP JNI bridge for the MapLibre Native C ABI."
    dependsOn(tasks.named("classes"))
    classpath = sourceSets.main.get().runtimeClasspath
    mainClass = "org.bytedeco.javacpp.tools.Builder"
    args(
      "-classpath",
      sourceSets.main.get().runtimeClasspath.asPath,
      "-Dplatform.linkpath=${nativeBuildDir.get()}",
      "org.maplibre.nativejni.internal.javacpp.MaplibreNativeC",
    )
    inputs.files(sourceSets.main.get().output.classesDirs)
    inputs.dir(rootProject.layout.projectDirectory.dir("include"))
    inputs.dir(nativeBuildDir)
    outputs.file(jniBridgeLibrary)
    mustRunAfter(tasks.named("compileTestJava"))
  }

tasks.named<Jar>("jar") { dependsOn(buildJavaCppNative) }

tasks.named<Javadoc>("javadoc") {
  dependsOn(tasks.classes)
  val main = sourceSets.main.get()
  source =
    main.allJava.matching {
      exclude("org/maplibre/nativejni/internal/**")
      exclude("module-info.java")
    }
  classpath = main.compileClasspath + main.output
  modularity.inferModulePath.set(false)
  isFailOnError = true
  options {
    encoding = "UTF-8"
    (this as StandardJavadocDocletOptions).apply {
      links("https://docs.oracle.com/en/java/javase/25/docs/api/")
    }
  }
}

val jniLibraryPathProperty = "org.maplibre.nativejni.library.path"

tasks.withType<Test>().configureEach {
  dependsOn(buildJavaCppNative)
  useJUnitPlatform()
  jvmArgs("--enable-native-access=ALL-UNNAMED")
  systemProperty(jniLibraryPathProperty, jniBridgeLibrary.get().asFile.absolutePath)
  inputs.file(jniBridgeLibrary).withPropertyName("jniBridgeLibrary")
}
