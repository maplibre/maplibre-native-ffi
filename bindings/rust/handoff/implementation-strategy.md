# Rust bindings implementation strategy

## Executive recommendation

Build the Rust binding in small, shippable layers rather than trying to mirror
the full Java FFM surface in one pass. The safest first slice is:

1. Add a Cargo workspace and `bindings/rust` package roots.
2. Implement `maplibre-native-sys` generated from `include/maplibre_native_c.h`
   with bindgen layout tests and link/load plumbing against the existing CMake
   artifact.
3. Implement `maplibre-native-support` for status/diagnostic conversion, raw
   out-pointer helpers, C string/string-view helpers, and RAII guards for
   snapshot/list handles.
4. Implement a narrow public `maplibre-native` crate covering process/global
   calls, `RuntimeHandle::{create, run_once, poll_event, close}`,
   `MapHandle::{create, set_style_json/url, close}`, and basic copied value
   descriptors.
5. Validate through real C ABI calls and Rust-owned lifetime tests, then expand
   by concept modules.

This matches the Rust conventions (`sys`/`support`/public split), the Java
implementation patterns (generated raw layer + internal
status/lifecycle/memory + public concept packages), and the current native build
model (CMake builds `maplibre-native-c`; language tests point at that artifact).

## Evidence and local constraints

### Binding architecture and Rust-specific requirements

- `docs/src/content/docs/development/bindings-rust.md:23-31` defines the crate
  split:
  - `maplibre-native-sys`: generated unsafe declarations.
  - `maplibre-native-support`: shared glue: status conversion, diagnostics,
    descriptor materializers, callbacks, build/link utilities.
  - `maplibre-native`: public safe Rust crate.
- `docs/src/content/docs/development/bindings-rust.md:35-39` requires `bindgen`
  generation from `include/maplibre_native_c.h`; generated bindings should not
  be hand edited.
- `docs/src/content/docs/development/bindings-rust.md:47-49` says native loading
  is dynamic at runtime: exact path from `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`,
  then system search path.
- `docs/src/content/docs/development/bindings-rust.md:75-79` requires
  thread-affine handles to be `!Send + !Sync` via `PhantomData<Rc<()>>`;
  `ResourceRequestHandle` is the known `Send` exception; `MapProjectionHandle`
  remains `!Send`.
- `docs/src/content/docs/development/bindings-rust.md:101-106` requires public
  fallible APIs to use `pub type Result<T> = std::result::Result<T, Error>` and
  never panic on native status.

### Shared binding conventions that Rust must preserve

- `docs/src/content/docs/development/bindings.md:68-72` defines the binding
  layers: internal C layer, internal support layer, public binding layer.
- `docs/src/content/docs/development/bindings.md:88-92` requires every
  long-lived opaque handle to have deterministic release; successful release
  makes later releases no-ops; wrong-thread destroy must report the language
  error.
- `docs/src/content/docs/development/bindings.md:141-145` requires reading the
  thread-local diagnostic immediately after a non-OK C status, before another C
  API call can overwrite it.
- `docs/src/content/docs/development/bindings.md:309-315` says binding tests
  should focus on adaptation: wrappers, ownership, copying, callbacks, threading
  errors, error mapping. C ABI tests already prove native behavior.

### Java FFM implementation patterns to copy conceptually

- Generated raw C layer: `bindings/java-ffm/build.gradle.kts:11-20` uses
  jextract against `include/maplibre_native_c.h`, includes `include/`, and
  whitelists generated symbols. Rust should use bindgen against the same
  umbrella header, with allowlists to `mln_*` and `MLN_*` as needed.
- Test native library path: `bindings/java-ffm/build.gradle.kts:31-44` derives a
  test exact library path from `MLN_FFI_BUILD_DIR` and passes it into Java. Rust
  tests should similarly set or consume `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`, or
  link with rpath/library path to `MLN_FFI_BUILD_DIR`.
- Native access/version check:
  `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/loader/NativeAccess.java:8-31`
  ensures the library is loaded before generated calls and checks
  `mln_c_version() == 0`. Rust support/public crate should check ABI version
  once during load/init.
- Status conversion:
  `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/status/Status.java:14-28`
  maps OK to return and otherwise captures `mln_thread_last_error_message()`
  immediately.
- Handle state:
  `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/lifecycle/HandleState.java:10-18`
  stores handle, parents, leak report; `:53-65` implements close-once and marks
  released only after native destroy succeeds.
