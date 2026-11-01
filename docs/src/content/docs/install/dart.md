---
title: Dart
description: Add the Dart package to a project.
---

## Install

Add the package from [pub.dev](https://pub.dev/packages/maplibre_native_ffi). It
needs Dart 3.11 or later.

```bash frame="terminal"
dart pub add maplibre_native_ffi
```

When it builds, the package downloads the C library for your target.

## Choose a backend

The package uses your platform's
[default backend](/maplibre-native-ffi/install/#choose-a-backend). To choose
another, map an operating system to a backend in your app's pubspec:

```yaml title="pubspec.yaml"
hooks:
  user_defines:
    maplibre_native_ffi:
      backend:
        android: vulkan
```

The operating systems are `android`, `ios`, `linux`, `macos`, and `windows`. The
backends are `opengl`, `metal`, and `vulkan`.

## Android

Initialize the library once per process, before you create a runtime, with the
calling thread's JNI environment and an Android `Context`:

```dart title="lib/main.dart"
initAndroid(NativePointer(jniEnv), NativePointer(context));
```

Get the JNI environment from [`package:jni`](https://pub.dev/packages/jni), and
the application context from
[`package:jni_flutter`](https://pub.dev/packages/jni_flutter).

## Snapshots

The package from pub.dev downloads the C library of its own release. A Git
dependency has no release to download from, so a snapshot takes two steps.
First, depend on the package from Git at the `unstable-native-snapshot` tag,
which points at the commit that the snapshot was built from:

```yaml title="pubspec.yaml"
dependencies:
  maplibre_native_ffi:
    git:
      url: https://github.com/maplibre/maplibre-native-ffi
      path: bindings/dart
      ref: unstable-native-snapshot
```

Then download the C library archive for your target from the
[snapshot release](https://github.com/maplibre/maplibre-native-ffi/releases/tag/unstable-native-snapshot),
unpack it, and [use it as your own build](#use-your-own-build).

## Use your own build

To link a C library that you built or downloaded, name it in `local_build`. Name
an install prefix, or a directory with prefixes one level down, such as a CMake
build root. A relative path is relative to the pubspec. A Git dependency on the
package always needs one.

```yaml title="pubspec.yaml"
hooks:
  user_defines:
    maplibre_native_ffi:
      local_build: ../maplibre-native-ffi/build
```

## Download from a mirror

If your builds can't reach GitHub, copy the release's archives to a server of
your own, in a directory named after the release tag, and name the server's base
URL in `release_url`. The package still checks each archive against the digest
that it pinned at release.

```yaml title="pubspec.yaml"
hooks:
  user_defines:
    maplibre_native_ffi:
      release_url: https://mirror.example.com/maplibre-native-ffi
```
