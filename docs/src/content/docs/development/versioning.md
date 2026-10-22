---
title: Versioning and release tags
description:
  Calendar-based package versions, compatibility epochs, and the release tag.
sidebar:
  order: 5
---

Tagged releases use a calendar-based version that retains one semantic
compatibility boundary:

```text
API_EPOCH.YYYYMM.REVISION
```

One version covers the native library and every binding. The format combines the
compatibility signal of [Semantic Versioning](https://semver.org/) with the
temporal signal of [Calendar Versioning](https://calver.org/).

## Version format

| Field       | Meaning                                                      |
| ----------- | ------------------------------------------------------------ |
| `API_EPOCH` | Compatibility generation. `0` identifies an unstable API.    |
| `YYYYMM`    | UTC year and month in which the tagged release is published. |
| `REVISION`  | Tagged release number within that month, starting at `0`.    |

An `API_EPOCH` of `0` makes no compatibility promise. A new `YYYYMM` or
`REVISION` may require host changes.

An `API_EPOCH` of `1` or greater promises compatibility throughout that epoch. A
release that breaks the public API, ABI, or documented behavior of the library
or a binding increments the epoch. Compatible releases retain the epoch, advance
`YYYYMM` when the month changes, and increment `REVISION` for each later release
in the same month.

```text
0.202608.0  unstable release
0.202608.1  later unstable revision
0.202609.0  unstable release in a later month
1.202701.0  first stable compatibility epoch
2.202705.0  release with a breaking change
```

The version month matches the publication month. A release delayed across a UTC
month boundary takes the new month and starts its revision at `0`. Snapshots
carry intervening development builds and do not consume tagged release
revisions. Snapshots and tagged releases use the same packaging and artifact
validation.

## Release tags

A release has one tag, `vAPI_EPOCH.YYYYMM.REVISION`, which names the exact
source commit and never moves. Every artifact of the release is built from that
commit and carries the version from its tag. Each binding's generated code
matches the headers at that commit, so the native library and the bindings
release together.

The release workflow accepts a tag when its revision follows every earlier
release in the same epoch and month. The tag month must be the current UTC
month, and the tagged commit must belong to `main` and have a successful main CI
run. Before it publishes each package, the workflow checks again that the tag
names the same commit and version. Releases run one at a time, so a tag pushed
during a release waits for it.

Earlier releases tagged the native library as `core/v<version>` and each binding
as `bindings/<name>/v<version>`. Those versions count as earlier releases, so a
new version follows every version that they published.