- Runtime creation and out pointers:
  `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/runtime/RuntimeHandle.java:55-64`
  allocates an out pointer, calls `mln_runtime_create`, checks status, then
  wraps the non-null result.
- Map parent retention and registry pattern:
  `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/map/MapHandle.java:69-81`
  creates a map from a runtime, wraps it, and registers it with the parent. Rust
  should keep parent relationships strong where native validity depends on them.
- Map close unregisters after successful destroy:
  `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/map/MapHandle.java:1522-1529`
  uses close-once and unregisters only after successful native destroy.

### Local C API facts that shape the Rust wrapper

- Public umbrella header: `include/maplibre_native_c.h:17-28` includes all
  domain headers. This is the bindgen input.
- Status enum: `include/maplibre_native_c/base.h:36-48` defines `mln_status`
  with `OK`, `INVALID_ARGUMENT`, `INVALID_STATE`, `WRONG_THREAD`, `UNSUPPORTED`,
  `NATIVE_ERROR` as `int32_t` values.
- Opaque handles: `include/maplibre_native_c/base.h:51-58` typedefs runtime,
  map, projection, offline snapshots/lists, JSON snapshots, resource request
  handles, and render sessions.
- Diagnostics: `include/maplibre_native_c/diagnostics.h:14-22` exposes
  `mln_thread_last_error_message()` and documents that the pointer remains valid
  until another C API call on the same thread writes diagnostics.
- Runtime destroy: `include/maplibre_native_c/runtime.h:581-602` says
  `mln_runtime_destroy` can return invalid state if maps are live, wrong thread
  if not on owner thread, and native error.
- Resource request release: `include/maplibre_native_c/runtime.h:541-548` is
  void, accepts null as no-op, and released handles must not be reused; this
  differs from fallible thread-affine destroy.
- Map destroy: `include/maplibre_native_c/map.h:915-931` says maps must not have
  attached render sessions, and wrong-thread/invalid-state errors are possible.
- Projection destroy: `include/maplibre_native_c/projection.h:33-52` creates a
  standalone projection snapshot with owner-thread affinity but no parent native
  validity dependency after creation.

### Existing build/test system

- `mise.toml:20` treats `bindings/*` as monorepo config roots, so
  `bindings/rust/mise.toml` is a natural home for Rust-specific tasks.
- `mise.toml:124-126` top-level `test` builds native via CMake then runs Zig C
  API tests against `MLN_FFI_BUILD_DIR`.
- `build.zig:28-32` shows existing tests link to `maplibre-native-c` by adding
  include path, library path, rpath, and system library name.
- `CMakeLists.txt` builds the shared C API library target `maplibre_native_c`
  and alias `maplibre_native_ffi::c`; Rust should consume the produced dynamic
  library rather than trying to build native itself in the first pass.
- There is currently no root Cargo workspace in this repo, no Rust tool pinned
  in `mise.toml`, no Rust formatting/lint route in `dprint.jsonc`/`hk.pkl`, and
  no Rust CI/task integration yet.

## Likely files/directories to change in implementation

### New Rust workspace/crates

- `Cargo.toml` at repository root, or `bindings/rust/Cargo.toml` plus root
  workspace membership decision.
- `Cargo.lock` (if this repo commits Rust locks for reproducible binding builds;
  decision needed).
- `bindings/rust/mise.toml` with `build`, `test`, possibly `bindgen`/`generate`
  tasks.
- `bindings/rust/crates/maplibre-native-sys/Cargo.toml`
- `bindings/rust/crates/maplibre-native-sys/build.rs`
- `bindings/rust/crates/maplibre-native-sys/wrapper.h` (likely includes
  `../../../include/maplibre_native_c.h` or uses include path)
- `bindings/rust/crates/maplibre-native-sys/src/lib.rs` (include generated
  bindings; maybe generated into `OUT_DIR` initially)
- `bindings/rust/crates/maplibre-native-support/Cargo.toml`
- `bindings/rust/crates/maplibre-native-support/src/{lib.rs,status.rs,diagnostics.rs,loader.rs,memory.rs,handle.rs}`
- `bindings/rust/crates/maplibre-native/Cargo.toml`
- `bindings/rust/crates/maplibre-native/src/{lib.rs,error.rs,runtime.rs,map.rs,render.rs,geo.rs,camera.rs,...}`
- `bindings/rust/crates/maplibre-native/tests/*.rs`

