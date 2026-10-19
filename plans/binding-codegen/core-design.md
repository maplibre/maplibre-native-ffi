# Native ownership for generated bindings

Generated bindings copy borrowed completion results inline or retain a generated
native capture for later decoding. Completion acceptance transfers the native
callback reference; delivery releases it after the callback returns. Cancelling
a host awaiter changes result ownership while the native callback remains live.

## Results and callback ownership

Typed result contracts distinguish owned handles from the borrowed records that
contain them. A capture owns each transferred handle until the binding adopts
it. Failed conversion disposes unclaimed fields, and destroying an unclaimed
capture disposes every resource it still owns. Generated capture routines copy
nested strings, slices, and active union members, with checked sizes and
alignment.

Handle annotations declare release, parent, and disposal operations. A failed
delivery copy or an abandoned creation result disposes its owned handle.
Callback registrations retain host state from acceptance through replacement or
native retirement. Replacement releases the old registration outside its lock.

## Retirement

Release consumes a handle and reports retirement through a completion. Disposal
consumes it without a completion. Both use the same retirement path. Disposal
accepts live children; private registry entries remain available for their
cleanup while public calls report that the parent is closed.

Each map and runtime embeds a retirement node and retains itself while that node
is reachable. Creation reserves the executor wake and the shared map pool
shutdown lane. Disposal admission and scheduling use those reserved objects;
release also allocates completion state and a tracked runtime submission for a
map. Native destruction and host callbacks may allocate.

Map retirement first cancels pending still images and closes camera transitions
on the runtime worker. It waits for children and submission leases, including a
command that leased the map before close and queued afterwards. A final worker
stage closes any transitions that those commands started, destroys the native
map, and releases map-owned callback state before reporting release completion.
Map release retains its submission sequence until that completion returns, so
later runtime barriers cover it. Pool shutdown then runs on the map cleanup
lane. The map retains a runtime cleanup lease until its resources and pool have
retired.

Runtime retirement waits for children, submission leases, and terminal outcomes
of earlier operations. An operation destroyed without an outcome completes as
cancelled. The runtime worker removes the private registry entry and releases
reachable callback and resource state. It then stops and destroys its run loop,
detaches its thread, retires the runtime's self-reference, and reports release
completion. The worker accesses no executor state after that final stage.
Runtime retirement uses no shared waiting thread, so a blocked runtime callback
cannot prevent another runtime from retiring. Map pool cleanup remains separate
from runtime work, and the cleanup lease preserves the executor until maps
finish.

Render sessions retain their maps until driver calls and attachment cleanup
retire. Graphics detach needs driver progress; abandonment can quarantine
resources. Disposing an acquired frame abandons its session because the frame
supplies no consumer GPU synchronization. Event batches own immutable storage
independently from runtime retirement.

## Validation

Native tests cover callback replacement and reentrancy, abandoned creation
results, disposal with allocation failures, pending operations, and map pool
shutdown. Ordering tests check transition closure inside map release completion,
commands queued after close cancellation, runtime release from a map completion,
and one runtime's callbacks waiting for another runtime to retire. Capture tests
cover nested borrowed storage and partially adopted owned results. Graphics
tests cover driver quiescence, attachment failure, abandonment, and acquired
frame synchronization.
