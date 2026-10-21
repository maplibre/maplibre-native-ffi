# MapLibre Native Go binding

The Go API is generated from the C headers. Commands return a future with their
terminal disposition and snapshot generation; queries return a future with a
copied result. Published snapshots return their result immediately.

Create a runtime with `RuntimeCreate` and a map with `RuntimeHandle.CreateMap`.
Closing either handle returns a future for native teardown. Keep servicing a
caller-driver render session while its attachment or detachment is pending.

`DrainFrameResults` returns a batch owner. Read its copied values, then close
the batch. Register a `RuntimeEventHandler` to receive copied event spans.
`DrainFrameResults` and `AcquireFrame` return nil and no error when nothing is
ready. Acquired GPU frames expose callback-scoped views: use the texture inside
`WithOpenglTexture` or the corresponding backend method, then close the frame
with the host's completion synchronization. View methods reject access after the
callback returns or from another OS thread.

Callbacks receive copied values and scoped response objects. Their generated
registration code retains Go closures until native retirement. Explicit `Close`
orders teardown. Go cleanup also retires abandoned owners and callback cycles,
and logs each owner that it disposes with `slog.Warn` on the default logger,
with the handle type and the handle as attributes.

Native returns `StatusInvalidState` for calls made inside a log or Vulkan
queue-lock callback. Other callbacks may call the library. A callback must wait
only for work that can finish before it returns, such as a submission's
acceptance. Waiting for a submission's completion can deadlock native work that
needs the callback to return.

A resource provider that returns `ResourceProviderDecisionHandle` keeps its
`ResourceRequestHandle`, and a resource transform owns its
`ResourceTransformRequestHandle`. Any goroutine can answer and close either
handle after the callback returns. A scoped response such as
`HttpHeaderTransformResponseScope` works only during its callback and on the
callback's thread.

Native cannot receive a panic, so the binding recovers a callback's panic and
returns the callback's declared failure value to native. The binding logs the
panic with `slog.Error` on the default logger, with the C callback type, the
panic value, and the stack as attributes. Install a handler with
`slog.SetDefault` to route these records. The handler runs on the native
callback's stack.

Errors carry the generated `Status` and a copied diagnostic. Use
`errors.Is(err, StatusInvalidState)` to match a status, or `errors.As` to read
an `*Error` and its diagnostic. Unknown statuses retain their integer value.
