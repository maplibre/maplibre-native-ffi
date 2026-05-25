# Java JNI java-bindgen suitability spike

This record captures the suitability spike that preceded the broad Java JNI
boundary rewrite. The spike was intentionally temporary: it used internal test
bridge classes, proved the generator behavior, and was removed before the final
implementation replaced the registration table.

## Scope

The spike tested `java-bindgen` only as an internal JNI boundary. It kept the
crafted public Java API intact and targeted existing bridge-style methods rather
than publishing generated Java types.

The spike covered four representative paths:

1. Runtime lifecycle: create a runtime from Java, return the native handle
   through a `long[]`, and destroy it through the existing wrapper flow.
2. Diagnostics: return a non-OK native status and read the thread-local native
   diagnostic through the Java `Status` path.
3. Opaque handle/out-array transfer: move an opaque native address through a JNI
   array without giving Java memory access.
4. Callback lifecycle: install and clear the log callback while Rust owns and
   releases the Java global reference.

## Results

The spike succeeded and showed that `java-bindgen` could generate the exported
JNI entry point while the existing Rust adapters preserved ownership,
diagnostics, callbacks, and panic boundaries.

It also found one integration requirement: the upstream macro parsed the
workspace root manifest, which is virtual in this repository. The JNI crate now
uses a fork branch where the macro reads `CARGO_MANIFEST_DIR/Cargo.toml`.

An earlier prototype also tried custom `class` and `method` macro attributes for
exact JNI symbol targeting. The final implementation does not need those
attributes because generated load-time registration maps arbitrary generated
wrapper symbols to the existing internal Java bridge declarations.

The failed path was `java-pack`; it was not needed for this JNI boundary rewrite
and failed with `Command java-pack: command new-test alias t is duplicated`.

## Permanent coverage after the spike

The temporary spike files were removed after the rewrite. The final branch keeps
coverage through permanent tests and generated build artifacts:

- `NativeRegistrationTest` verifies generated exports are callable and the
  intentionally omitted test native remains unresolved.
- `RuntimeHandleTest` covers runtime create/close and signed validation.
- `PanicBoundaryTest` covers panic containment and diagnostic propagation.
- `GlobalReferenceTest` and `LogCallbackStateTest` cover callback/global
  reference lifecycle.
- `NativeBufferTest` covers opaque native buffer ownership and validation.
- `bindings/java-jni/native/build.rs` now generates the java-bindgen wrappers
  and load-time registration metadata from the internal Java declarations.
