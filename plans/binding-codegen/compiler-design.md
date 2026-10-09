# Compile the C API into eight bindings

The binding compiler resolves the C declarations and their contracts into one
typed semantic model. Static language emitters generate public values,
operations, conversions, and callback trampolines from that model. Handwritten
runtimes implement host scheduling and memory primitives. A header change that
uses an established semantic rule requires regeneration and validation across
all eight bindings.

Correctness and performance have equal priority. Readability follows them, then
usability and implementation size. Existing public spellings and internal layers
may change when they obstruct a simpler safe model.

## Semantic model and lowering

The existing `Api` model preserves C syntax and typedef identity. It remains the
frontend representation. A separate resolver consumes that model and validates
all cross-declaration relationships once. Language emitters consume resolved
contracts; they do not infer ownership from names or maintain lists of supported
handles and functions.

The semantic model has these components:

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
pointer whose storage or length cannot be derived from a contract.

Recursive value plans describe both input materialization and output capture.
Optional fields become language optional values. Tagged unions become language
sum types. Unknown open enum values retain their integer representation.
Defaults come from the declared C initializer when one exists. Reserved storage
and structure sizes remain generated implementation details.

An operation plan identifies ordered actions for preparation, submission,
acceptance, rejection, completion, delivery, and abandoned delivery. It records
resources, their owners, and the transition that transfers each resource.
Emitters translate those actions into direct calls and structured cleanup in the
target language. The plan is compiler data, never a runtime interpreter.

Snapshots use immediate value capture and preserve the generation published with
the state. A snapshot operation remains synchronous. Asynchronous queries copy
borrowed data before the C callback returns. Command completions retain the
terminal disposition, status, diagnostic, and generation, including failure.

Handle creation has two independent outputs when the C contract requires them:
an immediately accepted handle and a later attachment completion. A generated
wrapper reserves parent ownership before submission, adopts only accepted
handles, and rolls back on rejection. A close consumes a wrapper only after
native acceptance. A discarded owned result uses its declared disposal protocol;
cleanup failure retains an observable owner instead of silently losing it.

## Runtime boundary

The runtime contains mechanisms whose implementation depends on the language:
completion roots, scheduling, native library loading, handle state, allocation,
thread attachment, and exception containment. Generated code supplies the
operation-specific converters, callback signatures, and lifecycle transitions.
Adding a camera field, source callback, render descriptor, or result type must
not add a handwritten runtime case.

| Binding | Stable mechanism and generated boundary                                                                                                                                                                                                                                                            |
| ------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Rust    | Typed futures, ownership state, allocation, and panic containment remain runtime code. Generated direct C calls and recursive conversions replace field-by-field core adapters. Owned result cleanup is explicit.                                                                                  |
| Swift   | Continuation delivery, retained callback roots, and synchronized handle state remain. Generated methods call C directly; native relay types and duplicate value representations can be removed.                                                                                                    |
| Zig     | Allocator-aware futures and synchronization remain. Generated values provide recursive disposal, including handles; abandoned creation results require an allocation-free release submission.                                                                                                      |
| Go      | Completion channels, cgo handle tokens, and callback ingress remain. Generated C trampolines and typed Go converters honor cgo pointer lifetime rules. Retained native storage contains tokens rather than Go pointers.                                                                            |
| .NET    | Task completion, GCHandle roots, handle state, and unmanaged callback ingress remain. Generated partial classes and static converters avoid reflection and dynamic dispatch over records.                                                                                                          |
| Kotlin  | Common public types and operations are generated once. JVM FFM, Android JNI, and Kotlin/Native emit direct platform calls over a shared semantic plan. Arenas, JNI/thread attachment, stable references, and continuation delivery remain platform runtime mechanisms.                             |
| Python  | PyO3 handles native callback ingress and releases the GIL around calls that can wait for callbacks. Generated native methods, Python methods, values, and stubs share one plan. Redundant Python forwarding may be removed when PyO3 preserves the public asynchronous and introspection contract. |
| Dart    | Native code captures borrowed callback storage before posting to an isolate. Generated native copiers and Dart decoders share value semantics. Listener tokens, isolate delivery, and native record release remain runtime mechanisms.                                                             |

