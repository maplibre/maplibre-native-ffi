---
title: Generate bindings
description: Change a C declaration and regenerate its language bindings.
sidebar:
  order: 2
---

Change the C declaration and its binding contract together, then regenerate the
bindings. A declaration that uses an existing value and ownership protocol needs
no handwritten wrapper. A new protocol needs compiler and runtime support before
its operations can pass the complete-coverage check.

## Change an interface

The entrypoints in `include/binding-interfaces.toml` select public declarations
and binding runtime adapters. Clang reads their types and `MLN_BINDING`
attributes. The attributes describe relationships that C types cannot express,
such as which parameter counts an array or which callback retires a context.

1. Edit the declaration under `include/` and its native implementation.
2. Describe its execution, values, and ownership with `MLN_BINDING` attributes.
3. Run `mise run bindings:generate`.
4. Inspect the generated signature and `bindings/generated-coverage.json`.
5. Run `mise run bindings:test-generator` and the affected binding suites.
6. Run `mise run bindings:check` to verify committed output.

The compiler rejects unknown attributes, inconsistent signatures, and unresolved
ownership relationships. Its source is the translation specification:

| Change                                              | Implementation                    |
| --------------------------------------------------- | --------------------------------- |
| Accepted attributes and signature constraints       | `tools/bindgen/schema.py`         |
| Value shapes, presence, and ownership relationships | `tools/bindgen/semantic.py`       |
| Native copies for deferred completions and calls    | `tools/bindgen/native_capture.py` |
| Language syntax and runtime calls                   | `tools/bindgen/emitters/`         |
| Header mutation and rejection tests                 | `tests/bindgen/`                  |

Use the execution category that describes the native operation. Commands report
mutation disposition and generation; queries return ordered values. Published
snapshots copy current state synchronously. Lifecycle operations transfer or
retire ownership, and render-driver operations service graphics work on the
required thread. The schema checks the signature that each category requires.
The C declaration documents the operation's behavior for callers.

The coverage report separates public operations from verified support
relationships, such as a default constructor or an owner's disposer. An
unsupported declaration includes the emitter's reason. Run the compiler with
`generate --check --require-complete` to reject any remaining unsupported public
declaration.

## Add a protocol

Resolve a new relationship once in the semantic model and let every emitter
consume that result. Keep language-specific code focused on its runtime: library
loading, exception conversion, garbage collector roots, completion delivery, and
scoped native access.

A retained callback transfers its root only after native acceptance. Rejected
registration releases the temporary root locally; accepted registration releases
it at native quiescence. Callback-scoped responses expire when their callback
returns. A callback whose contract forbids native reentry also constrains
cleanup that host finalizers initiate from that callback.

Some hosts cannot run code on a MapLibre thread, yet a logging or resource
provider callback must answer before it returns. A callback typedef marked
`deferred=VALUE` names the result a native adapter may return at once for such a
host. The adapter copies the call's arguments into a native-owned record and
delivers the record to the host later. The compiler accepts the annotation only
when the callback returns a value, VALUE is a value of the result's enum or an
integer its scalar result holds, the callback has one context parameter, and
every other parameter is an input borrowed for the call. A decision handle
transfers to the record, so VALUE must be the decision's accept value.

The native capture compiler generates one arguments record and one adapter
function per deferred typedef. `mln_adapter_deferred_callback_function()`
returns the adapter, and its context delivers each record to a listener function
or a Dart port. A call the adapter cannot copy returns the callback's failure
result. A record destroyed before the host adopts its decision handle fails that
request.

A borrowed GPU view holds a native scope through the host callback. The scope
keeps session resources alive if another frame is finalized. Native retirement
invalidates future views and waits for active scopes before releasing resources.
Explicit release and abandon report busy while a conflicting scope is active.

Each layer of a change has one place where it is tested:

| Layer                                        | Tested in                                                                                                       |
| -------------------------------------------- | --------------------------------------------------------------------------------------------------------------- |
| Native behavior of the declaration           | The C ABI suite in `tests/native`, which calls every exported function by name                                  |
| The protocol shape and its rejection         | A group in `tests/bindgen/fixtures/protocols.h`, and compiler tests in `tests/bindgen`                          |
| Generated code for the shape                 | The [shape table test](#test-the-generator) for every emitter, and an executed probe where the language has one |
| Handwritten runtime mechanism                | Each binding suite, through the conformance cases for callbacks, completions, and handles                       |
| One representative generated value per shape | Each binding suite, through the value conformance cases, against the native library                             |

Test the protocol through a public binding API, including its rejection and
retirement paths. A protocol that no case in `tests/conformance/cases.toml`
covers adds a case there, and the conformance check fails until every binding
maps it to a test or to a reason that it does not apply.

Generated files include their source notice and are formatted by the generation
task. Review the header, compiler rule, handwritten runtime mechanism, and
public behavior test together. The generated diff shows the result of that rule
across languages.

## Test the generator

`mise run bindings:test-generator` tests generator behavior. A test asserts on
the semantic plans, on an emitter's coverage report, or on generated code that
the test compiles and runs, so renames and formatting changes in an emitter
leave the tests unchanged.

`tests/bindgen/fixtures/protocols.h` declares one group per protocol shape, each
enabled by its own `MLN_PROTOCOL_*` macro, after base declarations that stand in
for the C API's shared types. A new protocol adds its group there. The shape
table test runs every group through every emitter. It lists each declaration
that an emitter reports unsupported and requires every other declaration to
generate, so an emitter that loses a shape fails the suite.

`tests/bindgen/test_cli.py` runs the compiler's command line and checks that
`--check` and `--require-complete` fail when they should, and that invalid
headers fail before any output is written.

An executed probe builds generated code with the binding's handwritten runtime
and runs it. The Go, Swift, Zig, and Dart probes generate bindings for the
`values` and `keywords` groups and call
`tests/bindgen/fixtures/protocols_stub.c`. The Rust probe compiles its own
declarations against an ABI stub, and the .NET probe runs the value conversions
of the `presence_mask` group. The Python tests execute the generated modules
over fakes of the native extension. A probe finds its toolchain through the
binding's mise configuration and skips when that toolchain is missing. The CI
hygiene job sets `MLN_BINDGEN_REQUIRE_TOOLCHAINS=1`, which turns each skip into
a failure.

The generator also writes one synthesized result for each completion copy kind
to `tests/native/abi/adapter/adapter_copy_cases_generated.inc`.
`tests/native/abi/adapter/copies.c` passes each result through an adapter
completion and checks that the delivered record holds equal content in storage
of its own. `tests/bindgen/test_native_capture.py` checks that every copy kind
has exactly one case.
