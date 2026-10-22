# Repository guidance

MapLibre Native FFI is a C API for MapLibre Native. A generator writes low-level
bindings for nine languages from its headers, over a handwritten runtime in each
binding. Mise provides every tool and task, and `mise tasks --all` lists them.

## Priorities

The project is prerelease. Make breaking changes rather than keep compatibility
shims.

Bindings stay low level and analogous to the C API and to each other: safety
first, then similarity, then idioms. A binding adds a convenience API only when
its language's safety or ergonomics strongly require one. It never repeats
validation that native already does.

MapLibre behavior is whatever the pinned `third_party/maplibre-native` submodule
does with the patches in `patches/maplibre-native/`. Answer questions about
engine behavior from that source; `mise deps` checks it out and applies the
patches if it is empty.

## Tests

Native behavior is tested once, in C, under `tests/native`. A binding suite
tests only what its binding adds: its runtime, its generated shapes, and its
platform integration. Before you delete a binding test, check that the C suite
covers its lines, as
[Code coverage](docs/src/content/docs/development/overview.md#code-coverage)
describes.

GPU objects in tests come from `tests/graphics`. The target or the build decides
a skip, never the environment, so fix the CI environment rather than skip a
test.

## Documentation

The docs site introduces concepts and tasks at a high level. Contracts belong in
doc comments: the C headers and each binding's package docs. No doc narrates how
the code is implemented; a comment inside the code explains a non-obvious choice
where it is made. The site's Development section holds only what maintainers
need and the code cannot say:

- [Overview](docs/src/content/docs/development/overview.md): machine setup,
  cross-compilation, debugging on a CI runner, and code coverage.
- [C API guidelines](docs/src/content/docs/development/c-api-guidelines.md):
  read before you change a C declaration.
- [Releases and snapshots](docs/src/content/docs/development/versioning.md):
  versions, tags, and snapshot publishing.
- [Kotlin publishing](docs/src/content/docs/development/kotlin-publishing.md):
  consume an unreleased Kotlin build.

## Issues and pull requests

[Rubberstamp](https://github.com/apps/rubberstamp-bot) labels each new issue and
pull request from `.config/rubberstamp.yml`. Don't label by hand; correct a
wrong label instead, and leave GitHub's native issue type unset.

Write a PR title and description for a reviewer who has not seen your session:

- Describe the final diff. Rewrite both when review changes the scope.
- Open with the user-visible problem and what users can now do. For internal
  work, give the concrete benefit to maintainers instead.
- Make it concrete: a small example of new API or commands, the trigger and the
  before and after of a bug, and measured numbers with their method.
- State what a host must adopt: new options, defaults, platforms, and breaking
  changes.
- Report validation that CI does not show, and its limits.

When you post AI-contributed GitHub content, including a new Discussion or
Issue, a reply, or a PR description or comment, append this disclosure:
`*AI-assisted — Tool: <tool>; model: <provider>/<model>*`

If Rubberstamp's CI plan skips coverage that a change needs, run every unit with
`gh workflow run CI --ref <branch>`.
