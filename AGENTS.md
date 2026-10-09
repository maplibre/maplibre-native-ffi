# MapLibre Native FFI

A C API for MapLibre Native, and low-level language bindings generated from its
headers.

## Project map

```text
include/                  Public C headers, the ABI surface
  binding-interfaces.toml The headers that the binding generator reads
src/                      C++ implementation and render backends
  testing/                Unexported seams for the internal test suite
tools/bindgen/            Generator: C headers to binding code (see README.md)
bindings/<language>/      Kotlin, Rust, Swift, Zig, .NET, Python, Go, Dart:
                          generated code over a handwritten runtime
tests/
  native/                 C suites for native behavior (see README.md)
  bindgen/                Generator tests and protocol fixtures
  conformance/            Cases that every binding suite maps to a test
  graphics/               GPU fixtures for every test suite (see README.md)
examples/                 One map app per binding and toolkit (see README.md)
docs/                     Astro/Starlight site; development/ is for maintainers
ci/                       Workflow generator, coverage planning, release tools
cmake/, CMakePresets.json Native build and the target/backend presets
third_party/              The MapLibre Native submodule
```

## Dev tool commands

Mise provides every tool and task. Run anything else under `mise exec --`.
`mise tasks --all` lists every task, and
[the development overview](docs/src/content/docs/development/overview.md) covers
platform setup and cross-compilation SDKs.

```bash
mise bootstrap --yes                   # Tools, packages, hooks (Windows: --skip files)
mise run build [preset]                # Build and install the native library
mise run test [preset]                 # Build, then run the native C suites
mise run //bindings/<language>:test    # Run one binding suite
mise run bindings:generate             # Regenerate every binding from the headers
mise run bindings:check                # Fail on stale or incomplete generated code
mise run bindings:test-generator       # Run the generator tests
mise run //examples/<example>:smoke    # Render one frame headless and exit
mise run //docs:build                  # Build the documentation site
mise run fix                           # Format and lint every file
hk fix <files>                         # Format and lint some files
```

The default preset is the host's; `CMakePresets.json` lists the others. Clangd
uses the compilation database of the last `mise run build <preset>`. The
pre-commit hook runs the formatters and linters, and `fix` stages what it
changes.

## Project invariants

### Design

- The project is prerelease. Make breaking changes rather than keep
  compatibility shims.
- Bindings stay low level and analogous to the C API and to each other. They
  expose MapLibre Native concepts directly, and follow their language's
  conventions for memory and thread safety. Prioritize safety, then similarity,
  then idioms.
- A binding adds a redundant API or convenience helper only when its language's
  safety or ergonomics strongly require one.
- Bindings never reimplement native validation. They validate only what they
  own: API shape, state, lifetimes, and memory safety.
- Generated binding code is never edited by hand. Change the header, the
  generator, or the binding runtime, then run `mise run bindings:generate`.
- Mise defines tools and workflows, CMake presets define native builds, and
  Gradle defines Android builds.
- Leave anything you touch tidier than you found it.

### Testing

- Native behavior is tested once, in C, under `tests/native`. Each binding suite
  tests only what its binding adds: its handwritten runtime, its generated
  shapes, and its platform integration.
- Each binding maps every case in `tests/conformance/cases.toml` to a test or to
  a reason that it does not apply. `scripts/check-conformance.py --strict`
  enforces the mapping.
- The ABI suite calls every exported function by name.
  `mise run check-export-calls` enforces it, and `tests/uncalled-exports.txt`
  only shrinks.
- Use `tests/native/internal` and the `src/testing` seams only for orderings
  that no public signal reaches. The seams stay unexported, which
  `mise run check-exports` enforces.
- Generator tests assert on semantic plans, coverage reports, or generated code
  that they compile and run, never on fragments of emitted source.
- Tests wait on signals, never on elapsed time, and serve every request from a
  local fixture. `scripts/check-test-hygiene.py` enforces this against a
  baseline that only shrinks.
- GPU contexts, textures, and surfaces in tests come from `tests/graphics`.
- The target or the build decides a skip, never the environment. Rendering tests
  run on every target, so fix the CI environment rather than skip one.
- Each example has a `smoke` task and no other tests.
- Before you delete a binding test, run `mise run coverage` for that binding and
  for `native`, then
  `mise run coverage-diff --only-in <binding> --not-in native`, and cover each
  listed line in C. See
  [Code coverage](docs/src/content/docs/development/overview.md#code-coverage).
- Skip trivial tests, tests of constants or third-party code, and negative
  assertions that prove little.

### Writing

- All prose follows the `docs-writing` skill in `.agents/skills/`: the docs
  site, READMEs, header comments, and this file.

### Pull requests

- Follow the PR template. Write **Summary** and **Test plan** in at most one
  sentence each, and follow [AI_POLICY.md](AI_POLICY.md).
- Every PR runs baseline CI, and a PR that leaves draft adds the ready targets.
  The labels `ci:apple`, `ci:android`, `ci:linux`, `ci:windows`, and `ci:ohos`
  add a platform, and `ci:full` runs every target and packaging check. Add
  `ci:full` for CI, shared toolchain, dependency, or publishing changes. See
  [CI coverage](docs/src/content/docs/development/overview.md#ci-coverage).
