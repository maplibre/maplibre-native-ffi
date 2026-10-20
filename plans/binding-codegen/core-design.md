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
is reachable. Teardown may allocate. Allocation failure after acceptance
terminates the process.

Map retirement first cancels pending still images and closes camera transitions
on the runtime worker. It waits for children and submission leases, including a
command that leased the map before close and queued afterwards. A final worker
stage closes any transitions that those commands started, destroys the native
map, and releases map-owned callback state before reporting release completion.
Pool shutdown then runs on the map cleanup lane. The map retains a runtime
cleanup lease until its resources and pool have retired.

A runtime submission holds its runtime lease until its completion and release
callback return, and a discarded command holds it until its work has run. A
queue-mode completion retires once its result is pushed to the queue, or
discarded because the queue was released or the process is exiting; the host's
drain is outside the submission. Completion acceptance precedes worker
publication. Resolution allocates no storage in the completion: a callback runs
inline, and a queue item that cannot be allocated falls back to one reserved at
submission. An operation destroyed without an outcome completes as cancelled.

Runtime retirement waits for children and pending submissions. The worker
removes the private registry entry, releases reachable callback and resource
state, and destroys its run loop. One shared finalizer lane joins the worker,
including thread-local cleanup, before retiring the runtime's self-reference and
reporting completion. Runtime destruction drains its autorelease pool before
that completion. A worker that exits slowly delays other runtimes' release
completions. Release completions must not block. This guarantee covers
runtime-owned state; shared services and the finalizer's return prevent treating
completion as a library-unload boundary. Map pool cleanup remains separate, and
its runtime lease preserves the executor until maps finish.

Render sessions retain their maps until driver calls and attachment cleanup
retire. Graphics detach needs driver progress; abandonment can quarantine
resources. Disposing an acquired frame abandons its session because the frame
supplies no consumer GPU synchronization. Event batches own immutable storage
independently from runtime retirement.

## Validation

Native tests cover callback replacement and reentrancy, abandoned creation
results, pending operations, and map pool shutdown. Ordering tests check
transition closure inside map release completion, commands queued after close
cancellation, runtime release from a map completion, and one runtime's callbacks
waiting for another runtime to retire. Capture tests cover nested borrowed
storage and partially adopted owned results. Graphics tests cover driver
quiescence, attachment failure, abandonment, and acquired frame synchronization.
