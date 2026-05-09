---
title: Java FFM Binding Conventions
description: Language-specific implementation conventions for Java FFM bindings.
---

Resources:

- Tracking issue:
  [#45](https://github.com/maplibre/maplibre-native-ffi/issues/45)
- [Foreign Function & Memory API](https://docs.oracle.com/en/java/javase/25/core/foreign-function-and-memory-api.html)
- [`jextract`](https://jdk.java.net/jextract/)
- [Java JNI conventions](/maplibre-native-ffi/development/bindings-java-jni/)

The Java FFM binding targets modern desktop/server JVMs with the final
[Foreign Function & Memory API](https://docs.oracle.com/en/java/javase/25/core/foreign-function-and-memory-api.html).
Use the
[Java JNI conventions](/maplibre-native-ffi/development/bindings-java-jni/) for
Android and other JVMs where FFM is unavailable or undesirable.

Target JDK 22 or newer. Earlier JDKs used preview or incubator FFM APIs that
produce a different binding surface.

Generate the internal C layer with `jextract`. Treat successful generation as
the header bindability check for this path. The public Java layer is handwritten
and wraps the generated layer with stable names, ownership rules, diagnostics,
and lifetime control.

Keep FFM types internal. Public APIs do not expose `Arena`, `MemorySegment`,
`MethodHandle`, or generated C layout classes. Backend-native handles cross the
public API as `NativePointer` values and convert to FFM pointer values only at
the generated layer boundary.

Use FFM arenas according to lifetime:

- per-call confined arenas for temporary structs, UTF-8 strings, out parameters,
  and scratch buffers;
- object-owned arenas only when native storage belongs to a Java object;
- explicit native buffers for large reusable off-heap storage;
- scope-owned callback arenas for process-global logging callbacks,
  runtime-scoped resource callbacks, and map/style-scoped custom geometry
  callbacks.

FFM upcall stubs may be invoked on MapLibre worker, network, logging, or
render-related threads. Their arena and Java callback state live until native
code can no longer call them. Callback adapters use thread-safe state and do not
assume the runtime owner thread.
