---
title: Kotlin publishing
description: Maven publication design for the Kotlin Multiplatform binding and its native runtimes.
sidebar:
  order: 6
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

Applications declare the binding and a runtime separately: the binding carries
the API, and a runtime carries the native payload for one backend. Applications
whose targets are all supported by one backend may declare that runtime in
`commonMain`. Applications using different backends select runtime publications
from their platform source sets. The runtimes depend on nothing of ours, which
keeps the Android runtime AARs consumable by hosts that are not written in
Kotlin.

Every `maplibre-native-ffi` module attaches Dokka-generated API documentation as
its Maven javadoc artifact. The generated site covers the common API and each
platform API. The runtime modules carry no public API, so their javadoc
artifacts remain empty placeholders for the Central requirement.

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

The native runtime publications are OpenGL and Vulkan for Linux x64, plus Metal
for macOS arm64, iOS arm64, and the iOS arm64 simulator. Each published
Kotlin/Native target has a matching runtime variant. Linux arm64 is absent
because Kotlin/Native has no arm64 Linux host, so that target would have to be
cross-compiled.

The Kotlin/Native Linux toolchain is the tightest consumer of the Linux archive.
Its sysroot supplies glibc 2.19 and GCC 8.3, and it statically links its own
libstdc++ into every consumer binary. Two properties of the archive keep that
link working, and both come from the zig toolchain described in the
[overview](/maplibre-native-ffi/development/overview/):

- The archive requires no glibc symbol newer than 2.17, because the toolchain
  targets that release rather than the build host's glibc.
- The archive carries its own C++ runtime with internal linkage, so it never
  needs, and never collides with, the C++ runtime a consumer links. Only the
  `mln_*` entry points keep external linkage.

Neither backend declares a graphics link requirement. Both open their loader on
first use, which matters here because the Kotlin/Native sysroot carries no
graphics libraries for a consumer's link to resolve one against.

Kotlin/Native test binaries in this repository link the C API shared library
instead of the archive. That library carries the same glibc floor, so the
Kotlin/Native sysroot resolves its references directly.

Gradle registers this target set consistently on every host. Local and CI
workflows invoke target-specific KLIB and test tasks, leaving targets
unavailable on the current host idle.

System graphics components remain platform dependencies, such as the Metal
frameworks supplied by the Apple SDK.

### Android

Each Android runtime AAR contains `jni/<abi>/libmaplibre-native-c.so` for its
backend. OpenGL and Vulkan use separate runtime publications because the native
library is compiled for one render backend, and both AARs name the library
identically, so an app packages one of them. The Android presets link libc++
statically and the C API library exports only its `mln_*` entry points, so the
AAR carries that one library per ABI and redistributes no `libc++_shared.so`.
Runtime AARs include the native dependency notices and the Android NDK notice
under `META-INF/licenses`.

Each Android runtime AAR also contains the JVM helper built from the pinned,
patched `rustls-platform-verifier` checkout acquired by mise, its consumer R8
rule, and the upstream licenses. The helper and Rust JNI descriptors use the
`org.maplibre.nativeffi.internal.rustlsplatformverifier` package so an app may
also consume the upstream helper without duplicate classes. Consumers add no
Rustls-specific Maven dependency.

The helper travels with the native library rather than with the Kotlin binding
because the native TLS stack is its only caller. An Android host written in
another language depends on a runtime publication alone and gets a complete,
packageable native payload; it binds nothing in the helper and adds no keep rule
of its own. Such a host still calls `mln_android_init` before creating a
runtime.

The Android target of `maplibre-native-ffi` carries the Kotlin API, the JavaCPP
bridge classes, and `jni/<abi>/libjniMaplibreNativeC.so`, which is private to
this binding. It publishes a consumer R8 rule for JavaCPP, which reads the
generated presets class reflectively and derives the JNI library name from a
live stack trace, so both survive minification only when R8 leaves the presets
package and the JavaCPP runtime package alone. Apps that minify get that rule
from the publication and add none of their own. The bridge links the NDK C++
runtime statically, so this AAR carries the Android NDK notice under
`META-INF/licenses` alongside the binary that embeds it.

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
redistributes. It carries their notices under
`META-INF/licenses/maplibre-native-c`. The JVM binding extracts that packaged
set to a versioned directory and loads packaged dependencies before the C API
library. Linux hosts still need the selected graphics loader and driver.
Explicit native-library path configuration remains available as an override.

## Snapshot publication

Snapshot versions end in `-SNAPSHOT` and publish from the exact commit that
passed the main CI workflow. A Linux x64 runner builds the Android publications,
reusing the matrix's CMake install archives to build only the JNI bridge and
final AARs, while macOS runners build the JVM, macOS, and iOS publications. Each
consumes native build artifacts produced by the platform and backend CI matrix.

A daily schedule drives publication rather than each push to `main`: the
workflow picks the latest successful CI run on `main` and publishes from its
artifacts. Each publishable component — the Kotlin modules, the native package
release, and the docs site — carries an input scope in `ci/snapshots.toml`, and
the workflow hashes that scope at the source commit. A component publishes when
its hash differs from the hash of the commit it last published from, so a change
confined to another binding leaves the Kotlin modules alone. That commit is
recorded as a floating `snapshot-state/<component>` tag, whose annotated message
carries the timestamp and run URL of the publish, and only components that
published successfully have their tag moved, so a failure is retried the next
day. `git ls-remote --tags origin 'refs/tags/snapshot-state/*'` reports where
every snapshot stands. Dispatch the workflow manually to publish sooner: `all`
applies the same gate immediately, and naming one component republishes it
whether or not its inputs changed.

The Central Portal namespace covering `org.maplibre.nativeffi` must be
registered with snapshot publishing enabled. The repository stores a Central
Portal user token in the `MAVEN_CENTRAL_USERNAME` and `MAVEN_CENTRAL_PASSWORD`
GitHub Actions secrets. Snapshot publication does not require signing.

Each host first stages its publications in a local Maven repository. CI merges
the Android and Apple repositories, then inspects the published AAR, JAR, and
KLIB payloads and compiles the example consumers against the merged repository.
After every local check succeeds, each host publishes its own target artifacts
to the Central Portal. Android and Apple leaf modules publish first; the
canonical multiplatform root modules publish last, after every leaf upload
succeeds. Snapshot workflows are serialized so two commits cannot interleave
those uploads. Publication jobs reuse the CI-produced native archives; they do
not rebuild MapLibre Native.

The initial snapshot workflow validates:

- Maven and Gradle module metadata for every publication;
- native archive presence in published KLIBs;
- JNI library placement and Rustls helper presence in Android AARs;
- Android runtime publications resolving without the Kotlin binding;
- native resource presence in JVM classifier JARs;
- Dokka-generated API pages in every API-bearing javadoc JAR;
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
