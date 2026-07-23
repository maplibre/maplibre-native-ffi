---
title: Kotlin publishing
description: Maven publication design for the Kotlin Multiplatform binding and its native runtimes.
sidebar:
  order: 5
---

The Kotlin Multiplatform binding publishes snapshot builds through Maven. Stable
releases will use the finalization design below. Consumers obtain every MapLibre
Native FFI component through Gradle; the host operating system continues to
provide its graphics frameworks, loaders, and drivers.

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
arm64, plus Metal for macOS arm64, iOS arm64, and the iOS arm64 simulator. Each
published Kotlin/Native target has a matching runtime variant.

Gradle registers this target set consistently on every host. Local and CI
workflows invoke target-specific KLIB and test tasks, leaving targets
unavailable on the current host idle.

System graphics components remain platform dependencies. Examples include Metal
frameworks from the Apple SDK and EGL, OpenGL, and Vulkan loaders supplied by
the Linux system.

### Android

Each Android runtime AAR contains its backend-specific JNI libraries under
`jni/<abi>`. OpenGL and Vulkan use separate runtime publications because the
native library is compiled for one render backend.

The Android target of `maplibre-native-ffi` directly includes the JVM helper
from the exact `rustls-platform-verifier-android` package selected by
Cargo.lock. It also includes the helper's consumer R8 rule and upstream
licenses, which Gradle copies from the locked `rustls-platform-verifier`
package. Consumers do not add a Rustls-specific Maven dependency.

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
MapLibre Native FFI shared library and the runtime dependencies that the project
redistributes. The JVM binding extracts that packaged set to a versioned
directory and loads packaged dependencies before the C API library. Linux hosts
currently provide zlib and libuv, in addition to the selected graphics loader
and driver. Explicit native-library path configuration remains available as an
override.

## Snapshot publication

Snapshot versions end in `-SNAPSHOT` and publish from the exact commit that
passed the main CI workflow. A Linux x64 runner builds the Linux x64 publication
and cross-compiles the Linux ARM64 publication, while macOS runners build the
macOS and iOS publications. Each consumes native build artifacts produced by the
platform and backend CI matrix. Android publication runs with the Linux x64
partition; it reuses the matrix's CMake install archives and builds only the JNI
bridge and final AARs.

The Central Portal namespace covering `org.maplibre.nativeffi` must be
registered with snapshot publishing enabled. The repository stores a Central
Portal user token in the `MAVEN_CENTRAL_USERNAME` and `MAVEN_CENTRAL_PASSWORD`
GitHub Actions secrets. Snapshot publication does not require signing.

Each host first stages its publications in a local Maven repository. CI merges
the Linux x64, Linux ARM64, and Apple repositories, then inspects the published
AAR, JAR, and KLIB payloads and compiles the example consumers against the
merged repository. After every local check succeeds, each host publishes its own
target artifacts to the Central Portal. Linux and Apple leaf modules publish
first; the canonical multiplatform root modules publish last, after every leaf
upload succeeds. Snapshot workflows are serialized so two commits cannot
interleave those uploads. Publication jobs reuse the CI-produced native
archives; they do not rebuild MapLibre Native.

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

## Stable release finalization

Stable releases use the same host build and staging partitions as snapshots.
Host jobs have no publishing credentials: they upload their local Maven
repositories as workflow artifacts. A single finalizer then:

1. merges the partitions and rejects missing modules or conflicting paths;
2. runs the same semantic payload and coordinate verification used for
   snapshots;
3. signs every publishable file and writes the required checksums;
4. creates one ZIP whose root is the Maven repository layout; and
5. uploads that ZIP once through the Central Publisher API.

The first stable release should use a user-managed Central deployment so the
validated bundle can be inspected before it is published. Switching to automatic
publication changes only the final API request, not target builds, staging,
merging, signing, or bundle assembly.

This makes the merged, verified repository the release source of truth and gives
stable releases one Central deployment boundary. Snapshot uploads use Central's
mutable Maven snapshot endpoint, so their finalizer instead preserves
leaf-first/root-last ordering; it cannot provide the same atomic deployment
semantics.