### Build/tooling integration

- `mise.toml`: add Rust tool pin (`core:rust` or equivalent available in mise),
  and possibly top-level `test` depends on `//bindings/rust:test` once stable.
- `dprint.jsonc`: add rustfmt exec if dprint does not already format Rust
  through an installed plugin.
- `hk.pkl`: add `cargo fmt --check`/`cargo clippy`/`cargo test` only after Rust
  toolchain is pinned and task cost is acceptable.
- `.github/workflows/**` and/or matrix scripts: include Rust binding tests after
  initial local task works.
- Docs reference generation later, not in first slice unless public API docs are
  required.

## Staged implementation strategy

### Stage 0 — Decisions and workspace/tooling skeleton

Goal: Make Rust a first-class binding root without wrapping API yet.

- Add Cargo workspace structure.
- Pin Rust and Clang/libclang prerequisites. `bindgen` needs libclang; this may
  come from Pixi/LLVM, Xcode/clang, or a Rust crate feature/config. Decide
  before CI rollout.
- Add `bindings/rust/mise.toml` with tasks that follow Java’s pattern:
  - `build` depends on `//:ensure-native-library` and runs `cargo build`.
  - `test` depends on `//:ensure-native-library` and runs `cargo test` with
    `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH` pointed to
    `$MLN_FFI_BUILD_DIR/${system library name}` or with library search/rpath
    configured by build scripts.

Recommended first choice: keep the Rust workspace under `bindings/rust` and use
a root `Cargo.toml` workspace only if Cargo invocation from repo root is
desired. Avoid pulling Rust crates into top-level `mise run test` until
sys/support/public smoke tests are reliable.

### Stage 1 — `maplibre-native-sys` generation and linking

Goal: Prove the C API is bindgen-compatible and Rust can link/load the native
artifact.

Implementation shape:

- `build.rs` runs `bindgen` against `include/maplibre_native_c.h`.
- Allowlist public symbols: `mln_.*`, `MLN_.*`; blocklist implementation-only
  symbols if bindgen discovers any.
- Pass include path to repo `include/`.
- Generate layout tests. This is explicitly required by Rust conventions as a
  bindability check.
- Link dynamically to `maplibre-native-c`:
  - Add `cargo:rustc-link-search=native=$MLN_FFI_BUILD_DIR` when set.
  - Add `cargo:rustc-link-lib=dylib=maplibre-native-c`.
  - Add platform rpath only if accepted by project policy; otherwise rely on
    `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`/loader or test env.
- Provide a tiny test that calls `mln_c_version()` and
  `mln_supported_render_backend_mask()`.

Important risk: bindgen and C23 fixed-underlying enum syntax
(`typedef enum mln_status : int32_t`) may require a sufficiently new
clang/libclang and `-std=c23`/`-xc`. This must be validated before wrapping
public APIs.

### Stage 2 — `maplibre-native-support` status, diagnostics, and loading

Goal: Centralize all raw-to-safe glue that bridge bindings will reuse.

Implement:

- `Status`/`RawStatus` conversion from `sys::mln_status` integer values.
- `ErrorKind::{InvalidArgument, InvalidState, WrongThread, Unsupported, NativeError, Unknown(i32)}`.
- `Error { kind, raw_status, diagnostic }` and `Result<T>` helper (public crate
  can re-export its own stable error type if desired).
- `check(status) -> Result<()>` that reads `mln_thread_last_error_message()`
  immediately on non-OK.
- ABI version check (`mln_c_version() == 0`) modeled after Java `NativeAccess`.
- Loader/link helper decision:
  - If using ordinary dynamic linking, support mostly configures cargo link
    paths.
  - If strict runtime dynamic loading is required, use `libloading` and
    generated function-pointer bindings instead of direct extern linkage. This
    is a major design choice needing approval because bindgen’s default
    `extern "C"` model links at load time, not lazy runtime loading.
- Helpers for C strings (reject embedded NUL for null-terminated inputs), string
  views, initialized null out-pointers, and RAII guards for owned snapshot/list
  handles.

Recommendation: start with ordinary dynamic linking for stage 1/2 validation,
but explicitly decide whether to satisfy the doc’s “loaded dynamically at
runtime” requirement with direct dynamic linkage + environment/library search,
or with `libloading` exact-path loading. Java uses explicit `System.load` before
generated calls; Rust direct externs cannot exactly mimic that unless the
process can locate the dylib before symbol use.

