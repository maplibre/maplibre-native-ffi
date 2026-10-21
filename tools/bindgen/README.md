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
unsupported declaration with the emitter's reason. Its `files` list records
every output, so the next run deletes a listed file that the generator no longer
writes, and a check reports it. Never edit generated files by hand; change the
header, the compiler rule, or the runtime helper, and regenerate.

Pick the execution category that describes the native operation. The schema
checks the signature that each category requires. A function without a
completion parameter is `immediate` unless it declares another synchronous
category.

| Execution     | Operation                                                        |
| ------------- | ---------------------------------------------------------------- |
| `command`     | Mutates state; completes with a disposition and a generation     |
| `query`       | Reads state; completes with a value                              |
| `operation`   | Runs other asynchronous native work; completes with its result   |
| `lifecycle`   | Creates, attaches, or retires a handle                           |
| `immediate`   | Returns its result synchronously                                 |
| `snapshot`    | Copies current state synchronously from a live handle            |
| `event_batch` | Drains frame results into an owned batch, or reports none queued |

## Name a declaration

Name a function `mln_<owner>_<verb>[_<object>]`. The owner is the handle that
the function acts on, its first parameter, so a binding's member reads as a verb
on that object: `mln_map_get_style_layer`, `mln_runtime_create_map`,
`mln_render_session_set_metal_surface_target`. A read uses `get_`, a read of
every object uses `list_`, and a predicate uses `is_`. A callback helper names
the record that it fills, as in `mln_http_header_transform_response_set_header`.
A function with no handle parameter names the subsystem or record that it
belongs to: `mln_log_set_callback`, `mln_network_get_status`,
`mln_camera_options_default`. A computation from one record names the record and
its result, as in `mln_logical_extent_physical_size`. A coordinate conversion
names its result and its input, as in `mln_map_projection_pixel_for_lat_lng`,
and a function that reports a fixed fact of the library is a noun, as in
`mln_c_version`.

Use `_release` for a handle's teardown. A successful release consumes the
handle; a failed release retains it, and an asynchronous release takes a
completion. A handle whose release is asynchronous, can fail on state, or takes
arguments also has `_dispose`: it consumes a live handle without blocking on
host work, and finalizers call it. `detach` and `abandon` end an attachment and
keep those verbs.

## Follow the ABI rules

Each declaration follows these rules, which every binding relies on:

- Declare each function as `MLN_API <type> <name>(...) MLN_NOEXCEPT`.
- A fallible function returns `mln_status` and takes
  `mln_diagnostic* out_diagnostic` as its last parameter, except a callback
  implementation, which matches its callback typedef. The handle comment in
  `base.h` defines the status for an invalid or released handle.
- A new or changed function lists every status that it returns under `Returns:`
  in its comment. An asynchronous function also lists its completion outcomes
  under `Completes with:`.
- Use fixed-width integers, `size_t` for counts and byte lengths, `float`,
  `double`, and `bool`. An enum declares a fixed underlying type, which is
  `uint32_t` unless its values need another. A struct member that holds an enum
  value has that integer type and `MLN_BINDING("enum=<enum>")`, because native
  can write a value that an older binding does not know. An enum of flags that
  combine is `kind=bitmask`.
- A struct is versioned by exactly one thing, and new members go at the end:
  - A struct that a caller passes through a pointer, as an input or as an output
    that the caller allocates, or that native passes by pointer as a callback
    argument, begins with `uint32_t size`. Native sets the size of each struct
    that it passes to a callback. For an input struct, and for an output struct
    that the caller allocates, native rejects a size below its own `sizeof` with
    `MLN_STATUS_INVALID_ARGUMENT` and accepts a larger one. `mln_diagnostic` is
    the exception: native accepts any size, writes no more than size bytes, and
    truncates the message to fit.
  - A record that native only delivers in borrowed storage carries no size. The
    stride of that storage versions it: `mln_completion_result.value_size` for a
    completion value, and `event_size` or `result_size` for an element of a
    strided view. A binding steps through an array by that stride. A record that
    is also passed by pointer keeps its size, and native sets it when delivering
    the record, as with `mln_camera_options`.
  - A struct that the public headers only embed by value carries no size. Its
    container versions it, and it cannot grow within an epoch, because growing
    it moves every later member of its parent.
  - When a sized struct is embedded by value, native ignores its size on input
    and sets it on output, as with the `camera` member of `mln_camera_update`
    and of `mln_map_view`.
  - Small value types, such as coordinates, points, IDs, and `mln_buffer_view`,
    are frozen and carry no size. Changing one means adding a new type.

  The schema rejects a size on a struct that the public headers reach only by
  value inside records, or only as a completion value or a strided element. An
  array that one of those records borrows reaches its elements the same way: a
  binding indexes an array without a stride by its own size, so those elements
  are frozen like an embedded struct.

