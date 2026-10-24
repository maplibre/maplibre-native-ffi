---
title: Releases and snapshots
description: Version format, release tags, registry setup, and snapshots.
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

A release has one tag, `vAPI_EPOCH.YYYYMM.REVISION`, on a commit on `main`.
`mise run ci:release-version --next` prints the next one.

pub.dev, crates.io, and npm enable automated publishing only for a package that
exists, so publish the first version of each package by hand, from a clean
checkout of a published release tag or from the release run.

- Dart: run `mise run //bindings/dart:prepare-release <tag>`, then
  `dart pub publish` from `bindings/dart`. On the package's admin page, enable
  publishing from GitHub Actions for `maplibre/maplibre-native-ffi`, with the
  tag pattern v{{version}} and the `pub.dev` environment.
- Rust: run `mise run //bindings/rust:prepare-release <tag>`, then
  `cargo publish --allow-dirty -p <crate>` for `maplibre-native-ffi-sys`,
  `maplibre-native-ffi-core`, and `maplibre-native-ffi`, in that order. In each
  crate's settings, add a trusted publisher for `maplibre/maplibre-native-ffi`,
  with the workflow `release.yml` and the `crates.io` environment.
- TypeScript: the release run's TypeScript job uploads the packed packages as
  the `npm-packages-<version>` artifact before it publishes. Download it and run
  `npm publish --access public <tarball>` for each addon package, then each
  backend package, then `@maplibre/native-ffi`. In each package's settings, add
  a trusted publisher for `maplibre/maplibre-native-ffi`, with the workflow
  `release.yml` and the `npm` environment.

Maven Central needs the `org.maplibre.nativeffi` namespace registered on the
Central Portal, with snapshots enabled, and a Central Portal user token in the
`MAVEN_CENTRAL_USERNAME` and `MAVEN_CENTRAL_PASSWORD` secrets. `GPG_PRIVATE_KEY`
and `GPG_PASSPHRASE` hold the key that signs every release and snapshot file.

NuGet.org trusted publishing can create a package, so .NET needs no first
publish by hand. Add a trusted publishing policy for
`maplibre/maplibre-native-ffi`, with the workflow `_dotnet-nuget.yml` and the
`nuget.org` environment, and set the repository variable `NUGET_USER` to the
NuGet.org account that owns the policy.

PyPI lets a pending trusted publisher create a project, so Python needs no first
publish by hand. Add a trusted publisher for each distribution, for
`maplibre/maplibre-native-ffi`, with the workflow `release.yml` and its own
environment:

| Distribution                         | Environment           |
| ------------------------------------ | --------------------- |
| `maplibre-native-ffi`                | `pypi`                |
| `maplibre-native-ffi-runtime-opengl` | `pypi-runtime-opengl` |
| `maplibre-native-ffi-runtime-metal`  | `pypi-runtime-metal`  |
| `maplibre-native-ffi-runtime-vulkan` | `pypi-runtime-vulkan` |

## Snapshots

The Publish snapshots workflow publishes every snapshot channel from `main`
daily. Dispatch it to publish now, or with `docs_only` to republish only the
docs site.