### Stage 3 — Public crate MVP: process globals and runtime lifecycle

Goal: Establish safe public API patterns and tests with minimal surface.

Implement:

- `Maplibre`-style module functions in Rust naming: `c_version()`,
  `supported_render_backends()`, `network_status()`, `set_network_status()`.
- `RuntimeOptions` with `Default`; materializer writes `size`, flags, copied C
  strings.
- `RuntimeHandle`:
  - stores `NonNull<sys::mln_runtime>` privately;
  - `PhantomData<Rc<()>>` makes it `!Send + !Sync`;
  - live/released state prevents double close and use-after-close;
  - `close(self)` or `close(&mut self)` returns `Result<()>` for deterministic
    release;
  - `Drop` attempts best-effort destroy only if Rust can prove owner-thread drop
    (the doc says safe Rust can prove due to `!Send`; still record/drop
    diagnostics on failure).
- Methods: `create`, `run_once`, `poll_event` returning owned
  `Option<RuntimeEvent>` with unknown payload fallback if needed.

Tests:

- `creates_runs_polls_and_closes_runtime` equivalent to Java.
- released runtime rejects later methods before native dispatch.
- wrong-thread behavior: because `RuntimeHandle` is `!Send`, normal safe Rust
  cannot move it to another thread; test wrong-thread only through an internal
  unsafe/raw test helper if worthwhile. The public guarantee is stronger than
  Java here.

### Stage 4 — Map lifecycle and simplest map commands

Goal: Prove parent retention and map owner-thread lifecycle.

Implement:

- `MapOptions { width, height, scale_factor, map_mode }` with `Default` and
  builder setters.
- `MapHandle` created from `&RuntimeHandle` or an owning parent reference
  strategy.
- Parent retention options:
  - Use `Rc<RuntimeInner>` shared between runtime and child maps, so maps keep
    runtime native validity while live.
  - Runtime `close` should return `InvalidState` or binding-owned error if maps
    are live; do not allow native invalid state to surprise callers when Rust
    can model it.
- Methods: `set_style_url`, `set_style_json`, `request_repaint`,
  `is_fully_loaded`, `close`.
- Runtime map registry for event source mapping can initially use Rust-assigned
  `MapId` copied into events, as Rust conventions recommend; do not expose raw
  map pointers.

Tests:

- map create/close with runtime.
- closing runtime with live map yields invalid-state/error and leaves runtime
  live.
- map close unregisters and later close no-ops.
- basic style JSON command and event polling, reusing Java test’s small inline
  style JSON.

### Stage 5 — Descriptor/value expansion before callbacks/rendering

Goal: Add low-risk data/value wrappers before complex lifetimes.

Good next slices:

- Core value structs: `LatLng`, `ScreenPoint`, `EdgeInsets`, `LatLngBounds`.
- Camera descriptors and field masks via `Option<T>` fields or explicit setters,
  with field masks internal to materializers.
- Closed enums with `#[non_exhaustive]`, explicit raw conversions,
  `Unknown(raw)` for output domains likely to drift.
- JSON/GeoJSON value trees only after deciding object order/duplicate key
  representation.
- Snapshot/list readers with internal RAII guards that destroy native handles
  even on copy failure.

Avoid starting with callbacks/render sessions/resource providers; their thread
and lifetime rules are the sharpest part of the C API.

### Stage 6 — Callbacks/resource provider/resource transform

Only after support crate has panic-catching trampolines and state ownership
patterns.

Critical invariants:

- Catch panics with `catch_unwind`; never unwind through C.
- Callback state must be `Send + Sync + 'static` for callbacks that may arrive
  on worker/network/render/logging threads.
- When replacing callbacks, install new native descriptor before closing old
  state; if install fails, close replacement and keep old active (matches Java
  and Rust docs).
- `ResourceRequestHandle` is `Send`, enforces one-shot completion, and releases
  exactly once on complete/explicit release/drop.

### Stage 7 — Render sessions and backend native pointers

Implement after map/runtime basics and descriptor materializers are stable.

Critical invariants:

- `RenderSessionHandle` holds map strongly.
- Single-session/attached-frame invalid states map to `ErrorKind::InvalidState`.
- `NativePointer` is an opaque borrowed address value; safe APIs expose no raw
  memory access.
