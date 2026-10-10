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
checks the signature that each category requires. A function without a
completion parameter is `immediate` unless it declares another synchronous
category.

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

## Annotate only what convention leaves open

The frontend completes each declaration's metadata with the conventions that
`Conventions` in `schema.py` derives from its C shape, so every later stage
reads a complete contract. An annotation states a departure from convention, and
the schema rejects one that restates a default.

| Declaration                         | Default                                                                                    |
| ----------------------------------- | ------------------------------------------------------------------------------------------ |
| Function without a completion       | `execution=immediate`                                                                      |
| Completion function                 | `result=void`; a value result has `shape=value` and is `borrowed`, or `owned` for a handle |
| Parameter                           | `direction=in`; an output pointer to a handle is `owned`                                   |
| Pointer                             | `ownership=borrowed`, except a callback                                                    |
| Pointer to a record                 | `length=1`                                                                                 |
| Character pointer                   | `length=nul;encoding=utf8`                                                                 |
| Buffer view or pointer to one       | `encoding=utf8`                                                                            |
| `void*`                             | `kind=context` in a callback signature, `kind=native_pointer` elsewhere                    |
| Value                               | `lifetime=owner` for a context, `lifetime=call` otherwise                                  |
| First struct member `uint32_t size` | `kind=size;default=sizeof`                                                                 |
| Member that a sibling names         | `kind=count` for `length`, `kind=presence_mask` for `mask`, `kind=tag` for `tag`           |
| `kind=reserved` member              | `default=0`                                                                                |
| Callback typedef                    | `thread=native;reentry=allow`; a void callback has `failure=contain`                       |
| Handle typedef                      | `parent=none`; its operations begin with its own name (`prefix=<handle>`)                  |
| Callback registration               | `user_data` names its one `kind=context` member or parameter                               |
| Record typedef                      | `default` names the one function that takes no arguments and returns the record            |

`STRUCT_SIZE_FIELD` in `schema.py` names the size member, because other structs
begin with an unrelated `uint32_t` member. A struct whose size member has
another name annotates it `kind=size`. `protocol.py` names the protocol types
that every handwritten runtime is written against: the status enum, the
diagnostic, the completion and its result, and the buffer view. No other rule
reads a declaration's name.

Three keys state what a C shape cannot:

- `prefix=` on a handle names the prefix of its operations when that differs
  from the handle's type name, as `mln_resource_request` does for
  `mln_resource_request_handle`.
- `fields=ordered` on a record of plain values says that its field order is part
  of its meaning, as with coordinates, so a binding may construct it
  positionally. The schema rejects it on a record with control, pointer, or
  array members.
- `synchronous=true` on a callback typedef says that native code relies on the
  callback's work being done when it returns, as with the queue lock's
  callbacks. A binding runs the callback on the calling thread and never
  delivers it later through a port. Dart can only deliver later, so a record
  with a native default keeps such a registration at its disabled default.

An annotation that names another declaration, such as `reentry_calls`,
`complete`, `cancel_registration`, or `wait_retired`, must name one that exists;
the schema reports the name that does not resolve.

## Where each rule lives

| Change                                              | Module              |
| --------------------------------------------------- | ------------------- |
| Header extraction and attribute parsing             | `frontend.py`       |
| Conventions, accepted attributes, and signatures    | `schema.py`         |
| Value shapes, presence, and ownership relationships | `semantic.py`       |
| Native copies for deferred callbacks and results    | `native_capture.py` |
| The value type that each completion delivers        | `native_results.py` |
| Language syntax and runtime calls                   | `emitters/`         |

Resolve a new relationship once in `semantic.py` and let every emitter consume
the plan. Emitters choose syntax and report the shapes that they cannot lower;
they never invent ownership.

The plan also names each public member once, and an emitter only converts its
case and escapes keywords:

| Plan field              | Rule                                                                                        |
| ----------------------- | ------------------------------------------------------------------------------------------- |
| `OperationPlan.member`  | `name=` if declared; else the name without its receiver's prefix; else without `mln_`       |
| `HandlePlan.stem`       | The handle's operation prefix without `mln_`, which owner type names extend                 |
| `BorrowedViewPlan.stem` | The view operation's member without a leading `get_`, read as `with_<stem>`                 |
| `PresenceGroup.member`  | A bit without its enum's shared prefix, or a boolean mask without `has_`                    |
| `MaskFlag.member`       | The flag constant without its enum's shared prefix                                          |
| `FieldPlan.public`      | False for a control role: size, reserved, count, stride, arena, mask, tag, context, release |
| `OperationPlan.status`  | Whether the function returns the status enum                                                |
| `OperationPlan.support` | The record default, handle disposal, or borrowed view scope that the operation backs        |

`native_results.py` writes `src/completion/completion_result_generated.inc`,
which specializes `CompletionResult` for each completion function other than a
command. The specialization records whether the header names a `result=` value,
and for a value it records the C type and whether the value is an array or
nullable. The native core delivers a success through one of two entry points in
`src/completion/completion_result.hpp`, each named by its function:

- `CompletionValue<&function>` delivers a value and takes its type and shape
  from the table.
- `valueless_completion<&function>()` completes a function with no value. Code
  that several functions share takes this from each entry point.

A success that disagrees with the header fails to compile, so the native library
delivers the type that every binding reads. A failure carries no value and
passes through `complete_failure`. Commands carry no value by schema and report
their disposition through `complete_command`.

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
