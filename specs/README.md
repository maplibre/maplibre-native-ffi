# Specifications

`specs/` holds normative specs for implementation families in this repo (for
example `*-map` example apps and language bindings). Each spec defines what
every implementation in that family must do so parallel codebases stay aligned.

## Referencing a spec section in code

Each spec section defines a stable anchor ID: `{#map-ex-<slug>}` in the Markdown
heading (see [`examples/map-example.md`](examples/map-example.md)).

In source, cite the ID in a trailing comment on the owning module, type, or the
smallest enclosing unit that implements the obligation:

```text
// map-ex: frame-loop
// map-ex: render-targets / owned-texture
```

Rules:

- Use the `map-ex:` prefix for the [map example spec](examples/map-example.md).
  Future specs get their own prefix (`map-bind:`, etc.).
- Prefer one comment per file for the module’s primary role
  (`map-ex: modules /
  map-state`), plus extra comments on code that implements
  a distinct subsection (for example a compositor or a single render-target
  variant).
- Slugs use kebab-case and mirror the heading anchor after the `map-ex-` prefix.
  Nested topics use `parent-slug / child-slug` in comments when helpful; anchors
  stay flat (`map-ex-render-targets-owned-texture`).
- Link to the file path in review discussion:
  `specs/examples/map-example.md#map-ex-frame-loop`.

## Normative language

Requirement bullets use [RFC 2119](https://www.rfc-editor.org/info/rfc2119)
keywords (**MUST**, **SHOULD**, **MAY**). Explanatory prose outside bullet lists
is informative unless it repeats a keyword.

## Profiles

When a host platform or language class needs different obligations, the spec
states them in an inline **Profile** subsection. Extract a separate profile
document only if that subsection grows unwieldy.

## Change control

Specs have no version field. `main` is always current; propose changes through
pull request like any other source file.
