# Repository guidance

MapLibre Native FFI is a C API for MapLibre Native, implemented by mln-rs, its
Rust port. A generator writes bindings for eight languages from the C headers.
Each binding also has a handwritten runtime. Mise provides every tool and task.
Run `mise tasks --all` to list the tasks.

Read these when the task needs them:

- [Guides](docs/src/content/docs/guides/): how to use the library, starting with
  [Create a map](docs/src/content/docs/guides/create-a-map.mdx).
- [Overview](docs/src/content/docs/development/overview.mdx): setup, everyday
  commands, other targets such as Windows under Wine, engine patches, CI, and
  releases.
- [Guidelines](docs/src/content/docs/development/guidelines.mdx): read before
  you change the public API or documentation.

## Priorities

The project is prerelease. Make breaking changes instead of adding compatibility
shims.

Bindings are low level. Each one mirrors the C API and the other bindings. When
these goals conflict, safety comes first, then similarity, then language idioms.
Add a convenience API to a binding only when its language's safety or ergonomics
require one. Don't repeat validation that native code already does.

The library is [mln-rs](https://github.com/louwers/mln-rs), the Rust port of
MapLibre Native, at the `third_party/mln-rs` submodule's pinned commit with the
patches in `patches/mln-rs/` applied. Its `crates/mln_c` crate implements the C
API, and the native build compiles it with Cargo. Answer questions about library
behavior from that source. `include/` stays the source of truth for the C API:
configuring a preset fails while mln-rs's copy in `crates/mln_c/include` differs
from it. If the submodule is empty, run `mise deps` to check it out and apply
the patches. Every `mise run`, `mise exec`, and commit hook does the same, which
discards submodule edits that no patch holds. Save a change to mln-rs to its
patch before you run any of them.

After a rebase, run `mise deps` before you stage anything.
`git submodule update` skips this submodule. Before you push, check
`git diff origin/main...HEAD --stat`. If `git add -A` staged a stale checkout,
the diff reverts the mln-rs pin.

## Tests

Test native behavior once, in C, under `tests/native`. A patch to mln-rs is the
exception: its tests go in mln-rs's own tests. A case that mln-rs doesn't pass
yet goes in the backend's list under `tests/native/expected-failures/`, with why
it fails. A binding's suite tests only what the binding adds: its runtime, its
generated code, and its platform integration.

Test behavior that callers can observe, not implementation details. Add a test
only when it would catch a regression in that behavior. Don't test what the
compiler, the generator, or another test already checks. Keep tests fast and
deterministic: wait for events instead of sleeping, and don't depend on timing
or test order. A binding test doesn't assert that an object was
garbage-collected, finalized, or deallocated. Scratch checks that you ran while
working don't need to be committed.

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
