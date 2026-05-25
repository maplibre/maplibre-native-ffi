# Java JNI java-bindgen integration

This note is for contributors who maintain the Java JNI Rust bridge. It records
why the bridge uses `java-bindgen`, what the suitability spike covered, and why
some JNI adaptation still lives in hand-written Rust.

## Suitability spike

The rewrite started with a temporary in-repo spike before the broad registration
table removal. The spike targeted existing internal bridge classes instead of
introducing public generated Java APIs. It proved these paths:

- runtime lifecycle: create a native runtime handle and destroy it through the
  existing Java wrapper flow;
- diagnostics: return a native status, then let Java read the thread-local
  diagnostic through the existing `Status` path;
- opaque handles and out arrays: fill a Java `long[]` from Rust and pass the
  handle back through Java-owned lifecycle code;
- callback lifecycle: install and clear the log callback while Rust owns the
  global reference and releases it exactly once.

The spike also exposed one integration requirement that this branch preserves:
`java-bindgen-macro` needs to parse the JNI crate manifest rather than the
workspace virtual manifest. The dependency points at a small fork branch with
that manifest lookup fix so this repository does not vendor the macro source.

The temporary spike files were removed after the rewrite. The permanent test
suite keeps the same safety coverage through `RuntimeHandleTest`,
`PanicBoundaryTest`, `NativeRegistrationTest`, `GlobalReferenceTest`,
`LogCallbackStateTest`, `NativeBufferTest`, and the Java JNI build/check tasks.

## Final boundary shape

`build.rs` scans the internal bridge declarations in
`src/main/java/org/maplibre/nativejni/internal/bridge` and emits a Rust source
file in `OUT_DIR`. That generated file declares small wrapper functions
annotated with `#[java_bindgen(...)]`. The macro emits named JNI exports for the
generated wrapper functions, and `build.rs` registers those function pointers to
the existing bridge classes and methods.

The Rust bridge keeps the hand-written adapter functions behind those exports.
Those functions own the safety-sensitive parts of the boundary: status mapping,
thread-local diagnostics, panic containment, string conversion, ownership
checks, callback global references, thread attachment, and signed-to-unsigned
validation. `java-bindgen` replaces duplicated export glue, while `build.rs`
generates the registration metadata from the same internal Java declarations; it
does not replace the safety adapters.

## Loading and artifact mismatch

The old bridge used a hand-written `RegisterNatives` table in `JNI_OnLoad`. The
new bridge keeps load-time registration, but `build.rs` generates the method
metadata from the internal Java declarations and points each registered method
at the matching `java-bindgen` export. This preserves load-time artifact
mismatch behavior without keeping a hand-maintained signature table.

`NativeRegistrationTest` preserves explicit coverage for that behavior:
generated exports are callable, while
`JniTestNative.unregisteredNativeForTesting` still throws `UnsatisfiedLinkError`
because it is intentionally omitted from the generated registration set.

## Remaining hand-written JNI

Some JNI remains hand-written because it enforces binding semantics that the
generator cannot infer from Java signatures:

- callbacks keep global references and owner-scoped state;
- resource and custom geometry paths attach native threads before Java upcalls;
- destroy paths preserve retry semantics when native destruction fails;
- arrays and buffers validate lengths and signed-to-unsigned conversions before
  calling the C ABI;
- panic and diagnostic boundaries must stay close to each native call.

These adapters are narrow by design. New bridge methods should prefer generated
exports plus small safety adapters over new registration tables or duplicated
JNI signatures.