Every foreign callback contains host exceptions or panics. Callback replacement
keeps the prior root alive until native invokes its release callback. Native
release determines quiescence; completion alone does not establish it. Reentrant
callbacks do not execute host code while a binding handle-state lock is held.
Blocking native teardown and driver service release the Python GIL whenever
native work may enter Python. Coroutine or future cancellation ends host
waiting; it does not revoke accepted native ownership.

Dart uses generated static copy functions for each reachable completion value. A
measuring pass computes checked, aligned storage, then one allocation holds the
record and all copied payload bytes. A writing pass copies each borrowed byte
once and rewrites pointers into that allocation. The generated dispatcher
selects a compiled copier; it does not interpret a type descriptor. Nested
arrays and union variants follow the same recursive rules as other bindings. The
direct callback bindings retain their immediate capture path.

## Implementation and verification

The compiler work divides into semantic resolution, runtime ownership, native
capture, and language rendering. These partitions share the typed contracts and
operation plans. Each migrated semantic category replaces production methods,
values, and conversions together, then removes the superseded layers.

The semantic resolver must account for every public declaration as a generated
binding operation, a generated value or callback, or a deliberate runtime
primitive. A runtime classification requires a mechanism-level reason. Renaming
an unsupported operation as runtime infrastructure does not complete coverage.

Validation uses header mutations that introduce nested arrays, masked values,
tagged unions, owned results, and registered callbacks. Mutations must change
all applicable outputs without a handwritten function entry. Invalid lifetime,
count, and tag relationships must fail with the declaration and source location.
Representative behavioral tests exercise rejection rollback, completion before
submission returns, callback replacement, conversion failure, abandoned owned
results, close rejection, and retained snapshots after source destruction.

Native capture tests destroy or overwrite source buffers before reading the
copied record. Allocation failure and overflow tests verify cleanup without
partial delivery. Runtime tests verify single release and callback quiescence.
Each binding's existing integration suite runs through the generated production
surface. Platform execution evidence remains separate from source compilation
and from tests linked against the parent checkout's native library.

## Native capture implementation

The native callback adapter now uses generated static capture functions from the
shared semantic plans. Its record and all copied payload storage occupy one
allocation. Type identifiers are deterministic hashes of the native type and
ownership. The generated dispatcher replaces the handwritten per-result switch
and the separate containers for strings, arrays, and result records.

Owned handle results remain owned by the native record until the Dart decoder
constructs a host owner and adopts the record. Discarded records and failed
captures invoke the declared disposal operation. Owned root arrays dispose every
element. Nested owned handle fields remain an explicit unsupported contract;
generation rejects them before any array or pointer conversion.

Two compiled capture tests verify recursive header mutation, source-buffer
independence, mask-controlled pointer access, invalid union tags, overflow,
allocation failure, handle adoption, owned-array disposal, and the distinction
between absent and present-empty buffers and arrays. The allocation check counts
exactly one allocation for a nested result. The shared generator suite passes 55
tests. Dart's 85 integration tests and static analysis pass against this
worktree's rebuilt macOS ARM64 Metal library.

The host microbenchmark `tests/bindgen/capture_benchmark.cpp`, which commit
`0de5d956a` added and the branch does not keep, captures 64 layer records, each
containing four 64-byte strings. Seven samples each run 20,000 iterations.
Median time fell from 5,717 ns with the previous native adapter to 1,473.2 ns
with generated capture, about 74% lower. This measures adapter creation,
copying, listener delivery, and destruction on this host; it does not measure
map rendering or other platforms.

## Creation-result ownership

