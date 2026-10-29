---
title: Releases and snapshots
description: Version format, release tags, and snapshot channels.
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
`mise run ci:release-version --next` prints the next one. The tag is signed and
annotated, and its message below the title line becomes the release notes. The
`release` agent skill drafts them.

## Snapshots

The Publish snapshots workflow publishes every snapshot channel from `main`
daily. Dispatch it to publish now, or with `docs_only` to republish only the
docs site.
