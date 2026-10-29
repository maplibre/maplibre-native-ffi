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
daily. Dispatch it to publish now.

## Documentation hosting

GitHub Pages serves the documentation from the latest release at
[maplibre.org/maplibre-native-ffi/](https://maplibre.org/maplibre-native-ffi/).
Set the repository's Pages source to GitHub Actions and allow release tags in
the `github-pages` environment's deployment rules.

Cloudflare Pages serves previews of `main` and pull requests. Create a Direct
Upload project named `maplibre-native-ffi`, with `main` as its production
branch. Store `CLOUDFLARE_ACCOUNT_ID` and `CLOUDFLARE_API_TOKEN` in this
repository's `cloudflare-pages` environment. The token needs Account /
Cloudflare Pages / Edit permission for that account. Allow `main` and pull
request merge refs in the environment's deployment rules. Fork and Dependabot
pull requests build the site without deploying it.

The same token can also authorize R2 cache writes when it has Object Read &
Write access to the `maplibre-native-ffi-sccache` bucket. Store its original
value as `CLOUDFLARE_API_TOKEN` in the `sccache` environment too, and its token
ID as `R2_SCCACHE_ACCESS_KEY_ID`. CI derives the S3 secret from the token, so
rotating it requires updating the original value in both environments.

Each preview's URL appears in the CI deployment and job summary. Pull requests
use the `pr-<number>` branch alias; `main` updates the project's production URL.
Cloudflare retains deployments after a pull request closes.

To deploy a built preview locally, authenticate with
`mise -C docs exec --no-deps -- wrangler login --device`, or set the same
Cloudflare environment variables. Run `mise run //docs:package-cloudflare`, then
`mise run //docs:deploy-cloudflare <branch>`. Mise installs the pinned Wrangler
CLI for the deployment task.
