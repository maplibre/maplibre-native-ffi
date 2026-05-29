# Specifications

These documents define what every implementation in a family must do so parallel
codebases stay aligned—starting with `*-map` example apps and, later, language
bindings. On the documentation site they appear under Specifications in the
sidebar; in the repository they are the `specs/` tree.

## Referencing a spec section in code

Cite the spec file and heading fragment (slug), not a bare label:

```text
// map-example.md#frame-loop
```

Use the spec basename (`map-example.md`, and later e.g. `bindings.md`) plus `#`
and the section slug. Slugs are kebab-case from the heading text—the same
fragment as in `[Frame loop](map-example.md#frame-loop)` links inside the spec.
Resolve the file from `specs/` (or `specs/examples/` for map-example) when
opening it; published URLs can come later.

For a subsection, use one fragment (`#owned-texture`) or `#render-target-modes`
on the parent section—whichever you are implementing.

## Spec language

Requirement bullets use [RFC 2119](https://www.rfc-editor.org/info/rfc2119)
keywords (**MUST**, **SHOULD**, **MAY**). Explanatory prose outside bullet lists
is informative unless it repeats a keyword.

Some rules apply only under a specific capability or constraint (graphics API,
memory model, thread ownership). State those with Applies when: on the rule or
in a short subsection.
