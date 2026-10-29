---
name: release
description:
  Use when asked to prepare or tag a release, or to write or rewrite a release's
  notes.
---

# Release

A release runs from the previous `v[0-9]*` tag to a `main` commit whose CI
passed; `mise run ci:release-version --next` names it.

## Find what changed

PR titles and descriptions show where to look, but confirm each change in the
code at the two endpoints, since a description can be stale.

- The C API: diff `include/maplibre_native_c/`, and read the implementation
  where a header doesn't say how behavior changed.
- Each binding: diff its public API and its package manifest, generated and
  handwritten.
- Packaging and platforms: diff the release workflows and `tools/release/`,
  `CMakePresets.json`, and the support matrix that
  `docs/scripts/generate-support-matrix.py` prints at each endpoint.
- The engine: the `third_party/maplibre-native` pin and
  `patches/maplibre-native/`.

Report the net change: something added and reworked within the range appears
only in its final form, or not at all. Cover only the artifacts that the release
publishes.

## Write

- Only what users observe. No tests, CI, refactors, or maintainer tooling.
- Organize by what changed, not by PR, and link no PRs.
- Open with one or two sentences on the release's theme, then use whatever
  sections this release needs.
- For a breaking change, say what a host must change. Merge changes that share a
  pattern into one line with an example; the compiler and API reference give the
  details.
- Use plain language, one line per change where possible. No title line and no
  links footer; CI appends the footer.

Revise the draft with the maintainer until they approve it.

## Publish

Notes travel in the signed tag. Its message is `MapLibre Native FFI <version>`,
a blank line, then the notes, and the publish run copies everything below the
title into the release body. End the file with a newline, or the signature fuses
onto the last line:

```sh
git tag -s --cleanup=verbatim -F <file> v<version> <commit>
```

Push it only when the maintainer says to. For a release that is already
published, replace the body above its links footer instead, and leave the tag.
