---
title: Releases and snapshots
description: Version format, the release tag, and snapshot publication.
sidebar:
  order: 5
---

## Version format

One version covers the native library and every binding:
`API_EPOCH.YYYYMM.REVISION`.

| Field       | Meaning                                                  |
| ----------- | -------------------------------------------------------- |
| `API_EPOCH` | Compatibility generation; `0` promises no compatibility  |
| `YYYYMM`    | UTC year and month of publication                        |
| `REVISION`  | Tagged release number within that month, starting at `0` |

An epoch of `1` or greater promises compatibility throughout the epoch. A
release that breaks the public API, ABI, or documented behavior of the library
or a binding increments it.

## Release tags

A release has one tag, `vAPI_EPOCH.YYYYMM.REVISION`, on a commit on `main` that
has passed CI. Every artifact of the release builds from that commit. The
release workflow rejects a tag out of order or from another month.

pub.dev and crates.io enable automated publishing only for a package that
exists, so publish the first version of each package by hand, from a clean
checkout of a published release tag.

- Dart: run `mise run //bindings/dart:prepare-release <tag>`, then
  `dart pub publish` from `bindings/dart`. On the package's admin page, enable
  publishing from GitHub Actions for `maplibre/maplibre-native-ffi`, with the
  tag pattern v{{version}}, as pub.dev writes it, and the `pub.dev` environment.
- Rust: run `mise run //bindings/rust:prepare-release <tag>`, then
  `cargo publish --allow-dirty -p <crate>` for `maplibre-native-ffi-sys`,
  `maplibre-native-ffi-core`, and `maplibre-native-ffi`, in that order. In each
  crate's settings, add a trusted publisher for `maplibre/maplibre-native-ffi`,
  with the workflow `release.yml` and the `crates.io` environment.

## Snapshots

The Publish snapshots workflow runs daily from the artifacts of the latest
successful CI run on `main`. A component publishes when its inputs in
`ci/snapshots.toml` changed since the commit that its
`snapshot-state/<component>` tag names, and a successful publish moves the tag.
Dispatch the workflow manually and name a component to republish it now.

pub.dev, crates.io, and npm keep every version that they publish, so the Dart,
Rust, and TypeScript bindings have no snapshot channel. The other channels keep
only the latest snapshot.
