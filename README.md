<p align="center">
  <a href="https://maplibre.org/">
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
  </a>
</p>

# MapLibre Native FFI

[![Release](https://img.shields.io/github/v/release/maplibre/maplibre-native-ffi?label=Release)](https://github.com/maplibre/maplibre-native-ffi/releases/latest)
[![License](https://img.shields.io/github/license/maplibre/maplibre-native-ffi?label=License)](./LICENSE)
[![Documentation](https://img.shields.io/badge/Documentation-blue?logo=astro&logoColor=white)](https://maplibre.org/maplibre-native-ffi/)
[![Slack](https://img.shields.io/badge/Slack-4A154B?logo=slack&logoColor=white)](https://slack.openstreetmap.us/)

[![C](https://img.shields.io/badge/C-vcpkg%20%C2%B7%20Conan%20%C2%B7%20xmake-00599C?logo=c&logoColor=white)](https://github.com/maplibre/maplibre-native-ffi/releases/latest)
[![Dart](https://img.shields.io/badge/Dart-pub.dev-0175C2?logo=dart&logoColor=white)](https://pub.dev/packages/maplibre_native_ffi)
[![.NET](https://img.shields.io/badge/.NET-NuGet-512BD4?logo=dotnet&logoColor=white)](https://www.nuget.org/packages/Maplibre.NativeFfi)
[![Go](https://img.shields.io/badge/Go-pkg.go.dev-00ADD8?logo=go&logoColor=white)](https://pkg.go.dev/github.com/maplibre/maplibre-native-ffi/bindings/go)
[![Kotlin](https://img.shields.io/badge/Kotlin-Maven%20Central-7F52FF?logo=kotlin&logoColor=white)](https://central.sonatype.com/artifact/org.maplibre.nativeffi/maplibre-native-ffi)
[![Python](https://img.shields.io/badge/Python-PyPI-3776AB?logo=python&logoColor=white)](https://pypi.org/project/maplibre-native-ffi/)
[![Rust](https://img.shields.io/badge/Rust-crates.io-000000?logo=rust&logoColor=white)](https://crates.io/crates/maplibre-native-ffi)
[![Swift](https://img.shields.io/badge/Swift-Swift%20Package%20Index-F05138?logo=swift&logoColor=white)](https://swiftpackageindex.com/maplibre/maplibre-native-ffi-distribution)
[![TypeScript](https://img.shields.io/badge/TypeScript-npm-3178C6?logo=typescript&logoColor=white)](https://www.npmjs.com/package/@maplibre/native-ffi)
[![ArkTS](https://img.shields.io/badge/ArkTS-OHPM-0A59F7?logo=harmonyos&logoColor=white)](https://ohpm.openharmony.cn/#/en/detail/@maplibre%2Fnative-ffi-opengl)
[![Zig](https://img.shields.io/badge/Zig-GitHub%20Releases-F7A41D?logo=zig&logoColor=white)](https://github.com/maplibre/maplibre-native-ffi/releases/latest)

MapLibre Native FFI is the
[MapLibre Native](https://github.com/maplibre/maplibre-native) map engine as a C
API, with bindings for Dart, Go, Kotlin, .NET, Python, Rust, Swift, TypeScript,
and Zig. Use it to build map components for a UI framework, or to render maps on
a server.

To start, read the
[concepts](https://maplibre.org/maplibre-native-ffi/concepts/), then
[install](https://maplibre.org/maplibre-native-ffi/install/) the package for
your language.

## Used by

- [MapLibre Compose](https://github.com/maplibre/maplibre-compose) brings maps
  to Compose Multiplatform.
- [MapLibre for Flutter](https://github.com/josxha/flutter-maplibre) brings maps
  to Flutter.

## Status

Until the first part of the version reaches 1, any release can break the API,
the ABI, or documented behavior. The
[support matrix](https://maplibre.org/maplibre-native-ffi/#status) shows which
platforms, architectures, and backends each language supports.

## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md).

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

BSD 2-Clause. See [LICENSE](./LICENSE).
