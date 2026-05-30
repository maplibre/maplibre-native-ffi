---
title: Specifications
description: Normative specs for parallel implementations (examples, bindings).
sidebar:
  order: 4
---

These documents define what every implementation in a family must do so parallel
codebases stay aligned—starting with `*-map` example apps and, later, language
bindings.

- [Map example](/maplibre-native-ffi/development/specifications/map-example/)

## Referencing a spec section in code

Cite the spec file and heading fragment (slug), not a bare label:

```text
// map-example.md#frame-loop
```

Use the spec basename (`map-example.md`, and later e.g. `bindings.md`) plus `#`
and the section slug. Slugs are kebab-case from the heading text—the same
fragment as in `[Frame loop](map-example#frame-loop)` links inside the spec.
Source files live in `docs/src/content/docs/development/specifications/` on this
site.

For a subsection, use one fragment (`#owned-texture`) or `#render-target-modes`
on the parent section—whichever you are implementing.

## Spec language

Requirement bullets use [RFC 2119](https://www.rfc-editor.org/info/rfc2119)
keywords (**MUST**, **SHOULD**, **MAY**). Explanatory prose outside bullet lists
is informative unless it repeats a keyword.

Some rules apply only under a specific capability or constraint (graphics API,
memory model, thread ownership). State those with Applies when: on the rule or
in a short subsection.
