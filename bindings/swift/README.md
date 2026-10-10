# MapLibre Native Swift binding

The Swift API is generated from the C headers. A command is an `async` method
that returns its terminal disposition and committed generation as a
`CommandCompletion`; a query is an `async` method that returns a copied result.
Published snapshots return their result synchronously.

An `async` operation runs on its caller's executor until it first suspends, and
it submits its work to native before that suspension. Operations that one actor
starts in order therefore reach native in that order. This holds for tasks that
the actor starts with `Task {}`, because the actor runs them in the order that
it enqueued them, and for `Task.immediate`, which submits before it returns. The
module builds with the `NonisolatedNonsendingByDefault` feature to give its
operations this behavior, so the package needs Swift 6.2 or later.

Cancelling a task that awaits an operation ends the wait with
`CancellationError`. The native work still finishes, and the binding disposes a
value that arrives afterwards, such as a created map.
