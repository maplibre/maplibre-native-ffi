# Generate bindings from the C API

Each public API declaration is maintained once, in the C headers. The eight
language bindings are generated from those declarations by shared executable
rules. The generator derives behavior from declarations and general rules,
because a table of source snippets for individual functions would keep the
maintenance burden that generation removes.

PR #760 targets `main` from the branch `sargunv/binding-codegen`, and includes
the executor work from the closed PR #631. The branch first moves execution into
the native core, ending at "Port the C example to native execution". Binding
generation starts at "Compile binding contracts from the C headers".

## Compiler and runtime

1. `tools/bindgen/frontend.py` reads the declarations that
   `include/binding-interfaces.toml` selects, using libclang. It retains typedef
   identity, layout, callback signatures, and source locations.
2. `MLN_BINDING` header annotations describe information that C types erase:
   execution category, pointer cardinality, ownership, presence, enum
   relationships, and variants. `tools/bindgen/schema.py` validates them.
3. `tools/bindgen/semantic.py` resolves those relationships into shared
   operation, value, callback, registration, and handle plans.
4. The emitters in `tools/bindgen/emitters/` generate public APIs, values, and
   conversions for each language. `tools/bindgen/native_capture.py` generates
   native copies for deferred delivery from the same value plans.
5. Handwritten runtimes provide library loading, callback roots and scheduling,
   close-once handle state, and platform graphics mechanisms. Native code owns
   call leases and retirement.

The [generator README](../../tools/bindgen/README.md) describes how to change a
declaration and test the generator.

## Design records

- [Architecture](architecture.md) states the priorities, the decisions that
  follow from them, and the known limits.
- [Compiler design](compiler-design.md) describes the semantic model, the
  runtime boundary per language, native capture, and the ownership protocols.
- [Native ownership](core-design.md) describes disposal, retirement, and
  render-session ownership in the native core.

## Review and testing

The [PR #760 review guide](https://claude.ai/artifact/Uo1dN7sLvs6XhSpxChjbAA)
tracks open findings and decisions. The
[test architecture plan](https://claude.ai/code/artifact/316f77f8-166a-420d-9a82-ccaae2a74af5)
records the design of the test rewrite and its decisions.

Review the C contracts and shared compiler rules first, then each language's
emitter and handwritten runtime. Header mutations, lifetime regressions, and
public binding suites exercise the generated behavior. Wholly generated files
carry the `linguist-generated` attribute, so the review diff separates them from
the code that maintainers change. Deterministic regeneration checks the
committed output against the rules.

Run the generator tasks through the repository's mise environment:

```sh
mise run bindings:generate
mise run bindings:check
mise run bindings:test-generator
```

`bindings/generated-coverage.json` records the generated files, the semantic
support relationships, and the operation coverage of each language. The check
task fails on an ambiguous contract, on an unsupported public declaration, and
on committed output that differs from regeneration. Native changes require a
build of this checkout's library before the binding suites run against it.
