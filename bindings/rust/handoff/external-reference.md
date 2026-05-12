# Research: Rust bindings external references and transferable implementation ideas

## Summary

The Rust binding should follow the repository’s documented three-crate shape:
generated `maplibre-native-sys`, shared `maplibre-native-support`, and safe
public `maplibre-native`. The strongest external evidence supports a narrow
unsafe raw layer, RAII/resource wrappers, explicit `Result` errors, `bindgen`
generation/layout checks, thread-safety assertions for raw-pointer types, panic
containment in callbacks, and compatibility-preserving bitflag/enum handling.

## Findings

1. **Issue #41 asks for generated raw Rust bindings plus a safe wrapper and
   smoke example.** The tracking issue body says “Raw binding tool: `bindgen`
   over `include/maplibre_native_abi.h`,” “Add an idiomatic safe Rust wrapper,”
   and “Add a smoke example.” Current docs say to generate from
   `include/maplibre_native_c.h`; the repo currently has
   `include/maplibre_native_c.h`, which exports the public C headers. Treat the
   issue’s `maplibre_native_abi.h` as stale or requiring maintainer confirmation
   before implementing generation.
   [Issue #41 API](https://api.github.com/repos/maplibre/maplibre-native-ffi/issues/41),
   repo path `include/maplibre_native_c.h`

2. **The unsafe/safe split is exactly the Rust FFI baseline.** The Rust Nomicon
   says foreign functions are unsafe because Rust cannot validate C
   declarations, pointer validity, or thread-safety, and then shows creating a
   safe interface that hides unsafe calls and uses Rust-owned buffers. This
   supports keeping raw bindgen declarations in `maplibre-native-sys` and
   exposing safe wrappers only from `maplibre-native`.
   [Rust Nomicon: FFI](https://doc.rust-lang.org/nomicon/ffi.html)

3. **Use RAII for native-owned handles, but keep explicit close for
   fallible/thread-affine destruction.** The Nomicon notes that foreign
   libraries often hand ownership of resources to callers and Rust destructors
   should release them, especially during panic. The project docs add a crucial
   constraint: destroy calls may return `MLN_STATUS_WRONG_THREAD`, so public
   `close` should return `Result<()>`, while `Drop` can only best-effort/log
   diagnostics. This mirrors Java FFM’s `HandleState.closeOnce` plus
   leak-reporting cleaner rather than cleaner-based destruction.
   [Rust Nomicon: Destructors](https://doc.rust-lang.org/nomicon/ffi.html), repo
   path
   `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/lifecycle/HandleState.java`

4. **Represent opaque C handles as typed opaque pointer wrappers in `sys`, then
   private non-null fields in safe handles.** The Nomicon recommends opaque
   `#[repr(C)]` structs with private fields/markers, and warns against empty
   enums as FFI types. It also notes marker fields can prevent unintended
   `Send`, `Sync`, and `Unpin`. That transfers directly to `mln_runtime`,
   `mln_map`, render session, projection, snapshot/result/list handles.
   [Rust Nomicon: Representing opaque structs](https://doc.rust-lang.org/nomicon/ffi.html)

5. **Thread-affine public handles need deliberate `!Send`/`!Sync` and regression
   tests.** Rust API Guidelines warn that raw-pointer types may auto-implement
   `Send`/`Sync` incorrectly and recommend compile-time tests asserting expected
   send/sync status. The bindings docs propose `PhantomData<Rc<()>>` for
   thread-affine handles and a special `Send` `ResourceRequestHandle`. Add tests
   such as `assert_not_impl_any!(RuntimeHandle: Send, Sync)` and
   `assert_impl_all!(ResourceRequestHandle: Send)`.
   [Rust API Guidelines C-SEND-SYNC](https://rust-lang.github.io/api-guidelines/interoperability.html#types-are-send-and-sync-where-possible-c-send-sync)

6. **`bindgen` should run from build tooling with a tight allowlist and layout
   tests.** The bindgen guide calls `build.rs` generation the recommended path
   because headers can vary by platform/architecture, but that requires builders
   to have libclang. Allowlisting restricts generation to needed
   types/functions/vars and transitive dependencies. For this repo, generation
   should allowlist `mln_.*` types/functions/constants and the public header,
   then compile generated tests as the Rust bindability check.
   [bindgen build.rs guide](https://rust-lang.github.io/rust-bindgen/library-usage.html),
   [bindgen allowlisting](https://rust-lang.github.io/rust-bindgen/allowlisting.html)

7. **If runtime dynamic loading is required, `libloading` fits but changes `sys`
   shape.** The Rust binding docs say the native library is loaded dynamically
   at runtime using `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH` then system search path.
   `libloading` provides cross-platform dynamic loading and ties `Symbol`
   lifetimes to the loaded `Library`, preventing common use-after-library-unload
   mistakes. Implementation choice: either generate normal extern declarations
   and rely on OS loader/linker, or generate/load a function table in `support`;
   docs currently imply dynamic runtime loading, closer to Java FFM’s
   `NativeLibrary`/`NativeAccess` pattern.
   [libloading docs](https://docs.rs/libloading/latest/libloading/), repo path
   `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/loader/NativeLibrary.java`

8. **Errors should be a meaningful public `Error` with raw status and copied
   diagnostic.** Rust API Guidelines require public error types to implement
   `std::error::Error`, `Display`, and preferably `Send + Sync`, and warn
   against `()` errors. `thiserror` can derive those impls without appearing in
   the public API, making it suitable for `ErrorKind`, raw `mln_status`, and
   diagnostic fields. Java FFM’s `Status.check` immediately calls
   `mln_thread_last_error_message()` on failure; Rust should do the same before
   any other C call on that thread.
   [Rust API Guidelines C-GOOD-ERR](https://rust-lang.github.io/api-guidelines/interoperability.html#error-types-are-meaningful-and-well-behaved-c-good-err),
   [thiserror docs](https://docs.rs/thiserror/latest/thiserror/), repo path
   `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/status/Status.java`

9. **Callbacks must catch panics and avoid unwinding into C.** The Nomicon says
   if panics or foreign exceptions cross a non-`unwind` ABI boundary, Rust
   panics abort and foreign exceptions entering Rust are UB; it recommends
   `catch_unwind` when Rust code may panic and should not abort. `ffi-support`
   reinforces the operational lesson: wrapping FFI bodies is required because
   unwinding through FFI is subtle and error-prone. For this repo: every C
   callback trampoline in `support` should `catch_unwind`, map failure to the
   documented C callback behavior, and never call thread-affine map/runtime APIs
   directly from worker callbacks.
   [Rust Nomicon: FFI and unwinding](https://doc.rust-lang.org/nomicon/ffi.html#ffi-and-unwinding),
   [ffi-support docs](https://docs.rs/ffi-support/latest/ffi_support/)

10. **Asynchronous callbacks need synchronization and deregistration/lifetime
    guarantees.** The Nomicon’s asynchronous callback section says C-created
    threads require synchronization, and if a callback targets a Rust object,
    the library must guarantee no callbacks after deregistration before the Rust
    object is destroyed. This maps to resource provider, resource transform,
    logging, and custom geometry callbacks: callback state should be
    owner-scoped, `Send + Sync + 'static` where native may invoke from arbitrary
    threads, and replacement should install new native state before dropping old
    state. Java FFM’s `LogCallbackState` demonstrates process-global callback
    ownership, shared arena/stub storage, catch-all exception handling, and
    replacement rollback on native installation failure.
    [Rust Nomicon: asynchronous callbacks](https://doc.rust-lang.org/nomicon/ffi.html#asynchronous-callbacks),
    repo path
    `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/callback/LogCallbackState.java`

11. **Use `CString`/`CStr` boundaries and reject embedded NULs for
    null-terminated inputs.** The Nomicon states Rust strings are not
    NUL-terminated and `CString` is needed for C strings. The shared binding
    docs require rejecting embedded NUL for null-terminated C inputs and using
    explicit byte lengths for string views. Implement support helpers for
    `CString::new` errors and `mln_string_view` byte-lifetime management.
    [Rust Nomicon: Interoperability with foreign code](https://doc.rust-lang.org/nomicon/ffi.html#interoperability-with-foreign-code)

12. **Public enums/flags need forward-compatibility.** Rust API Guidelines say
    new types should eagerly implement common traits; `bitflags` is designed for
    C-style flags and recommends an unnamed `_ = !0` flag for externally defined
    flags so future bits remain representable. For public C-backed enums, use
    explicit raw conversion and `#[non_exhaustive]`/`Unknown(raw)` where output
    can drift; for masks, use `bitflags!` with raw preservation.
    [Rust API Guidelines C-COMMON-TRAITS](https://rust-lang.github.io/api-guidelines/interoperability.html#types-eagerly-implement-common-traits-c-common-traits),
    [bitflags docs](https://docs.rs/bitflags/latest/bitflags/)

13. **Borrowed backend native pointers should be a tiny unsafe value type, not
    raw pointers in public APIs.** The shared docs and Rust docs both call for
    `NativePointer` over `void*`/backend handles. Implement as
    `#[repr(transparent)] struct NativePointer(NonNull<c_void>)` or possibly raw
    `usize`/`NonZeroUsize` depending on null requirements;
    construction/reconversion should be `unsafe` and API-specific docs should
    spell out that no ownership or memory access is granted. This mirrors Java
    FFM’s public `NativePointer` concept and internal
    `MemorySegment.ofAddress()` conversion policy.

14. **Java FFM implementation provides transferable patterns, not source shape
    to copy wholesale.** Useful existing references: `RuntimeHandle` keeps a map
    registry for event source lookup and uses confined arenas/out-pointers per
    call; `MapHandle` holds its parent runtime strongly; `NativeLibrary`
    implements exact-path env/property lookup; `Status` captures diagnostics
    immediately; `HandleState` centralizes live/released checks and parent
    references. Rust should translate these into ownership/lifetimes/RAII
    instead of synchronized Java state. Repo paths:
    `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/runtime/RuntimeHandle.java`,
    `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/map/MapHandle.java`,
    `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/loader/NativeLibrary.java`,
    `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/status/Status.java`,
    `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/lifecycle/HandleState.java`

## What matters for implementation

- **Crate layout:** `maplibre-native-sys` generated by bindgen;
  `maplibre-native-support` for status/diagnostics/materializers/callback
  trampolines/loading; `maplibre-native` safe public API.
- **Generation target:** use `include/maplibre_native_c.h` unless maintainers
  confirm the issue’s `include/maplibre_native_abi.h` name is intentional.
- **`sys` policy:** generated only, allowlist `mln_*`, compile/layout-test in
  CI, no public re-export from safe crate.
- **Library loading:** decide early between link-time externs and runtime
  `libloading` function table; docs promise runtime lookup via
  `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`, so implementation likely needs
  support-owned loader state.
- **Handles:** private non-null native pointer, live/released bit, parent
  retention except `MapProjectionHandle`, `PhantomData<Rc<()>>` for thread
  affinity, explicit `close() -> Result<()>`, best-effort/logging `Drop`.
- **Errors:** `Result<T, Error>` everywhere fallible; `ErrorKind`, raw status,
  copied diagnostic; unknown future status preserved.
- **Descriptors:** Rust-owned structs with `Default`/builder setters; support
  materializers write `size` and masks at call boundary.
- **Callbacks:** owner-scoped state, `Send + Sync + 'static` for arbitrary
  native threads, `catch_unwind`, no unwinding through C, one-shot resource
  request completion/release.
- **Rendering:** safe readback into `&mut [u8]`; frame handles borrow the
  session mutably to prevent nested/reentrant calls; unsafe `NativePointer`
  accessors tied to frame lifetime.
- **Tests:** smoke example from issue; real C-call adaptation tests for
  status/diagnostics; compile-fail or trait assertion tests for `Send`/`Sync`;
  callback panic containment; released-handle errors; parent-lifetime and
  frame-lifetime invariants.

## Risks

- **Header mismatch risk:** Issue #41 says `maplibre_native_abi.h`, docs/repo
  say `maplibre_native_c.h`.
- **Dynamic loading risk:** Plain bindgen externs do not by themselves implement
  the documented exact-path runtime lookup. A `libloading` table is more work
  and can complicate symbol typing/lifetimes.
- **Drop semantics risk:** Native destroy can fail on wrong thread; `Drop`
  cannot return `Result`. Rely on explicit `close` for correctness and keep
  `Drop` conservative.
- **Auto-trait risk:** Raw pointers/`NonNull` wrappers can accidentally become
  `Send`/`Sync` through field changes. Add static trait tests.
- **Callback lifetime risk:** Dropping closure state while native callbacks are
  in flight is the highest-risk area. Use owner-scoped state, active-call
  counters where needed, and install-before-drop replacement ordering.
- **Unwinding risk:** Any panic crossing a C callback boundary can abort or
  worse depending ABI; trampolines must catch.
- **Forward-compat risk:** Enums/flags from a C ABI can grow; preserve raw
  unknown values and unknown bits.

## Sources

- Kept: Add Rust bindings and smoke example, issue #41
  (https://github.com/maplibre/maplibre-native-ffi/issues/41 /
  https://api.github.com/repos/maplibre/maplibre-native-ffi/issues/41) — primary
  task scope and acceptance shape.
- Kept: Rust Nomicon: FFI (https://doc.rust-lang.org/nomicon/ffi.html) — primary
  Rust FFI safety, opaque structs, strings, destructors, callbacks, unwinding.
- Kept: bindgen User Guide: build.rs
  (https://rust-lang.github.io/rust-bindgen/library-usage.html) — recommended
  target-specific generation model.
- Kept: bindgen User Guide: allowlisting
  (https://rust-lang.github.io/rust-bindgen/allowlisting.html) — keeping
  generated raw surface precise.
- Kept: Rust API Guidelines: Interoperability
  (https://rust-lang.github.io/api-guidelines/interoperability.html) — common
  traits, conversions, `Send`/`Sync`, error type quality.
- Kept: libloading docs (https://docs.rs/libloading/latest/libloading/) —
  plausible implementation for documented runtime native-library lookup.
- Kept: bitflags docs (https://docs.rs/bitflags/latest/bitflags/) — C-style
  flags and unknown-bit compatibility.
- Kept: thiserror docs (https://docs.rs/thiserror/latest/thiserror/) — ergonomic
  stable `std::error::Error` implementation without public API coupling.
- Kept: ffi-support docs (https://docs.rs/ffi-support/latest/ffi_support/) —
  transferable panic/error/string patterns from production Rust FFI support; use
  as design inspiration, not necessarily a dependency.
- Kept: Java FFM local source paths listed above — existing binding evidence for
  loader, status, handle, callback, parent-retention patterns.
- Dropped: `safer-ffi` (https://docs.rs/safer-ffi/latest/safer_ffi/) — useful
  for exporting Rust APIs to C, but this project consumes an existing C ABI via
  bindgen.
- Dropped: UniFFI (https://github.com/mozilla/uniffi-rs) — excellent for
  generating Kotlin/Swift/Python/Ruby bindings from Rust components, but
  conflicts with this repo’s public C ABI + language-specific low-level binding
  model.
- Dropped: cbindgen (https://github.com/eqrion/cbindgen) — generates C headers
  from Rust; reverse direction from this task.
- Dropped: maplibre-native-rs search results — relevant ecosystem context, but
  not primary evidence for this repository’s C ABI binding implementation.

## Gaps

- Need maintainer decision on `include/maplibre_native_abi.h` vs
  `include/maplibre_native_c.h` before scripting bindgen.
- Need repository build-system details for Rust crate placement and mise task
  naming.
- Need exact native callback contracts from the C headers before finalizing
  trampoline return behavior and active-upcall shutdown rules.
- Need packaging decision for runtime dynamic loading: `libloading` table vs
  link-time externs plus platform loader configuration.