- An input struct whose defaults are not all zero has a `mln_<struct>_default()`
  function. It returns the struct with size set and every member at its default.
  A binding starts each record from that function, or from zero with size set
  when the struct has none. The schema rejects a default function for a record
  that native never reads, directly or embedded in an input.
- Host-lent input strings use `mln_buffer_view`. Native-lent strings use
  NUL-terminated `const char*`. Native rejects embedded NUL in views that reach
  C-string consumers, including paths, URLs, HTTP headers, and etags.
- An optional scalar or aggregate member of a struct names its bit in the
  struct's `fields` mask, a `uint32_t` whose `enum=` names a `kind=bitmask`
  enum: `MLN_BINDING("mask=fields;bit=<constant>")`. The mask names a sibling
  member. Each optional member has its own bit. Values that are present together
  form a record embedded under one bit, and the schema rejects a bit that guards
  two members of one struct. An array's count takes the array's presence and
  carries no mask or bit. Native treats an unknown bit in an input mask as
  invalid. A pointer-shaped value, such as a callback, an owned handle, a
  nul-terminated string, or a by-pointer parameter, is optional through
  `nullable=true`. A buffer-view parameter, result, or member that treats an
  empty view as absent is `optional=empty`.
- A reserved member is `kind=reserved`, and every writer sets it to zero.
- A handle output parameter owns the handle that it receives, and `*out_handle`
  must equal `MLN_HANDLE_NULL` on entry. Every other pointer is borrowed for the
  call unless its annotation says otherwise. A function that keeps an input
  copies it before returning, and its comment says so.

While `API_EPOCH` is `0`, a change may break the ABI, and growing a struct
breaks every caller built against the older header; see
[Versioning](../../docs/src/content/docs/development/versioning.md). A stable
epoch must accept every earlier published size of a struct and fill the missing
members from their defaults, and every binding must index an array by the stride
that native reports. The size member, the delivery stride, and the default
function make that possible, so new structs follow these rules now.

Bindings copy record defaults from the native default functions. A constructor
that supplies individual fields reads each omitted member from that copy. Go and
Zig callers seed records through the generated default functions.

## Annotate only what convention leaves open

The frontend completes each declaration's metadata with the conventions that
`Conventions` in `schema.py` derives from its C shape, so every later stage
reads a complete contract. An annotation states a departure from convention, and
the schema rejects one that restates a default. An annotation writes only the
values in `ANNOTATION_VALUES`. Conventions supply most of each key's other
values, such as `direction=in` and `shape=none`, and the semantic layer supplies
the rest: `lifetime=completion` for an array completion result. Handle release
consumption follows the release function's return type.

