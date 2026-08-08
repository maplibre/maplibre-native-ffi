---
title: Versioning and release tags
description: Calendar-based package versions, compatibility epochs, and monorepo release tags.
sidebar:
  order: 5
---

Tagged releases use a calendar-based version that retains one semantic
compatibility boundary:

```text
API_EPOCH.YYYYMM.REVISION
```

Each release family has its own version. The format combines the compatibility
signal of [Semantic Versioning](https://semver.org/) with the temporal signal of
[Calendar Versioning](https://calver.org/).

## Version format

| Field       | Meaning                                                                  |
| ----------- | ------------------------------------------------------------------------ |
| `API_EPOCH` | Compatibility generation for the family. `0` identifies an unstable API. |
| `YYYYMM`    | UTC year and month in which the tagged release is published.             |
| `REVISION`  | Tagged release number within that month, starting at `0`.                |

An `API_EPOCH` of `0` makes no compatibility promise. A new `YYYYMM` or
`REVISION` may require host changes.

An `API_EPOCH` of `1` or greater promises compatibility throughout that epoch. A
release that breaks a family's public API, ABI, or documented behavior
increments the epoch. Compatible releases retain the epoch, advance `YYYYMM`
when the month changes, and increment `REVISION` for each later release in the
same month.

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
revisions. Each family's snapshots and tagged releases use the same packaging
and artifact validation.

## Release families and tags

Artifacts that form one consumable release family share a version. Components
otherwise advance independently. A binding's target and runtime packages belong
to the same family when consumers use them as one binding release.

Source tags follow the monorepo layout:

| Release family | Source tag                                   |
| -------------- | -------------------------------------------- |
| Core           | `core/vAPI_EPOCH.YYYYMM.REVISION`            |
| A binding      | `bindings/<name>/vAPI_EPOCH.YYYYMM.REVISION` |

Release tags name the exact source commit and never move. Every artifact in a
release family is built from that commit and carries the version from its tag.

The release workflows accept the latest tag in a release family when its
revision follows every earlier tag in the same epoch and month. The tag month
must be the current UTC month, and the tagged commit must belong to `main` and
have a successful main CI run. A release workflow repeats the tag checks before
it publishes the release family.
