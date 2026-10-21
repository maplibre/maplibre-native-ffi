# MapLibre Native Swift binding

The Swift API is generated from the C headers. A command is an `async` method
that returns its terminal disposition and snapshot generation as a
`CommandCompletion`; a query is an `async` method that returns a copied result.
Published snapshots return their result synchronously.

An `async` operation runs on its caller's executor until it first suspends, and
it submits its work to native before that suspension. Operations that one actor
awaits in order therefore reach native in that order. Tasks that start
operations reach native in the order that their executor runs them:

- `Task.immediate` runs its operation until the first suspension before it
  returns, so tasks that one actor starts this way submit in the order that it
  starts them. It needs the macOS, iOS, or tvOS 26 runtime, or Swift 6.2 on
  Linux.
- `Task {}` enqueues its operation on the actor. The main actor runs its queue
  in order. Another actor runs tasks of equal priority in the order that it
  enqueued them, and it can run a task of higher or escalated priority first.

The module builds with the `NonisolatedNonsendingByDefault` feature to give its
operations this behavior, so the package needs Swift 6.2 or later.

Start a command group's begin, its commands, and its end as tasks on one actor,
and then await their completions. The group holds the map's render updates until
its end, so a host that awaits each command before it starts the next leaves its
render targets unchanged until every one of those completions arrives.

Cancelling a task that awaits an operation ends the wait with
`CancellationError`. The native work still finishes, and the binding disposes a
value that arrives afterwards, such as a created map.

Callbacks are non-throwing. A callback can call the C API through the binding;
native reports an invalid-state status where reentry is unsafe. The binding
reports callback decoding failures and leaked handles to standard error. When
the last reference to an open handle goes away, the binding disposes the handle
and reports whether disposal succeeded.
