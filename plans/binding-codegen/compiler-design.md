# Compile the C API into eight bindings

The binding compiler resolves the C declarations and their contracts into one
typed semantic model. Static language emitters generate public values,
operations, conversions, and callback trampolines from that model. Handwritten
runtimes implement host scheduling and memory primitives. A header change that
uses an established semantic rule needs only regeneration and validation across
all eight bindings.

## Frontend

The frontend uses the pinned `libclang` Python package. LLVM recommends
[libclang's stable C interface](https://clang.llvm.org/docs/Tooling.html) for
tools that traverse declarations without depending on the changing C++ AST.
Clang parses typedefs, parameter declarations, callback signatures, record
declarations, nested unions, and enum expressions.

The model in `tools/bindgen/model.py` preserves typedef identity alongside
canonical C types, so map and runtime handles stay distinct although both use
the same integer ABI. Pointers keep pointee qualifiers, callbacks keep their
function types and parameter names, and records keep their declared fields.
Incomplete records, bit fields, and fixed or incomplete arrays are represented,
so that an emitter can reject an unsupported shape explicitly. Host compiler
types such as `size_t` keep their typedef identities, and emitters preserve
their target width.

`MLN_BINDING("key=value;...")` expands to a Clang annotation when the compiler
extracts headers, and to nothing in ordinary builds. An annotation belongs to
the declaration, field, parameter, or typedef whose contract it describes. It
contains no target-language templates or function-name inventories. Every public
function declares an execution category.

`include/binding-interfaces.toml` separates the public C interface from the
native binding-runtime interface. Clang resolves the include closure of both
entrypoints, and a declaration outside both closures fails validation. Shared
includes keep their public classification. Runtime adapters are compiler inputs
that the coverage report lists separately from public operations. External
plugin ABI types keep their C names and canonical types, because a plugin
authoring emitter would need its own ownership contract.

The parser uses the configured Clang driver's builtin headers and the selected
system SDK. Cross-target extraction requires that target's include paths and
compiler arguments.

## Semantic model and lowering

`tools/bindgen/semantic.py` consumes the frontend model and validates every
cross-declaration relationship once. Language emitters consume the resolved
contracts, and keep no lists of supported handles and functions.

| Component | Required information                                                                                                                                 |
| --------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| Value     | Scalar width and signedness, open enum or flags, record members, fixed array, counted span, encoded bytes, active union variant, presence condition  |
| Handle    | Native identity, parent relationship, release operation, consumption condition, abandonment operation, permitted thread                              |
| Callback  | Signature, borrowed inputs, owned outputs, retained context, release callback, exception fallback, synchronous decision or asynchronous notification |
| Operation | Receiver or global owner, execution category, inputs, immediate outputs, completion payload, status policy, lifetime transitions                     |
| Storage   | Call, completion, registration, handle, or process lifetime; allocation and release responsibility                                                   |

Presence is a typed expression over sibling members: a boolean, mask bit, null
pointer, empty view, or discriminant. One mask bit may control several members.
Variants associate a discriminant value with exactly one union member. The
resolver verifies member existence, integer masks and tags, exhaustive known
variants, count types, release signatures, and parent identity. It rejects a
pointer whose storage or length no contract derives. A counted pointer requires
an explicit count relationship, and a single record reference requires
`length=1`.

Recursive value plans describe both input materialization and output capture.
Optional fields become language optional values, and tagged unions become
language sum types. Unknown open enum values keep their integer representation.
Defaults come from the record's declared default constructor and from field
`default` annotations. Reserved storage and structure sizes stay generated
implementation details.

An operation plan identifies ordered actions for preparation, submission,
acceptance, rejection, completion, delivery, and abandoned delivery. It records
resources, their owners, and the transition that transfers each resource.
Emitters translate those actions into direct calls and structured cleanup in the
target language. The plan is compiler data, and never a runtime interpreter.

Snapshots capture values immediately and preserve the generation published with
the state, and a snapshot operation stays synchronous. Asynchronous queries copy
borrowed data before the C callback returns. Command completions keep the
terminal disposition, status, diagnostic, and generation, including on failure.

The resolver accounts for every public declaration as a generated operation, a
generated value or callback, or a verified support relationship, such as a
default constructor or an owner's disposer. A support relationship requires a
checked consumer in the generated code. Labelling an unsupported operation as
runtime infrastructure does not complete coverage.

## Runtime boundary

The runtime contains mechanisms whose implementation depends on the language:
completion roots, scheduling, native library loading, handle state, allocation,
thread attachment, and exception containment. Generated code supplies the
operation-specific converters, callback signatures, and lifecycle transitions,
so a new camera field, source callback, render descriptor, or result type adds
no handwritten runtime case.

| Binding | Runtime mechanisms and generated boundary                                                                                                                                                                                     |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Rust    | Typed futures, ownership state, allocation, and panic containment are runtime code in the core crate, which Python shares. Generated owners call C through call helpers and recursive value conversions.                      |
| Swift   | Continuation delivery, retained callback roots, and synchronized handle state are runtime code. Generated operations are grouped by C header and owner, beside generated values, owners, and views.                           |
| Zig     | Allocator-aware futures, owner state, and synchronization are runtime code. Generated values provide recursive disposal, including handles, and unclaimed creation results use native disposal.                               |
| Go      | Completion channels, cgo handle tokens, and callback ingress are runtime code. Generated C trampolines and typed Go converters follow cgo pointer rules, and retained native storage holds tokens rather than Go pointers.    |
| .NET    | Task completion, callback roots, handle state, and unmanaged callback ingress are runtime code. Generated partial classes, `LibraryImport` declarations, and static converters avoid reflection and dynamic dispatch.         |
| Kotlin  | Common public types and operations are generated once. Common code calls one generated declaration object, with JVM FFM downcalls, Kotlin/Native cinterop calls, and Android JNI glue as generated platform shims.            |
| Python  | PyO3 handles native callback ingress and releases the GIL around calls that can wait for callbacks. Generated native methods, Python operations, owners, values, and stubs share one plan.                                    |
| Dart    | Native code captures borrowed callback storage before posting to an isolate. Generated native copiers and Dart decoders share value semantics. Listener tokens, isolate delivery, and native record release are runtime code. |

Every foreign callback contains host exceptions or panics. Callback replacement
keeps the prior root alive until native invokes its release callback. Native
release determines quiescence, which completion alone does not establish.
Reentrant callbacks never execute host code while a binding holds a handle-state
lock. Blocking native teardown and driver service release the Python GIL
whenever native work may enter Python. Coroutine or future cancellation ends
host waiting, and leaves accepted native ownership in place.

## Native capture

Dart is the one binding that cannot run host code on a MapLibre thread, so its
completions and deferred callbacks go through native capture. The native
callback adapter uses static capture functions that `native_capture.py`
generates for each reachable completion value. A measuring pass computes
checked, aligned storage, and then one allocation holds the record and all
copied payload bytes. A writing pass copies each borrowed byte once and rewrites
pointers into that allocation. The generated dispatcher selects a compiled
copier by a type identifier, which is a deterministic hash of the native type
and its ownership. Nested arrays and union variants follow the same recursive
rules as the other bindings, and the bindings with direct callbacks keep their
immediate capture path.

Owned handle results stay owned by the native record until the Dart decoder
constructs a host owner and adopts the record. Discarded records and failed
captures invoke the declared disposal operation, and owned root arrays dispose
every element. The compiler rejects nested owned handle fields before any array
or pointer conversion.

Dart completion messages transfer copied results through the VM's native-pointer
message finalizer, and undelivered messages dispose unclaimed result owners.
Logging and provider queues use static native wake callbacks. Request
cancellation keeps its native port context in the request through callback
quiescence. These paths remain callable after isolate shutdown without
executable Dart callback addresses.

Dart notification stubs are generated from registration descriptors with void
callbacks and recursively copyable scalar records. Each descriptor field has a
stable message identifier. Native code copies the arguments into a Dart port
message, and a shared release callback closes the native context after callback
quiescence. Host callbacks run in the registration zone, and owner close waits
for the release messages.

## Ownership protocols

Every completion submission has a shared transaction plan. Preparation reserves
separate caller and native callback roots. Rejection releases both, and native
release retires its root after callback quiescence. Capture copies borrowed
payloads, and delivery adopts owned payloads transactionally. Abandoned delivery
disposes resources that the caller did not adopt. Managed bindings dispose
unclaimed map and runtime owners through the native disposal entry points, and
explicit close returns the observed native teardown completion.

An owned immediate output stays separate from the completion payload. The plan
identifies the parent input, reserves that parent before submission, and adopts
the output on native acceptance. A failed attachment completion keeps its
immediately returned session owner. Session cleanup abandons the target before
CPU-only destruction. A failed cleanup keeps the owner for retry or for the
binding's cleanup-failure channel. Strong child-to-parent references preserve
parent lifetime through that cleanup. Frame release keeps its consumer
synchronization argument and consumes ownership only on success.

Resource-provider callbacks declare their decision handle, both decision values,
and request operations in the header, and resolution checks their types and
relationships. Entering completion or release commits the callback to handling
the request, including when completion returns an error. Pass-through
invalidates the provisional owner. Request release waits for in-flight request
calls and retires cancellation roots after native release returns. The
decision's `wait_retired` relationship is verified: the compiler rejects a
different receiver, extra parameters, a consuming operation, and callback
reentry into that wait. A cancellation registration's boolean output identifies
the already-cancelled path, which keeps no callback.

Direct registrations identify their context and either a release callback or
owner release. The value model carries callback registration descriptors through
nested input records. Operations keep the input parameter and the field path
separately, so a wake inside attachment options uses the same acceptance and
quiescence transaction as a top-level registration. Callback plans identify
their context parameter explicitly.

Native callback adapters declare their callback signature, context record, and
invoked callback protocol operations, and the compiler checks these
relationships. A backend credits a callback-only setter through an adapter only
when its emitted descriptor factory installs that adapter. Native release owns
the retained arenas and adopted queue handles of an adapter, even after the
receiving isolate exits. Release messages use unique registration identifiers,
so a reused allocation address cannot retire a newer host root.

## Callback scope and reentry

Callback-scoped responses carry their context field, permitted setters, and
callback types in `CallbackResponsePlan`. Bindings keep the original native
pointer and expire the wrapper when the callback returns. Callback `reentry`
metadata carries native call restrictions. Logging and logging-release callbacks
forbid native reentry while they execute.

Selective reentry policies identify the exact permitted owner and operations. A
cancellation registration captures its request identity, and its callback may
release that request without waiting for its own retirement. Bindings intersect
nested callback policies, so an inner callback cannot widen an outer
restriction.

## Borrowed views and arrays

Acquired-frame texture and producer-sync accessors declare `view_owner=frame`.
Their resolved borrowed-view plan retains the frame and names the release,
disposal, and abandonment boundaries of the owner and its ancestors. Bindings
scope the whole GPU descriptor, including integer resource IDs, and expire it
after successful consuming operations.

Native borrowed-frame scopes use the handle's verified `view_begin` and
`view_end` relationship. A successful begin retains the frame and its session
without allocation. Bindings end the scope in `finally` after synchronous host
use. Native disposal invalidates later scopes immediately and waits for active
session scopes before retiring graphics resources. Explicit frame release and
session abandonment return a busy status while scopes are active, so a callback
cannot deadlock by closing its own scope. A destroy call after asynchronous
abandonment waits for abandonment to finish. Reentry from the callbacks of that
abandonment returns a busy status, because the callback must return before
teardown can finish.

Strided arrays carry their byte-stride field in `ValuePlan.stride`. An
`ItemBufferPlan` links an array item's message offset and length to the counted
message arena of its batch. Bindings check slice bounds before decoding and copy
items using the declared stride. Encoded character and byte pointers resolve as
buffers that keep their declared UTF-8, JSON, or bytes representation. Strided
and arena-backed records support output copies, and input use fails compilation.

Adapter record `projection` metadata identifies the public value copied into an
adapter queue. Validation checks that every source field except the ABI size and
reserved storage keeps its name and C type. `ValuePlan.projection` provides the
public enums, presence groups, and buffer contracts to the generated adapter
decoder.

`handle_access=issued` keeps access to an issued generation identifier for
quiescence waits after owner release, and other operations require a live owner.
Tagged union fields can declare `empty_variant` for a verified tag value with no
payload, and unknown tags stay separate from that empty case. Enum plans include
the declared underlying integer type, with its width and signedness.

## Compiler verification

Header mutations introduce nested arrays, masked values, tagged unions, owned
results, and registered callbacks. A mutation changes every applicable output
without a handwritten function entry. Invalid lifetime, count, and tag
relationships fail with the declaration and its source location. Changing a
nested borrowed field, count relationship, union variant, ownership annotation,
or callback lifetime either changes generated behavior or rejects the header.
[Native ownership](core-design.md#behavioral-validation) lists the lifecycle
behavior that the binding and native suites exercise.
