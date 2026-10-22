---
title: Kotlin publishing
description:
  Publish the Kotlin binding to Maven, and consume an unreleased build.
sidebar:
  order: 6
---

The Kotlin binding publishes an API module and one runtime module per render
backend; [Install](/maplibre-native-ffi/install/#kotlin) lists them. Snapshots
and releases publish as
[Releases and snapshots](/maplibre-native-ffi/development/versioning/)
describes.

Publishing needs the `org.maplibre.nativeffi` namespace registered on the
Central Portal with snapshots enabled, and a Central Portal user token in the
`MAVEN_CENTRAL_USERNAME` and `MAVEN_CENTRAL_PASSWORD` secrets.

## Consume an unreleased build

The `local` operation builds each named preset and publishes the modules that
those presets feed to `build/packages/kotlin/maven-local`:

```bash
mise run //:kotlin:publish local macos-arm64-metal macos-arm64-vulkan macos-arm64-egl
```

Each edit publishes at its own version, `0.1.0-local.<commit>` with a
`.dirty-<hash>` suffix for uncommitted changes. If you pin the version with
`MAPLIBRE_MAVEN_VERSION`, republish every preset that you use after each native
change. Each publish rewrites a backend's JVM runtime module and the Android
AARs with only that run's presets, so name every preset of a backend in one run.

A pull request that runs the `maven` CI unit uploads its verified repository as
the `kotlin-maven-verified-<sha>` artifact, at version
`0.1.0-pr<number>.<run id>`:

```bash
gh run download <run id> --repo maplibre/maplibre-native-ffi \
  --pattern 'kotlin-maven-verified-*' --dir maplibre-maven
```

Declare the repository as the only source of the group, because Gradle takes
each module from the first repository that holds it, and `mavenLocal()` can hold
stale modules:

```kotlin
repositories {
  exclusiveContent {
    forRepository { maven { url = uri("/path/to/maven-local") } }
    filter { includeGroup("org.maplibre.nativeffi") }
  }
  mavenCentral()
}
```
