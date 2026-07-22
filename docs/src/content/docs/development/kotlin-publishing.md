---
title: Kotlin publishing
description: Maven publication design for the Kotlin Multiplatform binding and its native runtimes.
sidebar:
  order: 5
---

The Kotlin Multiplatform binding publishes snapshot and release builds through
Maven. Consumers obtain every MapLibre Native FFI component through Gradle; the
host operating system continues to provide its graphics frameworks, loaders, and
drivers.

## Coordinates

All publications use the `org.maplibre.nativeffi` group and share one version.
Snapshot consumers add the Central Portal snapshot repository because snapshots
are not served by the immutable Maven Central release repository.

```kotlin
repositories {
  maven {
    url = uri("https://central.sonatype.com/repository/maven-snapshots/")
    content { includeGroup("org.maplibre.nativeffi") }
  }
  mavenCentral()
}
```

| Artifact                             | Contents                                      |
| ------------------------------------ | --------------------------------------------- |
| `maplibre-native-ffi`                | Backend-agnostic Kotlin Multiplatform binding |
| `maplibre-native-ffi-runtime-opengl` | OpenGL, EGL, or WGL native runtime            |
| `maplibre-native-ffi-runtime-vulkan` | Vulkan native runtime                         |
| `maplibre-native-ffi-runtime-metal`  | Metal native runtime                          |

Each runtime publication depends on `maplibre-native-ffi`. Applications whose
targets are all supported by one backend may depend on that runtime from
`commonMain`. Applications using different backends select runtime publications
from their platform source sets.

```kotlin
commonMain.dependencies {
  implementation("org.maplibre.nativeffi:maplibre-native-ffi:$version")
}

androidMain.dependencies {
  implementation("org.maplibre.nativeffi:maplibre-native-ffi-runtime-opengl:$version")
}

linuxMain.dependencies {
  implementation("org.maplibre.nativeffi:maplibre-native-ffi-runtime-opengl:$version")
}

macosMain.dependencies {
  implementation("org.maplibre.nativeffi:maplibre-native-ffi-runtime-metal:$version")
}
```

## Platform payloads

### Kotlin/Native

Each native runtime KLIB embeds the complete backend-specific static archive
produced by the CMake build. Its cinterop definition also carries the system
link requirements for that target and backend. The final application links the
archive without acquiring a separate MapLibre Native FFI shared library or
framework.

The initial native runtime publications are OpenGL and Vulkan for Linux x64 and
arm64, plus Metal for macOS arm64. Other native targets can be added when their
consumer path is implemented.

System graphics components remain platform dependencies. Examples include Metal
frameworks from the Apple SDK and EGL, OpenGL, and Vulkan loaders supplied by
the Linux system.

### Android

Each Android runtime AAR contains its backend-specific JNI libraries under
`jni/<abi>`. OpenGL and Vulkan use separate runtime publications because the
native library is compiled for one render backend.

The Android target of `maplibre-native-ffi` directly includes the Kotlin helper
required by `rustls-platform-verifier`. It also includes the helper's consumer
R8 rule, upstream license, and provenance. The helper version is updated with
the Rust dependency. Consumers do not add a Rustls-specific Maven dependency.

### JVM

The runtime module name selects the render backend. A classifier on the JVM
runtime artifact selects the operating system and architecture.

```kotlin
implementation("org.maplibre.nativeffi:maplibre-native-ffi:$version")
runtimeOnly(
  "org.maplibre.nativeffi:maplibre-native-ffi-runtime-opengl-jvm:$version:natives-linux-x64"
)
```

Classifier names use `natives-<os>-<arch>`, for example `natives-linux-arm64`,
`natives-macos-arm64`, and `natives-windows-x64`. The runtime JAR contains the
MapLibre Native FFI shared library and its redistributable runtime dependencies.
The JVM binding extracts the complete runtime set to a versioned directory and
loads dependencies before the C API library. Explicit native-library path
configuration remains available as an override.

## Snapshot publication

Snapshot versions end in `-SNAPSHOT` and publish from the exact commit that
passed the main CI workflow. One macOS runner coordinates Maven publication and
Apple Kotlin/Native compilation. It consumes native build artifacts produced by
the platform and backend CI matrix.

The Central Portal namespace covering `org.maplibre.nativeffi` must be
registered with snapshot publishing enabled. The repository stores a Central
Portal user token in the `MAVEN_CENTRAL_USERNAME` and `MAVEN_CENTRAL_PASSWORD`
GitHub Actions secrets. Snapshot publication does not require signing.

The publish task first stages all publications in a local Maven repository. It
then inspects the published AAR, JAR, and KLIB payloads and compiles the example
consumers against that repository. Upload to the Central Portal begins only
after every local check succeeds.

The initial snapshot workflow validates:

- Maven and Gradle module metadata for every publication;
- native archive presence in published KLIBs;
- JNI library and Rustls helper presence in Android AARs;
- native resource presence in JVM classifier JARs;
- published JVM consumption through the Compose and LWJGL examples;
- published Android consumption through the Android map example.

Existing binding tests continue to cover Kotlin/Native behavior. An iOS target
for the Compose map example is a separate follow-up and is not required for the
initial snapshot publication.
