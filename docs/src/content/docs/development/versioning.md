---
title: Versioning and release tags
description: Calendar-based package versions, compatibility epochs, and Git tag forms for every release component.
sidebar:
  order: 5
---

This document outlines the intended versioning and release conventions. It does
not document the current status.

Tagged releases use a calendar-based version that retains one semantic
compatibility boundary:

```text
API_EPOCH.YYYYMM.REVISION
```

Each release component has its own version. This combines the compatibility
signal of [Semantic Versioning](https://semver.org/) with the temporal signal of
[Calendar Versioning](https://calver.org/).

## Version format

| Field       | Meaning                                                                     |
| ----------- | --------------------------------------------------------------------------- |
| `API_EPOCH` | Compatibility generation for the component. `0` identifies an unstable API. |
| `YYYYMM`    | UTC year and month in which the tagged release is published.                |
| `REVISION`  | Tagged release number within that month, starting at `0`.                   |

An `API_EPOCH` of `0` makes no compatibility promise. A new `YYYYMM` or
`REVISION` may require host changes.

An `API_EPOCH` of `1` or greater promises compatibility throughout that epoch. A
release that breaks a component's public API, ABI, or documented behavior
increments the epoch. Compatible releases retain the epoch, advance `YYYYMM`
when the month changes, and increment `REVISION` for each later release in the
same month.

```text
0.202608.0  unstable release
0.202608.1  later unstable revision; compatibility is not promised
0.202609.0  later unstable release; compatibility is not promised
1.202701.0  first stable compatibility epoch
1.202702.0  compatible release in a later month
1.202702.1  another compatible release in that month
2.202705.0  release with a breaking change
```

The version month matches the publication month. A release delayed across a UTC
month boundary takes the new month and starts its revision at `0`. Snapshots
carry intervening development builds and do not consume tagged release
revisions.

## Release components

Artifacts that form one consumable release family share a version. Components
otherwise advance independently. Kotlin is the first component published as a
tagged release. The other rows define the convention that each component will
use when its publishing workflow is added.

| Component | Release family                                                          | Source tag                    |
| --------- | ----------------------------------------------------------------------- | ----------------------------- |
| Core      | CPack archives for every native preset                                  | `core/v0.202608.0`            |
| Kotlin    | The multiplatform binding, target publications, and all runtime modules | `bindings/kotlin/v0.202608.0` |
| Rust      | The public crate and every published support crate                      | `bindings/rust/v0.202608.0`   |
| Python    | The Metal, OpenGL, and Vulkan distributions                             | `bindings/python/v0.202608.0` |
| .NET      | The managed binding and all backend runtime packages                    | `bindings/dotnet/v0.202608.0` |
| Dart      | The Dart package                                                        | `bindings/dart/v0.202608.0`   |
| Swift     | The Swift package                                                       | `bindings/swift/v0.202608.0`  |
| Go        | The Go module under `bindings/go`                                       | `bindings/go/v0.202608.0`     |
| Zig       | The Zig package                                                         | `bindings/zig/v0.202608.0`    |

Source tags follow the monorepo layout. `core/` names the native C distribution,
and `bindings/<name>/` names a language binding. Go consumes its source tag
directly, as required for a
[module in a repository subdirectory](https://go.dev/doc/modules/managing-source).
The Zig manifest version matches the release version; its minimum Zig compiler
version remains separate.

As Swift has no central registry, a Swift source tag publishes to a static
[Swift package registry](https://docs.swift.org/swiftpm/documentation/packagemanagerdocs/registryserverspecification/).
The release workflow generates the source archive, manifest response, release
metadata, checksum, and release index. It deploys those immutable files to a
static host that can set the registry's required response headers, such as
[Cloudflare Pages](https://developers.cloudflare.com/pages/configuration/headers/).
GitHub Pages cannot set those headers and therefore does not host the registry
directly. The registry omits the optional publication endpoint because the
release workflow publishes its files.

Release tags name the exact source commit and never move. Every artifact in a
release family is built from that commit and carries the version from its tag.
The Kotlin family follows the coordinate and finalization design in
[Kotlin publishing](/maplibre-native-ffi/development/kotlin-publishing/).