Managed cleanup for unclaimed map and runtime owners uses the native disposal
entry points in .NET, Python, Rust, and Swift. Explicit close still returns the
observed native teardown completion. Zig futures now dispose unclaimed map and
projection results, and Kotlin cancellation disposes a newly constructed map
without creating another future.

Go owned creation futures keep adoption state with their shared result storage.
Only the first successful `Await` transfers responsibility to the caller;
subsequent awaits return the same owner. A finalizer disposes a completed result
that no caller received. Attaching that finalizer to shared storage preserves
copied future wrappers. Native callback retention keeps that storage alive until
delivery and callback release finish.

Host integration checks pass on macOS ARM64 Metal: .NET 209, Python 218, Rust
243, Swift 130, Dart 85, Kotlin JVM 187 and Native 211, and Zig 194. Python
skips 23 tests and Zig skips four tests for other native backends. Go's uncached
package suites and vet pass. These checks cover creation ownership; they do not
establish arbitrary graphics-attachment graph finalization.

## Resolved ownership protocols

Every completion submission now has a shared transaction plan. Preparation
reserves separate caller and native callback roots. Rejection releases both;
native release retires its root after callback quiescence. Capture copies
borrowed payloads, and delivery adopts owned payloads transactionally. Abandoned
delivery disposes resources that the caller did not adopt. These actions are
compile-time instructions for language emitters.

An owned immediate output remains separate from the completion payload. The plan
identifies the parent input, reserves that parent before submission, and adopts
the output on native acceptance. A failed attachment completion retains its
immediately returned session owner. Session cleanup abandons the target before
CPU-only destruction. A failed cleanup retains the owner for retry or the
binding's cleanup-failure channel. Strong child-to-parent references preserve
parent lifetime through that cleanup. Frame release retains its consumer
synchronization argument and consumes ownership only on success.

Resource-provider callbacks declare their decision handle, both decision values,
and request operations in the header. Resolution checks their types and
relationships. Entering completion or release commits the callback to handling
the request, including when completion returns an error. Pass-through
invalidates the provisional owner. Request release waits for in-flight request
calls and retires cancellation roots after native release returns. Direct
registrations also identify their context and either a release callback or owner
release. Cancellation registration's boolean output identifies the
already-cancelled path that retains no callback.

Four semantic tests introduce renamed protocol declarations and mutate parent,
release, callback, decision, and cancellation relationships. They verify that
attachment failure preserves ownership and that frame synchronization remains a
required release input. Disposal support classification follows the declared
handle consumer; release operations remain public operations.

## Native graph retirement and interface boundaries

Session disposal consumes the public owner and schedules abandonment on a
cleanup lane reserved during attachment. Retirement waits for an existing driver
call, quarantines graphics resources, and destroys CPU ownership. Frame disposal
uses that same session retirement because a discarded frame supplies no consumer
GPU completion. Explicit frame release still requires the declared
synchronization. Map disposal accepts attached sessions and waits for their
child leases; runtime disposal waits for map cleanup. Session destruction admits
one closer before joining its worker or releasing wake state.

The header entrypoint manifest separates the public C interface from the native
binding-runtime interface. Clang resolves both include closures and preserves
all 288 exports in the inventory: 264 public functions and 24 runtime adapter
functions. Any declaration outside both closures fails validation. Shared
includes retain their public classification. Runtime adapters remain resolved
compiler inputs and are reported separately from generated public methods.

### Nested registrations and borrowed GPU views

The value model carries callback registration descriptors through nested input
records. Operations retain the input parameter and the field path separately, so
a wake inside attachment options uses the same acceptance and quiescence
transaction as a top-level registration. Callback plans identify their context
parameter explicitly.

Acquired-frame texture and producer-sync accessors declare `view_owner=frame`.
Their resolved borrowed-view plan retains the frame and names owner and ancestor
release, disposal, and abandonment boundaries. Bindings scope the whole GPU
descriptor, including integer resource IDs, and expire it after successful
consuming operations.

