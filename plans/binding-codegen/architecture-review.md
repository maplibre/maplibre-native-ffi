# Shared binding compiler

Generate every mechanical binding from a validated semantic model of the C
headers. Preserve the existing native executor and direct callback paths. The
compiler resolves ownership, presence, conversion, and lifecycle behavior once;
language backends render those decisions as ordinary compiled code.

## Architecture and native contracts

The prototype's compiler frontend is reusable. Its `Api` model describes C
syntax, and its schema checks annotation vocabulary. Its eight emitters each
decide what a record means, which operations belong to an owner, and how results
cross a callback. Their separate `SCALARS`, `OWNERS`, and result classifiers
explain why production coverage differs between bindings. Extending those
classifiers independently would multiply the remaining ownership decisions.

Introduce a shared semantic pass after extraction and before rendering:

1. Resolve typedef identity, enum domains, defaults, counted pointers, presence
   masks, tagged unions, and foreign pointer values into recursive value plans.
2. Resolve parameters, receivers, outputs, completion payloads, and ownership
   transitions into operation plans.
3. Resolve callback registration, invocation, failure, and retirement into
   callback plans that use each language's small runtime.
4. Emit public values, operations, conversions, callback trampolines, and native
   capture functions from those plans.

Each plan retains the C declaration and source location. A pointer with unknown
length or lifetime produces a compiler diagnostic. A union with an unknown
active-member rule produces a compiler diagnostic. Native foreign handles stay
opaque values and carry their actual backend ownership contract. Binding
backends MUST NOT infer ownership from a function name or a language template.

The completion protocol already supports a common runtime. In
`src/completion/completion.cpp`, acceptance releases a pending inline
completion, rejection drops it without invoking the host, and delivery releases
callback state after the terminal callback returns. Generated code roots state
before submission and transfers the native reference only upon acceptance. The
host reference survives an inline completion until the submission returns.
Cancelling a host wait leaves the native reference live until native retirement.

Owned results need a generated cleanup transaction. It first records every
transferred handle, then constructs public values, then commits their ownership
to the host. Allocation or conversion failure releases each uncommitted handle.
An abandoned future and an unclaimed native capture use the same disposal plan.
`bindings/zig/src/completion.zig` currently calls `deinit` only when the value
type declares it; its map-creation comment explicitly documents a leaked map
when that future is abandoned. A typed disposal operation must replace that
convention. Native disposal must have a defined failure contract: a close that
allocates a completion, queue node, or thread cannot be described as infallible.

Native generational handles and object leases already protect use against
retirement in `src/handles/handle_table.hpp`. Binding handles need close-once
state and acceptance rollback. They do not need a second per-call lease or a
global lock. The old active-use wait requirement in the binding specification
conflicts with its BND-197 rule and should be removed. Graphics service retains
its explicit thread contract; ordinary control calls stay callable from any
thread.

One C callback contract merits simplification. Resource-request cancellation
registers a callback and user data without a release hook, and reports prior
cancellation through an output boolean. Every binding consequently implements
its own invocation and retirement rules. An owned callback descriptor with
native inline delivery and release after the final invocation would use the same
registration protocol as the other callback families. Reentrant release must
still complete without waiting for its own callback.

## Correctness and performance boundaries

Value plans are compiler data. Generated runtime code directly copies fields,
branches on presence and tags, and loops over counted arrays. Scalar values and
fixed records require no reflection, serialization, or heap allocation. Input
storage lasts through native acceptance because native submissions copy their
inputs. Borrowed completion storage is copied before callback return.

Hosts that accept direct callbacks keep that path. Dart requires native capture
before isolate delivery; generate its capture and destruction functions from the
same value plans. Replace the handwritten copy-kind switch and record field
inventory in `src/c_api/callback_adapter.cpp`. A checked size pass followed by a
single aligned allocation can capture a nested borrowed result. Generated static
dispatch selects its compiled copier. Owned handles transfer into that capture
and are disposed if delivery or adoption fails. Other bindings incur none of the
isolate adapter's copying or queue costs.

Count public operations separately from ABI support functions. A default
constructor implements generated input defaults; a buffer view and destroy
implement a generated copied value; a callback response builder implements a
generated trampoline. Each support function requires a verified consumer
relationship. A handwritten label or an unexplained exclusion cannot establish
completion. Platform constraints belong to target capabilities and are checked
alongside the generated public surface.

## Implementation partitions and acceptance

| Partition          | Deliverable                                                                                                                      |
| ------------------ | -------------------------------------------------------------------------------------------------------------------------------- |
| Semantic compiler  | Typed value, operation, handle, and callback plans; relational annotation validation; declaration-level diagnostics.             |
| Native lifecycle   | Disposal that handles abandoned owned results and failed adoption, with actual native allocation and teardown behavior verified. |
| Native capture     | Generated capture, ownership transfer, and destruction for isolate delivery; removal of handwritten result-shape switches.       |
| Language rendering | Public owners, values, operations, and static conversions from shared plans; removal of replaced per-operation wrappers.         |
| Integration        | Reproducible generation, dependency-based coverage, existing binding suites, and representative lifecycle and mutation tests.    |

The compiler tests MUST demonstrate that changing a nested borrowed field, count
relationship, union variant, ownership annotation, or callback lifetime changes
generated behavior or rejects the header. Lifecycle tests MUST cover inline
completion, rejection, asynchronous failure, abandoned creation, failed
adoption, replacement with an in-flight callback, and reentrant retirement.
Native capture tests MUST retain copied results after the callback returns and
release unclaimed owned results. Public integration tests remain the behavior
gate for each of the eight bindings.

The migration is complete when every supported public operation and value is
generated, every ABI support declaration has a checked generated consumer, and
the superseded handwritten operation and conversion code has been removed. Local
tests against the parent's installation establish only the unchanged ABI; native
changes require a new native build and matching binding tests.

## Implemented boundary

The shared compiler now resolves the current declarations into value shapes,
parameter directions, result ownership, callback signatures, and descriptor
retirement hooks. All eight emitters enter through that compiler. Static native
capture and the .NET recursive record conversions consume its value plans.
Counted pointers require an explicit count relationship; a single record
reference requires `length=1`. Presence groups preserve shared mask bits, and
standalone mask flags remain visible in the model.

Resolving an operation's shape does not establish a complete lifecycle protocol
or a generated public wrapper. Resource-provider decisions and inline replies,
request terminal ownership, acquired-frame GPU synchronization, and render
attachment with both immediate and deferred results still need protocol plans
and backend implementations. Registration plans record acceptance and retirement
boundaries, but do not yet generate every registration family. Backend coverage
continues to report unsupported operations. Only a checked default-constructor
relationship currently classifies an operation as compiler support; other ABI
helpers still require a verified consumer before they can leave public coverage.