| Declaration                         | Default                                                                                               |
| ----------------------------------- | ----------------------------------------------------------------------------------------------------- |
| Function without a completion       | `execution=immediate`                                                                                 |
| `execution=event_batch`             | `absent_on=MLN_STATUS_NOT_READY`                                                                      |
| Completion function                 | `result=void`; a value result has `shape=value` and is `borrowed`, or `owned` for a handle            |
| Command completion                  | Discardable; a null completion discards the outcome. Other completion functions require a completion. |
| Parameter                           | `direction=in`; an output pointer to a handle is `owned`                                              |
| Pointer                             | `ownership=borrowed`, except a callback                                                               |
| Pointer to a record                 | `length=1`                                                                                            |
| Character pointer                   | `length=nul;encoding=utf8`                                                                            |
| Buffer view or pointer to one       | `encoding=utf8`                                                                                       |
| `void*`                             | `kind=context` in a callback signature, `kind=native_pointer` elsewhere                               |
| Value                               | `lifetime=owner` for a context, `lifetime=call` otherwise                                             |
| First struct member `uint32_t size` | `kind=size`                                                                                           |
| Member that a sibling names         | `kind=count` for `length`, `kind=presence_mask` for `mask`, `kind=tag` for `tag`                      |
| `kind=reserved` member              | Zeroed reserved storage                                                                               |
| Callback typedef                    | a void callback has `failure=contain`                                                                 |
| Handle typedef                      | `parent=none`; its operations begin with its own name                                                 |
| Callback registration               | `user_data` names its one `kind=context` member                                                       |
| Record typedef                      | `default_function` names the one function that takes no arguments and returns the record              |

`STRUCT_SIZE_FIELD` in `schema.py` names the size member, because other structs
begin with an unrelated `uint32_t` member. `protocol.py` names the protocol
types that every handwritten runtime is written against: the status enum, the
diagnostic, the completion and its result, and the buffer view. It also names
`MLN_STATUS_NOT_READY`, which reports a drain with nothing queued, and the queue
delivery types: the delivery queue and batch handles, one delivery, its payload
union, and the batch view. A function that takes or returns a delivery queue or
batch belongs to the delivery protocol, which each binding runtime drives
itself, so the generator treats it, and every type that only it reaches, as a
runtime export with no public member. The completion result's erased value
member is also defined in `protocol.py`.

These keys state what a C shape cannot:

- `delivery=` on a callback typedef names the `mln_delivery_type` constant whose
  `mln_delivery_payload` variant carries a copy of the callback's arguments. The
  variant's record mirrors the arguments other than the context, in order, by
  name and C type: a pointer becomes a `lifetime=owner` pointer that the
  delivery owns, and a handle stays the same handle. Callbacks of the same shape
  may share a record, as a custom source's fetch and cancel do. A registration
  whose callbacks all declare a delivery can name a queue in place of them, so a
  host that cannot run code on a native thread receives the copies after
  draining. The schema rejects the key on a `synchronous=true` callback.
- `synchronous=true` on a callback typedef says that native code relies on the
  callback's work being done when it returns, as with the queue lock's
  callbacks. A binding runs the callback on the calling thread and never
  delivers it later through a queue or a wake. Dart can only deliver later, so a
  record with a native default keeps such a registration at its disabled
  default.
- `absent_on=` on a function names a failure status of `mln_status` that reports
  its one output as absent rather than failed, as `MLN_STATUS_NOT_READY` does
  for `mln_render_session_acquire_frame` when no frame has rendered. A drain is
  absent on `MLN_STATUS_NOT_READY` by convention, so a drain writes the key only
  to name another status. A binding returns its language's empty form for that
  status, such as `None`, `nil`, or `null`, and reads or adopts the output only
  on success. The schema accepts the key only on a function that returns a
  status, takes no completion, and has exactly one output. The semantic plan
  also rejects it on a borrowed view, a consuming operation, and a call that
  passes a callback registration, because an absent call publishes nothing.
  Dart, Go, Rust, Swift, and Zig return any output as absent; .NET, Kotlin, and
  Python return only an owned handle as absent.
- `accepted_unless=` on a function names a boolean output that, when set on
  success, reports that native kept nothing from the function's one callback
  registration, as `out_cancelled` does for
  `mln_resource_request_set_cancel_callback`. The caller still owns that
  registration, so a binding releases its roots before returning.
- `borrowed_owner=` on a callback names a borrowed handle parameter that
  identifies the registration's receiver. The host's callback already holds that
  owner, so bindings leave the redundant parameter out of its signature.

`capture=manual` on a delivery record leaves its native capture to the core.
Runtime events use this to share record and message storage across a batch.

