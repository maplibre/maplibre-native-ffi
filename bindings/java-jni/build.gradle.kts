import org.gradle.api.tasks.compile.JavaCompile
import org.gradle.api.tasks.testing.Test

plugins { `java-library` }

repositories { mavenCentral() }

dependencies {
  implementation("org.bytedeco:javacpp:1.5.11")

  testImplementation(platform("org.junit:junit-bom:6.0.3"))
  testImplementation("org.junit.jupiter:junit-jupiter")
  testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

tasks.withType<JavaCompile>().configureEach { options.release = 25 }

val nativeBuildDir =
  providers
    .environmentVariable("MLN_FFI_BUILD_DIR")
    .orElse(rootProject.layout.projectDirectory.dir("build/macos-arm64-metal").asFile.absolutePath)

val javaCppNativeOutputDir = layout.buildDirectory.dir("classes/java/main")

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
    outputs.dir(javaCppNativeOutputDir)
  }

val copyJavaCppNative =
  tasks.register<Copy>("copyJavaCppNative") {
    dependsOn(buildJavaCppNative)
    from(javaCppNativeOutputDir) { include("**/${System.mapLibraryName("jniMaplibreNativeC")}") }
    into(javaCppNativeOutputDir)
    includeEmptyDirs = false
    duplicatesStrategy = DuplicatesStrategy.INCLUDE
    eachFile { path = name }
  }

val javaCppNativeLibraryPath = javaCppNativeOutputDir.map {
  it.file(System.mapLibraryName("jniMaplibreNativeC")).asFile.absolutePath
}

val javaCppTestNativeOutputDir = layout.buildDirectory.dir("classes/java/test")

val buildJavaCppTestNative =
  tasks.register<JavaExec>("buildJavaCppTestNative") {
    group = "verification"
    description = "Builds the JavaCPP JNI bridge used by JavaCPP boundary tests."
    dependsOn(tasks.named("compileTestJava"))
    classpath = sourceSets.test.get().runtimeClasspath
    mainClass = "org.bytedeco.javacpp.tools.Builder"
    args(
      "-classpath",
      sourceSets.test.get().runtimeClasspath.asPath,
      "org.maplibre.nativejni.internal.javacpp.JavaCppBoundaryTestC",
    )
    inputs.files(sourceSets.test.get().output.classesDirs)
    inputs.dir(layout.projectDirectory.dir("src/test/javacpp"))
    outputs.dir(javaCppTestNativeOutputDir)
  }

tasks.named<Jar>("jar") { dependsOn(copyJavaCppNative) }

tasks.named<JavaCompile>("compileTestJava") { dependsOn(copyJavaCppNative) }

tasks.named<Javadoc>("javadoc") {
  isFailOnError = true
  options {
    encoding = "UTF-8"
    (this as StandardJavadocDocletOptions).apply {
      links("https://docs.oracle.com/en/java/javase/25/docs/api/")
      addBooleanOption("Xdoclint:none", true)
      addStringOption("exclude", "org.maplibre.nativejni.internal:*")
    }
  }
}

val nativeJniLibraryPath = providers.systemProperty("org.maplibre.nativejni.library.path")
val nativeJniLibraryEnv = providers.environmentVariable("MAPLIBRE_NATIVE_JNI_LIBRARY_PATH")
val javaLibraryPath = providers.systemProperty("java.library.path")
val requireNativeTests = providers.systemProperty("org.maplibre.nativejni.tests.requireNative")

tasks.withType<Test>().configureEach {
  dependsOn(copyJavaCppNative)
  dependsOn(buildJavaCppTestNative)
  useJUnitPlatform()
  jvmArgs("--enable-native-access=ALL-UNNAMED")
  inputs.property("org.maplibre.nativejni.library.path", nativeJniLibraryPath.orElse(""))
  inputs.property("MAPLIBRE_NATIVE_JNI_LIBRARY_PATH", nativeJniLibraryEnv.orElse(""))
  inputs.property("java.library.path", javaLibraryPath.orElse(""))
  inputs.property("org.maplibre.nativejni.tests.requireNative", requireNativeTests.orElse(""))
  systemProperty(
    "org.maplibre.nativejni.library.path",
    nativeJniLibraryPath.orElse(javaCppNativeLibraryPath).get(),
  )
  javaLibraryPath.orNull?.let { systemProperty("java.library.path", it) }
  requireNativeTests.orNull?.let {
    systemProperty("org.maplibre.nativejni.tests.requireNative", it)
  }
}
