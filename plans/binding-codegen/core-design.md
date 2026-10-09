# Native ownership for generated bindings

Generated bindings keep the borrowed completion ABI for runtimes that can
execute a native callback. Hosts that receive callbacks later use generated
native capture functions. Handle creation reserves native retirement work, so
abandoning an owned creation result consumes its handle without allocating a
future or native submission state.

## Decisions and costs

| Candidate                            | Consequence                                                                                                                                          | Decision                                                                   |
| ------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| Universal owned completion results   | Every scalar and command gains allocation or reference counting, including runtimes that copy inline.                                                | Keep borrowed results.                                                     |
| Runtime reflection for nested copies | One interpreter handles records, but traverses descriptors and trusts offsets on every copy.                                                         | Generate typed capture routines and use a small shared allocation runtime. |
| Static no-op completion for disposal | Removes binding future allocation; native map and runtime release still allocate gates, operation state, queue entries, and a runtime waiter thread. | Insufficient for abandoned ownership.                                      |
| Reserved retirement work             | Adds bounded state per map and runtime, and one executor wake per runtime; disposal enqueues existing nodes.                                         | Use for abandonment.                                                       |
| Rebuild callback registrations       | Adds native churn where acceptance and release already establish ownership.                                                                          | Generate callback roots from existing contracts.                           |

Completion delivery accepts inline execution and releases its callback state
only after delivery returns. A generated submission reserves both caller and
native references before crossing the ABI. Rejection drops both references
locally, and acceptance lets the native release callback retire its reference.
Cancelling a host awaiter changes result ownership, and leaves the native
callback lifetime unchanged.

Custom source registration distinguishes pending, accepted, and adopted callback
ownership. Runtime resource registration retains shared callback state, replaces
it under a lock, and releases the old registration outside that lock. Header
registration annotations and generated roots describe both mechanisms on top of
the existing callback scheduler.

## Ownership boundaries

A typed result contract identifies owned handles independently from the borrowed
storage that contains their numeric IDs. Map creation delivers an owned handle
through its completion, so a failed adapter copy or an abandoned host future
disposes that handle.

A generated capture owns all transferred resources until the binding adopts
them. Destroying an unclaimed capture disposes its resources. Conversion uses a
transactional ownership guard: successful wrapper construction transfers each
owned field, and failure releases only the fields that remain unclaimed. Nested
borrowed strings, slices, and active union members are copied from the same
plans that lower inputs. Capture code checks length multiplication and alignment
before allocating. Immutable copied bytes need no global registry or lock.

A handle typedef declares its release, parent, and disposal operations. For
example, `mln_map` declares
`kind=handle;release=mln_map_release;parent=mln_runtime;dispose=mln_map_dispose`.
`tools/bindgen/schema.py` defines the accepted keys.

The disposal ABI consumes valid handles synchronously and performs native
retirement asynchronously. Map and runtime disposal accept live children and
wait for their cleanup, so garbage collectors may finalize parents before
children. Consumed parents reject public calls while private registry entries
support child cleanup. An attached session holds a lease on its map until
renderer workers quiesce and the attachment retires. The release completions
remain the observable cleanup operations.

Retirement uses an intrusive work node embedded in the object, a self-reference
that survives handle-table removal, and a wake reserved when the runtime starts.
The runtime worker cancels pending map work and waits for submission leases
before destroying map state. Blocking pool shutdown runs on a dedicated map
teardown lane, and the map's runtime cleanup lease survives this shutdown.
Runtime retirement waits for children, earlier operations, and cleanup leases.
It then removes its private registry entry, retires callbacks on the executor,
and joins the executor from a separate reserved lane. The separate lanes prevent
runtime retirement from waiting on map cleanup queued behind it.

A render attachment produces a session before its completion. Its wrapper owns
that handle through failure, cancellation, and abandonment. Native CPU-side
abandonment and session destruction are distinct from graphics detach.
Abandonment can quarantine resources, while detach needs driver progress, so a
generic handle finalizer cannot promise graceful graphics destruction. Acquired
frames carry consumer GPU synchronization. Their disposal abandons the session
and quarantines its target, because a discarded frame supplies no consumer
synchronization. Session disposal reserves its retirement node and cleanup
worker at attachment, and admission stays allocation-free during an active
driver call. Event batches use an owned handle with an immutable borrowed view.

## Behavioral validation

Tests exercise rejected registration, accepted installation failure, replacement
while a callback is active, and callback reentrancy during release. An inline
completion test drops the host future before native release. Creation tests
abandon the future before and after delivery, then confirm that the parent can
retire. Allocation-failure tests inject failure after object creation and show
that eligible disposal consumes the handle and retires its callback state. Map
retirement runs with outstanding operations, and runtime retirement runs while
map pool cleanup is pending.

Capture tests invalidate the original nested storage before decoding the
captured record. They cover both offline region union variants, and fail
conversion after an owned handle enters the record. An unclaimed or partially
decoded record releases each owned handle exactly once. Graphics tests cover
attach failure, caller-driver abandonment, acquired-frame synchronization, and
target loss, which compile-only coverage cannot establish.

## Boundaries of the guarantee

The allocation-free guarantee covers disposal admission and reserved scheduling.
Native backend destruction can still invoke platform code, host release
callbacks, or destructors that allocate. Allocation-failure tests report that
boundary and make no claim about the external graphics stack.
