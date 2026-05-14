# Rust bridge binding plan

## Audience and purpose

This plan is for maintainers and contributors who build native-extension bridge
bindings on top of the Rust crates. The Rust/core refactor has established the
shared adaptation layer. The remaining work is to prove that Python, Node.js,
and Java JNI can share `maplibre-native-core` without adopting the public Rust
crate's handle, lifetime, callback, or finalizer model.

## References

- [Binding conventions](../../docs/src/content/docs/development/bindings.md)
- [Rust binding conventions](../../docs/src/content/docs/development/bindings-rust.md)
- [C API conventions](../../docs/src/content/docs/development/c-conventions.md)
- [Concepts](../../docs/src/content/docs/concepts.md)

## Design conclusion

Bridge bindings should build on `maplibre-native-core`, not on the public
`maplibre-native` crate. They may touch `maplibre-native-sys` directly only in
host-runtime trampoline code or where a C ABI feature has no core abstraction
yet.

`maplibre-native-core` owns bridge-neutral C ABI adaptation:

- status-code mapping and diagnostic capture;
- copied values and enum domains;
- descriptor materialization, including `size` fields, field masks, string
  views, arrays, and backing storage;
- native result copying and exactly-once result-handle release;
- event, log, resource request, and resource response copying;
- callback trampoline primitives that copy C arguments, catch panics, and return
  documented C callback outcomes;
- explicit close-once native handle state that bridge crates can wrap with their
  own finalizer and synchronization policy.

Each bridge crate owns its host runtime policy above core.

## Target bridge crates

```text
maplibre-native-python
  PyO3 extension crate.
  Python classes, exceptions, buffers, queues, GIL/free-threaded synchronization,
  context managers, and leak-reporting finalizers.

maplibre-native-node
  napi-rs add-on crate.
  N-API classes, TypeScript declarations, environment cleanup hooks,
  thread-safe functions, Symbol.dispose, and leak-reporting finalizers.

maplibre-native-jni
  jni-rs bridge crate.
  JNI method registration, Java exceptions, global references, JVM thread
  attach/detach, Android packaging hooks, and leak-reporting finalizers.
```

## Future milestone: prove the bridge boundary

Create thin vertical slices for Python, Node.js, and JNI before filling broad
coverage.

Deliverables:

- Add minimal PyO3, napi-rs, and jni-rs crates.
- Implement the same small feature slice in each bridge:
  - ABI version;
  - network status;
  - runtime create/close;
  - map create/close;
  - one copied event or copied value conversion.
- Keep bridge-specific exceptions, finalizers, loading hooks, callback queues,
  and runtime attachment local to each bridge crate.
- Record any direct `sys` calls needed by the slices. Move repeated direct `sys`
  sequences into `core`, or document why they are host-runtime trampolines.

Acceptance criteria:

- Python, Node.js, and JNI can use the shared layer without wrapping the public
  Rust crate.
- Each bridge crate owns its host runtime's public safety model.
- Direct `sys` usage outside `core` stays limited to host-runtime trampoline
  boundaries or documented missing core adapters.
- Early bridge code validates the architecture before broad binding generation
  begins.

## Bridge-specific ownership

Python owns:

- PyO3 classes and exceptions;
- CPython 3.14 and free-threaded synchronization;
- Python buffers and `bytes` conversions;
- context-manager helpers;
- Python callback queues;
- leak reporting through `__del__`, weak finalizers, or GC hooks.

Node.js owns:

- napi-rs classes and TypeScript declarations;
- N-API environment ownership;
- `ThreadsafeFunction` callback routing;
- environment cleanup hooks;
- `Symbol.dispose` and finalizer leak reporting.

Java JNI owns:

- `JNI_OnLoad` and native method registration;
- Java exceptions and status translation;
- `JavaVM`, `JNIEnv`, global references, and local-reference scopes;
- native thread attach/detach;
- Android/JVM loading policy.

## Review rules for bridge work

Use these rules when reviewing future bridge binding changes:

- Code that initializes C `size` fields, field masks, nested native arrays,
  string backing storage, or result-handle copying belongs in `core`.
- Code that chooses public method names, exceptions, finalizers, callback
  queues, wrapper retention, or scheduler behavior belongs in the bridge crate.
- If two bridge crates need the same direct `sys` sequence, move that sequence
  into `core`.
- If an abstraction requires Python, Node.js, Java, or public Rust concepts to
  explain its invariants, keep it above `core`.
- If a finalizer might run on an arbitrary thread, make it report a leak unless
  the native release function is documented as thread-independent and
  infallible.
- Keep examples small and binding-focused; full SDK behavior belongs above these
  low-level bindings.
