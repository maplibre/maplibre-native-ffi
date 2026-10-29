<p align="center">
  <picture>
    <source
      media="(prefers-color-scheme: dark)"
      srcset="https://maplibre.org/img/maplibre-logos/maplibre-logo-for-dark-bg.svg"
    >
    <source
      media="(prefers-color-scheme: light)"
      srcset="https://maplibre.org/img/maplibre-logos/maplibre-logo-for-light-bg.svg"
    >
    <img
      alt="MapLibre Logo"
      src="https://maplibre.org/img/maplibre-logos/maplibre-logo-for-light-bg.svg"
      width="200"
    >
  </picture>
</p>

# MapLibre Native FFI

[![MapLibre](https://img.shields.io/badge/MapLibre-396CB2)](https://maplibre.org/)
[![Slack](https://img.shields.io/badge/Slack-4A154B?logo=slack&logoColor=white)](https://slack.openstreetmap.us/)
[![License](https://img.shields.io/github/license/sargunv/maplibre-native-ffi?label=License)](./LICENSE)

[![Maven Central](https://img.shields.io/maven-central/v/org.maplibre.nativeffi/maplibre-native-ffi)](https://central.sonatype.com/artifact/org.maplibre.nativeffi/maplibre-native-ffi)
[![NuGet](https://img.shields.io/nuget/v/Maplibre.NativeFfi)](https://www.nuget.org/packages/Maplibre.NativeFfi)
[![pub.dev](https://img.shields.io/pub/v/maplibre_native_ffi)](https://pub.dev/packages/maplibre_native_ffi)
[![PyPI](https://img.shields.io/pypi/v/maplibre-native-ffi)](https://pypi.org/project/maplibre-native-ffi/)
[![crates.io](https://img.shields.io/crates/v/maplibre-native-ffi)](https://crates.io/crates/maplibre-native-ffi)
[![npm](https://img.shields.io/npm/v/@maplibre/native-ffi)](https://www.npmjs.com/package/@maplibre/native-ffi)
[![OHPM](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fohpm.openharmony.cn%2Fohpm%2F%40maplibre%252Fnative-ffi-opengl&query=%24%5B%27dist-tags%27%5D.latest&label=OHPM&color=blue)](https://ohpm.openharmony.cn/#/en/detail/@maplibre%2Fnative-ffi-opengl)

[![Go Reference](https://pkg.go.dev/badge/github.com/maplibre/maplibre-native-ffi/bindings/go.svg)](https://pkg.go.dev/github.com/maplibre/maplibre-native-ffi/bindings/go)
[![Swift versions](https://img.shields.io/endpoint?url=https%3A%2F%2Fswiftpackageindex.com%2Fapi%2Fpackages%2Fmaplibre%2Fmaplibre-native-ffi-distribution%2Fbadge%3Ftype%3Dswift-versions)](https://swiftpackageindex.com/maplibre/maplibre-native-ffi-distribution)
[![Swift platforms](https://img.shields.io/endpoint?url=https%3A%2F%2Fswiftpackageindex.com%2Fapi%2Fpackages%2Fmaplibre%2Fmaplibre-native-ffi-distribution%2Fbadge%3Ftype%3Dplatforms)](https://swiftpackageindex.com/maplibre/maplibre-native-ffi-distribution)
[![vcpkg](https://img.shields.io/github/v/tag/maplibre/maplibre-native-ffi-distribution?label=vcpkg)](https://maplibre.org/maplibre-native-ffi/install/#vcpkg)
[![Conan](https://img.shields.io/github/v/tag/maplibre/maplibre-native-ffi-distribution?label=Conan)](https://maplibre.org/maplibre-native-ffi/install/#conan)
[![xmake](https://img.shields.io/github/v/tag/maplibre/maplibre-native-ffi-distribution?label=xmake)](https://maplibre.org/maplibre-native-ffi/install/#xmake)
[![C / Zig](https://img.shields.io/github/v/release/maplibre/maplibre-native-ffi?label=C%20%2F%20Zig)](https://github.com/maplibre/maplibre-native-ffi/releases/latest)

This project provides an experimental C API for
[MapLibre Native](https://github.com/maplibre/maplibre-native). It is built for
low-level language bindings and host integrations that need a C boundary instead
of direct C++ interop.

The API exposes MapLibre Native concepts directly. Framework concerns such as
gestures, widgets, declarative UI, and application lifecycle integration belong
in downstream adapters.

MapLibre Native FFI is pre-1.0. The C ABI is unstable while `mln_c_version()`
returns `0`.

## Documentation

Read the documentation site for concepts, usage guides, the generated C API
reference, and contributor notes.

- [Install](https://maplibre.org/maplibre-native-ffi/install/)
- [Concepts](https://maplibre.org/maplibre-native-ffi/concepts/)
- [Usage guides](https://maplibre.org/maplibre-native-ffi/guides/create-a-map/)
- [Reference](https://maplibre.org/maplibre-native-ffi/reference/c/)
- [Development overview](https://maplibre.org/maplibre-native-ffi/development/overview/)
- [Contributing](./CONTRIBUTING.md)

## Status

The C API covers most of the MapLibre Native API surface. Advanced extension
APIs, including custom layers and shader registry access, are not yet included.

The [Overview](https://maplibre.org/maplibre-native-ffi/#status) lists supported
platforms and backends.

## Sponsors

CI is provided by [Blacksmith](https://www.blacksmith.sh/) under its open source
program.

<a href="https://www.blacksmith.sh/">
  <img
    alt="CI powered by Blacksmith"
    src="https://github.com/user-attachments/assets/7d8455df-0b86-4676-9df8-fefe1f69ae4b"
    width="240"
  >
</a>

## License

BSD 3-Clause. See [LICENSE](LICENSE).

Copyright (c) 2026 MapLibre contributors.
