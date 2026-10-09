# Binding generator

`tools/bindgen` reads the public C headers with libclang and writes every
binding's native declarations and public operations. Each binding pairs that
output with a handwritten runtime, which owns callback roots, completions,
status checks, and handle state. A generated operation names its C function,
maps its parameters, and calls a runtime helper, so a change to one of those
steps changes one helper that every operation shares.

## Change a C declaration

`include/binding-interfaces.toml` selects the headers that the generator reads.
`MLN_BINDING` attributes on a declaration describe what its C type cannot
express, such as which parameter counts an array or which callback retires a
context.

1. Edit the declaration under `include/` and its implementation under `src/`.
2. Annotate its execution, values, and ownership with `MLN_BINDING`.
3. Run `mise run bindings:generate`.
4. Review the generated signatures and `bindings/generated-coverage.json`.
5. Run `mise run bindings:test-generator` and the affected binding suites.

`mise run bindings:check` fails when the committed output is stale or when a
public declaration has no generated operation. The coverage report lists each
unsupported declaration with the emitter's reason. Never edit generated files by
hand; change the header, the compiler rule, or the runtime helper, and
regenerate.

Pick the execution category that describes the native operation. The schema
checks the signature that each category requires.

| Execution       | Operation                                                      |
| --------------- | -------------------------------------------------------------- |
| `command`       | Mutates state; completes with a disposition and a generation   |
| `query`         | Reads state; completes with a value                            |
| `operation`     | Runs other asynchronous native work; completes with its result |
| `lifecycle`     | Creates, attaches, or retires a handle                         |
| `immediate`     | Returns its result synchronously                               |
| `snapshot`      | Copies current state synchronously from a live handle          |
| `event_batch`   | Drains queued events or frame results into an owned batch      |
| `render_driver` | Services graphics work on the thread that the driver requires  |

## Where each rule lives

| Change                                              | Module              |
| --------------------------------------------------- | ------------------- |
| Header extraction and attribute parsing             | `frontend.py`       |
| Accepted attributes and signature constraints       | `schema.py`         |
| Value shapes, presence, and ownership relationships | `semantic.py`       |
| Native copies for deferred callbacks and results    | `native_capture.py` |
| Language syntax and runtime calls                   | `emitters/`         |

Resolve a new relationship once in `semantic.py` and let every emitter consume
the plan. Emitters choose syntax and report the shapes that they cannot lower;
they never invent ownership.

## Test a change

Each layer of a change has one home:

| Layer                                  | Tested in                                                  |
| -------------------------------------- | ---------------------------------------------------------- |
| Native behavior                        | The C ABI suite in `tests/native`                          |
| A protocol shape and its rejection     | A group in `tests/bindgen/fixtures/protocols.h`            |
| Generated code for every shape         | `tests/bindgen/test_generation.py` and the executed probes |
| A binding's runtime and a sample value | The binding suite, through `tests/conformance/cases.toml`  |

A new protocol adds a group to `protocols.h`, enabled by its own
`MLN_PROTOCOL_*` macro. `test_generation.py` runs every group through every
emitter, and each declaration must generate unless `PROTOCOL_GAPS` lists it for
that emitter. A protocol that no conformance case covers adds a case to
`cases.toml`, and every binding maps it.

An executed probe compiles generated code with its binding's runtime and runs it
against `tests/bindgen/fixtures/protocols_stub.c`. A probe skips when its
toolchain is missing, and CI sets `MLN_BINDGEN_REQUIRE_TOOLCHAINS=1` to turn the
skip into a failure.
