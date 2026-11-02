---
title: Kotlin
description: Add the Kotlin Multiplatform library to a project.
---

## Install

Add the binding to your common source set from Maven Central:

```kotlin title="build.gradle.kts"
repositories {
  mavenCentral()
}

kotlin {
  sourceSets {
    commonMain.dependencies {
      implementation("org.maplibre.nativeffi:maplibre-native-ffi:{{version}}")
    }
  }
}
```

Then add a runtime to each target, as the sections below show. A runtime holds
the native library for one
[backend](/maplibre-native-ffi/install/#choose-a-backend):
`maplibre-native-ffi-runtime-opengl`, `-metal`, `-vulkan`, or `-webgpu`.

## Android

Android needs API level 24 or later. Add a runtime to `androidMain`:

```kotlin title="build.gradle.kts"
androidMain.dependencies {
  implementation("org.maplibre.nativeffi:maplibre-native-ffi-runtime-vulkan:{{version}}")
}
```

Call `initAndroid(context)` once before you create a runtime.

## Native

Add a runtime to the source set of each native target, such as Metal for Apple
platforms and Vulkan for Linux:

```kotlin title="build.gradle.kts"
appleMain.dependencies {
  implementation("org.maplibre.nativeffi:maplibre-native-ffi-runtime-metal:{{version}}")
}
linuxMain.dependencies {
  implementation("org.maplibre.nativeffi:maplibre-native-ffi-runtime-vulkan:{{version}}")
}
```

## JVM

The JVM needs JDK 25 or later. Its runtime holds the native library in a
separate JAR for each platform. Add the JAR for each platform that your
application runs on, by its classifier: `natives-<os>-<arch>`, such as
`natives-linux-x64` or `natives-macos-arm64`. Use `linux-musl` for musl, and
`windows-mingw` for MinGW on Windows.

```kotlin title="build.gradle.kts"
jvmMain.dependencies {
  runtimeOnly("org.maplibre.nativeffi:maplibre-native-ffi-runtime-vulkan-jvm:{{version}}:natives-linux-x64")
}
```

The binding calls native code through Java's foreign function API, so start the
JVM with `--enable-native-access=ALL-UNNAMED`.

## Wasm

Add the OpenGL runtime to `wasmJsMain` to render with WebGL, or the WebGPU
runtime to render with WebGPU. Before you use the binding, call
`loadWasmModule()` and wait for it. The page must meet the
[web requirements](/maplibre-native-ffi/install/#web).

## Snapshots

Snapshots of `main` publish daily to the Maven snapshot repository, as version
`0.1.0-SNAPSHOT`:

```kotlin title="build.gradle.kts"
repositories {
  maven("https://central.sonatype.com/repository/maven-snapshots/") {
    content { includeGroup("org.maplibre.nativeffi") }
  }
  mavenCentral()
}
```

Then use `0.1.0-SNAPSHOT` as the version of the binding and its runtimes.
