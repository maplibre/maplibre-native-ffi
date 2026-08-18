---
title: Asynchronous architecture
description: Design and migration plan for one-shot completion and event streams.
sidebar:
  order: 4
---

The C API uses separate mechanisms for immediate work, one-shot asynchronous
work, event streams, and graphics-thread service. Each mechanism matches the
shape of the work that it carries. The API has no general readiness multiplexer.

## Target design

Immediate functions finish before they return. Creation, release, published
snapshots, detached projection calls, and validation use ordinary return values
and status codes.

One-shot asynchronous functions accept a completion descriptor. A successful
submission copies its inputs and transfers the descriptor's callback state to
the C API. The C API invokes the completion exactly once with terminal status, a
borrowed diagnostic, and the function's typed result. The completion may run
before the submission function returns. The C API invokes the descriptor's
release callback after the completion returns and after no native path can
invoke it again.

Command completions additionally carry committed, superseded, failed, or
cancelled disposition and the map snapshot generation that a commit published.
Commands have no numeric correlation ID and do not publish a second completion
through the runtime event stream.

Bindings copy completion data before the callback returns and resolve the target
language's eager future, promise, task, continuation, or explicit async value.
Cancelling a host wait does not cancel accepted native work unless the C
function explicitly returns a cancellation token.

An accepted command always resolves to a command-completion value. Failed and
cancelled commands carry their disposition, terminal status, and diagnostic in
that value; they do not reject the asynchronous container. Submission rejection
and binding bridge failures reject the asynchronous container because no
accepted command exists to describe.

Repeated observations use an owned ABI-format queue. The owning runtime or
render session invokes its direct wake callback when a queue changes from empty
to nonempty. One drain transfers the complete queue into an independently owned
batch. The wake callback schedules a later host drain and returns promptly; it
does not deliver individual records or call host application code.

Caller-driven render sessions expose a separate driver-work wake callback. The
callback schedules service on the graphics thread. Graphics-thread service is
the only mechanism whose progress depends on a host thread.

These four mechanisms form the complete execution model:

| Work shape                      | C mechanism                              |
| ------------------------------- | ---------------------------------------- |
| Finishes during the call        | Return value and status                  |
| Finishes once after the call    | Owned completion callback                |
| Produces repeated records       | Owned queue, direct wake, and full drain |
| Requires a host graphics thread | Direct driver-work wake and service      |

## Migration plan

1. Define the common completion ownership contract and typed completion result
   shapes in the public C headers.
2. Convert runtime and map commands from command IDs and command-finished events
   to command completions.
3. Convert ordered queries, lifecycle transitions, offline work, and render work
   from public operation handles to typed completions.
4. Keep cancellation handles only for work whose public contract supports
   cancellation after submission.
5. Give runtime events, frame results, and caller-driver work direct owner
   callbacks. Remove notification sources, ready endpoints, and ready batches.
6. Remove operation polling, waiting, status inspection, diagnostic copying,
   finish, release, and typed take functions after their callers migrate.
7. Map completions to each binding's ordinary eager asynchronous value. Keep raw
   C callback declarations private.
8. Update examples to schedule only the continuation that consumes a result;
   submission itself remains immediate and preserves runtime order.
9. Delete command correlation registries, operation waiter maps, notification
   dispatchers, and readiness adapters from every binding.
10. Verify synchronous rejection, inline completion, later completion,
    cancellation of a host wait, native teardown, callback release, full event
    drains, frame-result drains, and graphics-thread wakeups.

The migration is complete when no public binding exposes command IDs, operation
handles, notification sources, or ready endpoints, and when native one-shot work
requires no host event drain to complete.
