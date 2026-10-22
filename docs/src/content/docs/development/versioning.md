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

pub.dev enables automated publishing only for a package that exists, so publish
the Dart package's first version by hand: run
`mise run //bindings/dart:prepare-release <tag>` for a published release tag,
then `dart pub publish` from `bindings/dart`. On the package's admin page,
enable publishing from GitHub Actions for `maplibre/maplibre-native-ffi`, with
the tag pattern v{{version}}, as pub.dev writes it, and the `pub.dev`
environment.

## Snapshots

The Publish snapshots workflow runs daily from the artifacts of the latest
successful CI run on `main`. A component publishes when its inputs in
`ci/snapshots.toml` changed since the commit that its
`snapshot-state/<component>` tag names, and a successful publish moves the tag.
Dispatch the workflow manually and name a component to republish it now.

The TypeScript binding has no snapshot channel. The other channels keep only the
latest snapshot, and npm keeps every version that it publishes, so the binding
publishes nothing until its first tagged release.
