# MapLibre Native Go binding

The Go API is generated from the C headers. Commands return a future with their
terminal disposition and committed generation; queries return a future with a
copied result. Published snapshots return their result immediately.

Create a runtime with `RuntimeCreate` and a map with `RuntimeHandle.MapCreate`.
Closing either handle returns a future for native teardown. Keep servicing a
caller-driver render session while its attachment or detachment is pending.

`DrainEvents` and `DrainFrameResults` return batch owners. Read their copied
values, then close the batch. Acquired GPU frames expose callback-scoped views:
use the texture inside `WithOpenglTexture` or the corresponding backend method,
then close the frame with the host's completion synchronization. View methods
reject access after the callback returns or from another OS thread.

Callbacks receive copied values and scoped response objects. Their generated
registration code retains Go closures until native retirement and enforces the
callback operations declared in the headers. Explicit `Close` orders teardown;
Go cleanup also retires abandoned owners and callback cycles.

Callback admission is per OS thread. While a callback runs, its goroutine is
locked to the native thread and can make only the calls that the callback's
declaration admits. Any other call returns `ErrInvalidState` before it reaches
native. A goroutine that the callback starts runs on another thread and has no
such restriction. The callback must return without waiting on that goroutine's
work, because native work can depend on the callback's return.

A resource provider that returns `ResourceProviderDecisionHandle` keeps its
`ResourceRequestHandle`, and any goroutine can complete and close that handle
after the provider returns. A scoped response such as
`ResourceTransformResponseScope` works only during its callback and on the
callback's thread.