Every callback registration is a struct with `kind=callback_registration`, which
a function takes by pointer. A registration may hold one `mln_delivery_queue`
member marked `kind=delivery_queue`, which a binding runtime sets in place of
its callbacks; it is a control role, so no binding exposes it. Each of the
registration's callbacks must be the protocol completion callback or declare
`delivery=`, and one `MLN_DELIVERY_TYPE_RETIRED` delivery follows the last of
them. Dart, whose host code runs only on its isolate, uses the queue for every
such registration, and a wake for a registration whose one callback takes
nothing but its context.

No annotation names a callback's thread. Every generated binding treats a
callback as able to run on any native thread, and each callback's header comment
states the threads that it runs on.

An annotation that names another declaration, such as `release=` or
`borrowed_owner=`, must name one that exists; the schema reports the name that
does not resolve.

## Document a declaration

Headers state caller contracts, and generated API pages present them. The
[documentation site](../../docs/src/content/docs/index.mdx) introduces concepts
and task paths. Link guides to the relevant reference page instead of repeating
status lists, ordering rules, or detailed ownership contracts.

Each generated declaration carries the first paragraph of its header comment and
a link to its header's page in the C API reference. The paragraph ends at a
blank line, a list item, or a heading such as `Returns:`, so write a first
paragraph that stands alone as a summary. The C header remains the full
contract: status lists, output parameters, and ownership rules name C concepts
that each binding expresses differently, so the bindings repeat only the
summary.

A comment documents the declaration directly after it. Only whitespace,
`MLN_BINDING` annotations, and the `typedef` before a tag may separate the two.
A declaration without a comment has no generated comment, because the reference
lists only documented declarations.

The summary keeps C identifiers as written and marks each `mln_` and `MLN_` name
as code. `docs.py` renders it in each language's comment syntax and escapes its
prose for that language's documentation tool. A record field and an enum
constant carry their summary without the link, which their type carries.

## Where each rule lives

| Change                                                    | Module              |
| --------------------------------------------------------- | ------------------- |
| Header extraction and attribute parsing                   | `frontend.py`       |
| Summaries, reference links, and comment escaping          | `docs.py`           |
| Conventions, accepted attributes, and signatures          | `schema.py`         |
| Value shapes, presence, and ownership relationships       | `semantic.py`       |
| Native copies for queue deliveries and completion results | `native_capture.py` |
| The value type that each completion delivers              | `native_results.py` |
| Language syntax and runtime calls                         | `emitters/`         |

Resolve a new relationship once in `semantic.py` and let every emitter consume
the plan. Emitters choose syntax and report the shapes that they cannot lower;
they never invent ownership.

`BoundApi.returned` holds the records and unions that a binding copies from
native: those that an operation's output or result, a callback's argument, or a
record default reaches. A binding builds a callback registration from host
callbacks, so native returns one only in a record default, with null callbacks,
in a field without presence. A copy of such a default leaves a registration
field unset, and the copy of a registration's own default copies only its other
fields. A copy of an optional or referenced registration would have to keep its
presence, so the plan rejects one in a default, along with every other operation
or callback that returns a registration.

The plan also names each public member once, and an emitter only converts its
case and escapes keywords:

| Plan field              | Rule                                                                                                 |
| ----------------------- | ---------------------------------------------------------------------------------------------------- |
| `OperationPlan.member`  | The name without its receiver's prefix, or else without `mln_`                                       |
| `HandlePlan.stem`       | The handle's operation prefix without `mln_`, which owner type names extend                          |
| `BorrowedViewPlan.stem` | The view operation's member without a leading `get_`, read as `with_<stem>`                          |
| `MaskFlag.member`       | The flag constant without its enum's shared prefix                                                   |
| `FieldPlan.public`      | False for a control role: size, reserved, count, stride, mask, tag, context, release, delivery queue |
| `OperationPlan.status`  | Whether the function returns the status enum                                                         |
| `OperationPlan.support` | The record default, handle disposal, or borrowed view scope that the operation backs                 |
| `OperationPlan.absence` | The `absent_on=` status, its value, and the output that it reports absent                            |

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
