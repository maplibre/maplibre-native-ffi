# Specifications

These documents define what every implementation in a family must do so parallel
codebases stay aligned—starting with `*-map` example apps and, later, language
bindings. On the documentation site they appear under **Specifications** in the
sidebar; in the repository they are the `specs/` tree.

## Referencing a spec section in code

Cite a section in a trailing comment:

```text
// map-ex: frame-loop
```

Section slugs match the heading text (kebab-case). See
[map example spec](examples/map-example.md).

## Spec language

Requirement bullets use [RFC 2119](https://www.rfc-editor.org/info/rfc2119)
keywords (**MUST**, **SHOULD**, **MAY**). Explanatory prose outside bullet lists
is informative unless it repeats a keyword.

Some rules apply only under a specific capability or constraint (graphics API,
memory model, thread ownership). State those with **Applies when:** on the rule
or in a short subsection.
