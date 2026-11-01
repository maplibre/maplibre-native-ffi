# Repository guidance

MapLibre Native FFI is a C API for MapLibre Native. A generator writes bindings
for nine languages from the C headers. Each binding also has a handwritten
runtime. Mise provides every tool and task. Run `mise tasks --all` to list the
tasks.

Read these when the task needs them:

- [Concepts](docs/src/content/docs/concepts.md): how the library works.
- [Overview](docs/src/content/docs/development/overview.md): setup, everyday
  commands, other targets such as Windows under Wine, engine patches, CI, and
  releases.
- [Guidelines](docs/src/content/docs/development/guidelines.md): read before you
  change the public API or documentation.

## Priorities

The project is prerelease. Make breaking changes instead of adding compatibility
shims.

Bindings are low level. Each one mirrors the C API and the other bindings. When
these goals conflict, safety comes first, then similarity, then language idioms.
Add a convenience API to a binding only when its language's safety or ergonomics
require one. Don't repeat validation that native code already does.

The engine is the `third_party/maplibre-native` submodule at its pinned commit,
with the patches in `patches/maplibre-native/` applied. Answer questions about
engine behavior from that source. If the submodule is empty, run `mise deps` to
check it out and apply the patches.

After a rebase, run `mise deps` before you stage anything.
`git submodule update` skips this submodule. Before you push, check
`git diff origin/main...HEAD --stat`. If `git add -A` staged a stale checkout,
the diff reverts the engine pin.

## Tests

Test native behavior once, in C, under `tests/native`. An engine patch is the
exception: its tests go in upstream's `test/` tree. A binding's suite tests only
what the binding adds: its runtime, its generated code, and its platform
integration.

Test behavior that callers can observe, not implementation details. Add a test
only when it would catch a regression in that behavior. Don't test what the
compiler, the generator, or another test already checks. Keep tests fast and
deterministic: wait for events instead of sleeping, and don't depend on timing
or test order. Scratch checks that you ran while working don't need to be
committed.

Only the target or the build decides whether a test is skipped, never the
environment. If a test fails in CI because of the environment, fix the
environment instead of skipping the test.

## Issues and pull requests

[Rubberstamp](https://github.com/apps/rubberstamp-bot) labels each new issue and
pull request, using the rules in `.config/rubberstamp.yml`. Don't add labels by
hand. If a label is wrong, correct it. Leave GitHub's issue type unset.

PR titles follow Conventional Commits, because the title becomes the
squash-merge commit message. Mark a title as breaking only when the change
breaks a user-facing contract. Changes to tooling, tests, or CI are never
breaking.

Write a short title and description for a reviewer who will read the diff. Say
what the diff, its doc comments, and CI can't:

- Open with the problem and what users can now do, or what maintainers gain.
- Show one small example of new API, or a bug's trigger with before and after.
- For a breaking change, say what users must change.
- List checks that CI can't run. Leave out the tests, builds, and checks that CI
  runs, even if you also ran them locally.
- Rewrite both when review changes the scope.

Append this disclosure to any AI-assisted content you post on GitHub, such as an
issue, a discussion, a reply, or a PR description or comment:
`*AI-assisted — Tool: <tool>; model: <provider>/<model>*`

Rubberstamp also picks which CI units a pull request runs. If it skips a unit
that the change needs, ask before you start a manual CI run.