Dart owner finalizers call disposal dispatch generated from handle metadata. A
native Dart-port wake adapter posts integer notifications through native API
version 2; a closed receiver port discards notifications safely during isolate
teardown. The adapter owns its descriptor context until native release.

Callback-scoped responses carry their context field, permitted setters, and
callback types in `CallbackResponsePlan`. Bindings keep the original native
pointer and expire the wrapper when the callback returns. Callback `reentry`
metadata carries native call restrictions; logging and logging-release callbacks
forbid native reentry while they execute.

Strided arrays carry their byte-stride field in `ValuePlan.stride`. An
`ItemBufferPlan` links an array item's message offset and length to its
containing batch's counted message arena. Bindings check slice bounds before
decoding and copy items using the declared stride. Encoded character and byte
pointers resolve as buffers, preserving their declared UTF-8, JSON, or bytes
representation.

Adapter record `projection` metadata identifies the public value copied into an
adapter queue. Validation checks that every source field except ABI size and
reserved storage retains its name and C type. The resolved
`ValuePlan.projection` provides the public enums, presence groups, and buffer
contracts to the generated adapter decoder. Strided and arena-backed records
currently support output copies; input use fails compilation until an encoder
can reconstruct their storage.

Native borrowed-frame scopes use the handle's verified `view_begin` and
`view_end` relationship. A successful begin retains the frame and its session
without allocation. Bindings end the scope in `finally` after synchronous host
use. Native disposal invalidates later scopes immediately and waits for active
session scopes before retiring graphics resources. Explicit frame release and
session abandonment return `BUSY` while scopes are active, so a callback cannot
deadlock by closing its own scope. Map and runtime retirement already waits for
the attached session to detach. A destroy call after asynchronous abandonment
waits for abandonment to finish. Reentry from that abandonment's callbacks
returns `BUSY`, because the callback must return before teardown can finish.

`handle_access=issued` preserves access to an issued generation identifier for
quiescence waits after owner release. Other operations require a live owner.
Tagged union fields can declare `empty_variant` for a verified tag value with no
payload; unknown tags remain separate from that empty case. Enum plans include
the declared underlying integer type, including its width and signedness.

Native callback adapters declare their callback signature, context record, and
invoked callback protocol operations. The compiler checks these relationships. A
backend credits a callback-only setter through an adapter only when its emitted
descriptor factory actually installs that adapter. Dart's transform and resource
rule factories use this path. Native release owns their retained arenas and
adopted queue handles, even after the receiving isolate exits. Release messages
use unique registration identifiers so reused allocation addresses cannot retire
a newer host root.

Dart notification stubs are generated from registration descriptors with void
callbacks and recursively copyable scalar records. Each descriptor field has a
stable message identifier. Native code copies the arguments into a Dart port
message; a shared release callback closes the native context after callback
quiescence. Descriptor storage is needed only through admission. Host callbacks
run in the registration zone, and owner close waits for the release messages.
The generated fixture compiles a newly declared record callback and verifies
both native message contents and the emitted Dart registration.

Dart completion messages transfer copied results through the VM's native-pointer
message finalizer. Undelivered messages dispose unclaimed result owners. Logging
and provider queues use static native wake callbacks, and request cancellation
retains its native port context in the request through callback quiescence.
These paths remain callable after isolate shutdown without executable Dart
callback addresses.

Selective callback reentry policies identify the exact permitted owner and
operations. A cancellation registration captures its request identity; its
callback may release that request, but cannot wait for its own retirement.
Bindings intersect nested callback policies so an inner callback cannot widen an
outer restriction.

Kotlin generates the full public operation surface for JVM, Android, and Native.
Generated owner calls retain their wrapper through native return with a platform
reachability fence. Synchronous copies of owner-backed buffers and event batches
reserve the owner while copying; a concurrent close returns `BUSY` and remains
retryable. Callback roots belong to the reachable owner, and the native callback
registry keeps weak references to those roots. Native release clears accepted
roots after callback quiescence.