- Owned texture frame handles prevent nested frame acquisition and session
  mutation while live. Use Rust borrowing to make reentrant session calls
  impossible where practical.

## Edge cases to validate early

- Bindgen handles C23 fixed-underlying enum syntax and bool layout across
  supported OS/toolchains.
- `mln_status` raw type is signed; non-status enum/mask domains are unsigned.
  Preserve raw values exactly.
- Out-pointers must be initialized to null before C handle-creating calls; wrap
  only successful non-null outputs.
- `mln_thread_last_error_message()` must be copied before any other C API call
  after a non-OK status.
- Released wrappers should fail before native dispatch with a binding-owned
  invalid-state error.
- Native destroy failures (wrong thread/invalid state) must leave wrappers live
  so callers can retry on the owner thread or after closing children.
- `Drop` must avoid double release. If it cannot return an error, it should
  record/log diagnostics without panicking.
- Null-terminated string inputs reject embedded NUL; `mln_string_view` uses
  UTF-8 bytes plus byte length.
- `MapProjectionHandle` does not retain its parent map for native validity but
  remains `!Send`.
- Snapshot/list handles are destroyed on all paths, including
  copy/materialization errors.
- Direct dynamic linking vs explicit runtime loading affects whether
  `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH` can be an exact library path. This is a
  design decision, not just implementation detail.

## Validation commands

Initial local validation after Rust tasks exist:

```bash
mise run //:build
mise run //bindings/rust:build
mise run //bindings/rust:test
mise run test
mise run fix
```

Direct Cargo equivalents (task internals may differ):

```bash
MLN_FFI_BUILD_DIR=${MLN_FFI_BUILD_DIR:-build/host} cargo test --workspace
cargo fmt --all --check
cargo clippy --workspace --all-targets -- -D warnings
```

Sys-generation-specific checks:

```bash
cargo test -p maplibre-native-sys
cargo test -p maplibre-native-support
cargo test -p maplibre-native
```

If direct dynamic linking is used, run one test with the library path absent to
confirm the failure message is actionable, and one with
`MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`/`MLN_FFI_BUILD_DIR` set to the exact native
artifact.

## Decisions needing approval

1. **Runtime loading model:** Should Rust use direct `extern "C"` dynamic
   linkage generated by bindgen, or explicit `libloading` function-pointer
   loading to honor exact-path runtime loading like Java? Direct linkage is
   simpler; `libloading` better matches docs but complicates generated sys
   shape.
2. **Workspace location:** Root Cargo workspace vs self-contained
   `bindings/rust` workspace. Root workspace improves discoverability;
   self-contained keeps language package state localized.
3. **Cargo.lock policy:** Commit lockfile for reproducible binding/CI builds, or
   omit for library-crate norms. Given this repository pins tools heavily,
   committing a lockfile is likely safer.
4. **Rust toolchain source:** Add Rust to top-level `mise.toml` and CI, and
   decide where libclang comes from for bindgen.
5. **Top-level test integration timing:** Include Rust tests in `mise run test`
   immediately, or wait until the sys/support/public MVP is stable.
6. **Drop behavior:** Whether `Drop` should attempt native destroy for
   thread-affine handles (Rust docs imply yes because `!Send` proves
   owner-thread drop) or only leak-report like Java cleaners. Rust can be
   stricter than Java, but callbacks/parent cycles may complicate teardown.
7. **Public API ownership shape:** `close(self)`, `close(&mut self)`, or an
   interior-state `close(&self)` pattern. `close(self)` is most Rust-idiomatic
   for deterministic consume, but Java-like idempotent close requires retained
   released state.

## Suggested first implementation slice

For the first PR/slice, keep scope deliberately narrow:

- Add Rust tool/task scaffolding.
- Add `maplibre-native-sys` with bindgen generation, layout tests, and a smoke
  test calling `mln_c_version()`.
- Add minimal `maplibre-native-support::status` with diagnostic capture and
  tests using `mln_network_status_set(999_999)` to force `INVALID_ARGUMENT`,
  mirroring `StatusAndMemoryTest`.
- Add no public `maplibre-native` API yet, or only
  `c_version()`/`supported_render_backends()` if needed to prove layering.

This slice gives maximum signal on the riskiest unknowns: bindgen compatibility,
linking/loading, libclang/toolchain setup, and diagnostic/status conversion. It
avoids callbacks, render sessions, parent lifetimes, and broad API design until
the foundation is proven.
