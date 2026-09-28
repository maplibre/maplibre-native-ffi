# Generate bindings from the C API

The goal is to maintain each public API declaration once, in the C headers, and
generate its language bindings from shared executable rules. All eight bindings
now account for every public declaration. [Completion](completion.md) tracks the
final integration and validation gates.

PR #760 targets `main` and includes the executor work from the closed PR #631.
Implementation began at `5f6976bdf6ff826428e7d92a94ce5899246205da` in branch
`sargunv/binding-codegen`.

## Compiler and runtime

1. `tools/bindgen/frontend.py` reads public declarations with libclang,
   retaining typedef identity, layout, callback signatures, and source
   locations.
2. Header annotations describe information C types erase: execution category,
   pointer cardinality, ownership, presence, enum relationships, and variants.
3. `tools/bindgen/semantic.py` validates those relationships and resolves shared
   operation, value, callback, registration, and handle plans.
4. Static language backends generate supported public APIs and conversions.
   `tools/bindgen/native_capture.py` generates completion copies for deferred
   delivery from the same value plans.
5. Handwritten runtimes provide loading, callback rooting and scheduling,
   close-once state, and platform graphics mechanisms. Native code owns call
   leases and retirement.

The generator must derive behavior from declarations and general rules. A table
of source snippets for individual functions would preserve the maintenance
burden and does not satisfy the goal. Each generated production operation must
replace its handwritten implementation and pass public integration tests.

## Design and review

The current [architecture](architecture.md) follows correctness and performance
first, readability second, usability third, code size fourth, and compatibility
last. Breaking API changes are intentional where they improve that ordering.

The [review surface](review-surface.md) compares maintained and generated files
with both `main` and the executor parent, and defines the counting rules.

The Astra team split implementation and independent review across the
[semantic model](architecture-review.md), [native retirement](core-design.md),
and [callback compiler and host runtimes](compiler-design.md). The coordinator
integrates the language backends and validation. Earlier prototype findings
remain in [header-model.md](header-model.md),
[native-languages.md](native-languages.md),
[managed-languages.md](managed-languages.md), and [python.md](python.md); those
files describe the initial implementation, including tests against the parent
checkout's artifact.

Current results and platform limits are recorded in
[completion.md](completion.md) and [validation.md](validation.md).

## Reproduction and completion

Run through the repository's mise environment:

```sh
mise run bindings:generate
mise run bindings:check
mise run bindings:test-generator
```

The task names are defined in the root mise configuration. The underlying
commands are `python -m tools.bindgen generate`, the same command with
`--check --require-complete`, and
`python -m unittest discover -s tests/bindgen`.

`bindings/generated-coverage.json` records generated files, semantic support
relationships, and per-language operation coverage. Ambiguous contracts stop
generation. Unsupported public lowering remains explicit, and
`python -m tools.bindgen generate --check --require-complete` fails until the
migration is complete.

The validation includes header mutations that compile and execute generated
nested captures and values, allocation-failure disposal tests, existing public
binding suites, and deterministic regeneration. Current native changes require
this checkout's built library. Local host evidence does not substitute for
Android, browser, Windows, or Linux CI.
