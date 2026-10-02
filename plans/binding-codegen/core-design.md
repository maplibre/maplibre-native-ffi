# Native ownership for generated bindings

Generate typed operations and value conversion from the public header contracts.
Keep the borrowed completion ABI for runtimes that can execute a native
callback. Use generated native capture functions for hosts that receive
callbacks later. Reserve native retirement work during handle creation, so
abandoning an owned creation result can consume its handle without allocating a
future or native submission state.

## Decisions and costs

| Candidate                            | Consequence                                                                                                                                          | Decision                                                                   |
| ------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| Universal owned completion results   | Every scalar and command gains allocation or reference counting, including runtimes that copy inline.                                                | Keep borrowed results.                                                     |
| Runtime reflection for nested copies | One interpreter handles records, but traverses descriptors and trusts offsets on every copy.                                                         | Generate typed capture routines and use a small shared allocation runtime. |
| Static no-op completion for disposal | Removes binding future allocation; native map and runtime release still allocate gates, operation state, queue entries, and a runtime waiter thread. | Insufficient for abandoned ownership.                                      |
| Reserved retirement work             | Adds bounded state per map/runtime and one executor wake per runtime; disposal can enqueue existing nodes.                                           | Use for abandonment.                                                       |
| Rebuild callback registrations       | Adds native churn where acceptance and release already establish ownership.                                                                          | Generate callback roots from existing contracts.                           |

Completion delivery accepts inline execution and releases its callback state
only after delivery returns. A generated submission reserves both caller and
native references before crossing the ABI. Rejection drops both references
locally; acceptance lets the native release callback retire its reference.
Cancellation of a host awaiter changes result ownership, not native callback
lifetime.

Custom source registration already distinguishes pending, accepted, and adopted
callback ownership. Runtime resource registration retains shared callback state,
replaces it under a lock, and releases the old registration outside that lock.
These mechanisms need explicit registration annotations and generated roots.
They do not require a new callback scheduler.

## Ownership boundaries

A typed result contract must identify owned handles independently from the
borrowed storage that contains their numeric IDs. Native map creation currently
transfers its pending handle before invoking the completion. A failed adapter
copy or an abandoned host future must therefore dispose that handle. The
existing Dart adapter returns a null record on allocation failure and has no
owned-result cleanup. Generating its existing switch verbatim preserves that
leak.

A generated capture owns all transferred resources until the binding adopts
them. Destroying an unclaimed capture disposes its resources. Conversion uses a
transactional ownership guard: successful wrapper construction transfers each
owned field, and failure releases only fields that remain unclaimed. Nested
borrowed strings, slices, and active union members are copied from the same IR
that lowers inputs. Capture code checks length multiplication and alignment
before allocating. Immutable copied bytes need no global registry or lock.

Representative contracts are `owned_handle=mln_map`, `dispose=mln_map_dispose`,
`registration=options`, `acceptance=submission_ok`,
`release=options.release_user_data`, `count=tile_url_count`, and
`variant=definition.type`. Exact spellings belong to the semantic schema.

The disposal ABI consumes valid handles synchronously and performs native
retirement asynchronously. Map and runtime disposal accept live children and
wait for their cleanup, so garbage collectors may finalize parents before
children. Consumed parents reject public calls while private registry entries
support child cleanup. An attached session owns a map child lease until renderer
workers quiesce and the attachment retires. The existing release completions
remain the observable cleanup operations.

Retirement uses an intrusive work node embedded in the object, a self-reference
that survives handle-table removal, and a wake reserved when the runtime starts.
The runtime worker cancels pending map work and waits for submission leases
before destroying map state. Blocking pool shutdown runs on the existing map
cleanup lane. The map's runtime cleanup lease survives this shutdown. Runtime
retirement waits for children, earlier operations, and cleanup leases, removes
its private registry entry, retires callbacks on the executor, then joins it
from a separate reserved cleanup lane. The separate lanes prevent runtime
retirement from waiting on map cleanup queued behind it.

A render attachment produces a session before its completion. Its wrapper must
own that handle through failure, cancellation, and abandonment. Native CPU-side
abandonment and session destruction remain distinct from graphics detach:
abandonment can quarantine resources, while detach needs driver progress. A
generic handle finalizer cannot promise graceful graphics destruction. Acquired
frames carry consumer GPU synchronization. Their disposal operation abandons the
session and quarantines its target instead of inventing consumer
synchronization. Session disposal reserves its retirement node and cleanup
worker at attachment; admission remains allocation-free during an active driver
call. Event batches can use a simpler owned handle plus immutable borrowed view
lifetime.

## Behavioral validation

Tests must exercise rejected registration, accepted installation failure,
replacement while a callback is active, and callback reentrancy during release.
An inline completion test must drop the host future before native release.
Creation tests abandon the future before delivery and after delivery, then
confirm that the parent can retire. Inject allocation failure only after object
creation and prove that eligible disposal consumes the handle and retires its
callback state. Exercise map retirement with outstanding operations and runtime
retirement while map pool cleanup remains pending.

Capture tests invalidate original nested storage before decoding the captured
record, cover both offline region union variants, and fail conversion after an
owned handle enters the record. An unclaimed or partially decoded record must
release each owned handle exactly once. Graphics tests cover attach failure,
caller-driver abandonment, acquired-frame synchronization, and target loss;
compile-only coverage cannot establish those behaviors.

The guarantee concerns disposal admission and reserved scheduling. Native
backend destruction can still invoke platform code, host release callbacks, or
allocator-using destructors. Allocation-failure tests must report that boundary
and must not label the entire external graphics stack allocation-free.
