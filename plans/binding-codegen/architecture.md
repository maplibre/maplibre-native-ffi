# Binding compiler architecture

A shared binding compiler resolves the C headers into one semantic model, and
every language binding is generated from that model. Correctness and performance
rank first, readability second, usability third, code size fourth, and
compatibility last. Existing API shapes change when that improves those
priorities.

## Decisions

- Public C declarations are the source for types and exported operations. Header
  metadata supplies relationships that C erases. Metadata describes behavior
  rather than language-specific source templates.
- One semantic resolver owns value shapes, presence, registration lifetimes,
  result ownership, and operation effects. Language emitters generate static
  code from the resolved plans and never infer ownership from a function name.
- Direct callbacks and borrowed completion values are the fast path. A host
  runtime that requires deferred delivery captures data before native returns,
  with capture code generated from the same value model.
- Callback roots, host scheduling, library loading, and handle close-once state
  are runtime mechanisms. Per-operation signatures, values, and conversions are
  generated, and each generated operation calls its C function through shared
  runtime helpers.
- Native code owns call leases and retirement. Bindings preserve host memory
  safety without duplicating native synchronization.
- An unclaimed owned result has a defined cleanup path that cannot fail. Cleanup
  starts without an optional completion observer.

## Limits

A new C ownership protocol needs a shared compiler rule and runtime support
before generation succeeds. The compiler rejects unknown shapes, because it
cannot infer ownership or callback quiescence from pointer types alone.

Bindings with tracing collectors hold callback roots on their owners, so a
callback that captures its owner remains collectable. Rust and Swift use
reference counting, so their callers use weak captures when a retained callback
refers to its owner.

Native disposal admission and scheduling are allocation-free. Teardown on other
threads may allocate, as
[native ownership](core-design.md#boundaries-of-the-guarantee) describes.
