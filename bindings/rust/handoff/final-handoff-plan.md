# Final handoff plan: Rust bindings

## Feature summary

Build first-party Rust bindings for the MapLibre Native FFI C ABI. The intended
shape is three crates:

1. `maplibre-native-sys`: generated, unsafe raw C declarations from
   `include/maplibre_native_c.h`.
2. `maplibre-native-support`: internal glue for loading/linking, status and
   diagnostic conversion, memory/string/materializer helpers, callback
   trampolines, and reusable RAII guards.
3. `maplibre-native`: public safe Rust API organized around C API concepts
   (`runtime`, `map`, `render`, `resource`, `geo`, `camera`, etc.).

The first shippable slice should prove toolchain, bindgen compatibility, native
library linkage/loading, status/diagnostic handling, and a tiny safe smoke API
before expanding into lifecycles, callbacks, and rendering.

## External references: what they teach

- Issue #41 asks for raw Rust bindings via `bindgen`, an idiomatic safe Rust
  wrapper, and a smoke example. It mentions `include/maplibre_native_abi.h`, but
  the repo and Rust docs use `include/maplibre_native_c.h`; treat the issue
  header name as stale unless maintainers say otherwise.
- Rust Nomicon FFI guidance supports the repo’s split: keep raw declarations
  unsafe, wrap them in safe RAII/value APIs, use `CString`/`CStr` at string
  boundaries, represent opaque C handles as opaque pointer wrappers, and prevent
  unwinding through C callbacks with `catch_unwind`.
- Rust API Guidelines support explicit `Result<T, Error>`, meaningful error
  types implementing `std::error::Error`, deliberate `Send`/`Sync` auto-trait
  decisions, and trait assertions for raw-pointer wrapper types.
- `bindgen` guidance supports build-time generation with allowlists and layout
  tests; expect a libclang requirement and verify C23 header parsing.
- `libloading` is the right external reference if the Rust binding must exactly
  implement runtime loading from `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`; direct
  bindgen `extern` linkage is simpler but less exact.
- `bitflags` guidance supports forward-compatible C-style flags with unknown-bit
  preservation; public enums that can drift should be `#[non_exhaustive]` or
  include `Unknown(raw)` for outputs.

## Local codebase implications

### Must-follow docs and C ABI facts

- `docs/src/content/docs/development/bindings-rust.md` explicitly defines the
  three-crate Rust architecture, bindgen input (`include/maplibre_native_c.h`),
  dynamic loading expectation, `!Send + !Sync` thread-affine handles,
  `ResourceRequestHandle: Send`, explicit `close() -> Result<()>`, and public
  `Result<T, Error>` behavior.
- `docs/src/content/docs/development/bindings.md` requires deterministic
  release, parent retention where native validity depends on it, immediate
  capture of `mln_thread_last_error_message()` after a non-OK status, hidden raw
  C/sys details, and binding tests through real C calls.
- `include/maplibre_native_c.h` is the umbrella public header.
  `include/maplibre_native_c/base.h` defines `mln_status` values and opaque
  handles. `diagnostics.h` documents the thread-local diagnostic pointer
  lifetime. Runtime/map/render/session headers define wrong-thread and
  invalid-state destroy behavior.
- Public C ABI is currently ABI version `0`; no compatibility shim is needed
  yet.

### Existing implementation model to mirror selectively

The Java FFM binding is the strongest local precedent:

- Generated raw layer from `include/maplibre_native_c.h` using a whitelist.
- Loader checks native access once and validates `mln_c_version() == 0`.
- Status conversion captures diagnostics immediately.
- `HandleState` rejects null handles, tracks live/released state, retains
  parents, releases deterministically, marks released only after successful
  native destroy, and leak-reports finalizer paths.
- Per-call helpers allocate out-pointers, reject embedded NULs, copy C
  strings/string views, and materialize C structs.
- Tests exercise actual C ABI calls for version/backends, network status,
  diagnostics, runtime/map lifecycle, callback installation, released-handle
  checks, parent-lifetime behavior, and wrong-thread mapping.

Rust should translate these patterns into ownership/borrowing/RAII rather than
copying Java’s synchronized state model.

## Recommended approach

### Phase 0: decisions and scaffolding

- Add Rust workspace/crates under `bindings/rust` unless a root workspace is
  deliberately chosen.
- Add Rust build/test tasks in `bindings/rust/mise.toml`; root `mise.toml`
  already treats `bindings/*` as config roots.
- Decide how Rust and libclang are provided. Current top-level tool pins do not
  include Rust, Cargo, or bindgen/libclang.
- Do not immediately wire Rust into top-level `mise run test` until the MVP is
  reliable.

### Phase 1: `maplibre-native-sys`

- Use `bindgen` in `build.rs` against `include/maplibre_native_c.h` with include
  path `include/`.
- Allowlist public symbols/types/constants (`mln_.*`, `MLN_.*`) and generate
  layout tests.
- Link against the existing CMake-produced dynamic library `maplibre-native-c`
  using `$MLN_FFI_BUILD_DIR` when set.
- Add a smoke test that calls `mln_c_version()` and
  `mln_supported_render_backend_mask()`.
- Validate bindgen can parse the C23 header syntax, especially fixed-underlying
  enums.

### Phase 2: `maplibre-native-support`

- Implement status conversion and diagnostics:
  - `ErrorKind::{InvalidArgument, InvalidState, WrongThread, Unsupported, NativeError, Unknown(i32)}`.
  - `Error { kind, raw_status, diagnostic }`.
  - `check(status)` copies `mln_thread_last_error_message()` immediately on
    failure before any other C call.
- Add ABI version validation (`mln_c_version() == 0`).
- Add helper APIs for initialized null out-pointers, non-null wrapping, C
  strings with embedded-NUL rejection, `mln_string_view` materialization, and
  simple RAII native-owned guards.
- Defer callback trampolines until basic handle/status APIs are tested.

### Phase 3: minimal public `maplibre-native`

- Expose tiny process/global APIs first: `c_version()`,
  `supported_render_backends()`, `network_status()`, `set_network_status()`.
- Then add `RuntimeHandle` and `MapHandle` foundations:
  - Private `NonNull<sys::mln_*` fields.
  - `PhantomData<Rc<()>>` for `!Send + !Sync` thread affinity.
  - Released/live state to prevent use-after-close and double destroy.
  - Explicit close that returns `Result<()>`; destroy failure leaves wrapper
    live for retry.
  - Parent retention, likely through an internal
    `Rc<RuntimeInner>`/`Rc<MapInner>` model.
- Add only simple methods initially: runtime create/run/poll/close; map
  create/close and style URL/JSON setters.

### Phase 4+: expand by safer concepts before sharp ones

- Add value descriptors and simple structs (`LatLng`, `ScreenPoint`,
  `EdgeInsets`, camera options) before JSON trees, callbacks, or render
  sessions.
- Add resource transform/provider callbacks only after support has
  panic-catching trampolines, owner-scoped state, replacement rollback, and
  exactly-once `ResourceRequestHandle` behavior.
- Add render sessions/texture frames after runtime/map lifetimes are mature; use
  Rust borrowing to prevent session mutation while a frame is acquired.

## Likely files to change

- New workspace/tasks:
  - `bindings/rust/Cargo.toml` or root `Cargo.toml` plus workspace entries.
  - `bindings/rust/mise.toml`.
  - Possibly `Cargo.lock` if the project chooses reproducible lockfiles for Rust
    builds.
- New sys crate:
  - `bindings/rust/crates/maplibre-native-sys/Cargo.toml`
  - `bindings/rust/crates/maplibre-native-sys/build.rs`
  - `bindings/rust/crates/maplibre-native-sys/wrapper.h`
  - `bindings/rust/crates/maplibre-native-sys/src/lib.rs`
- New support crate:
  - `bindings/rust/crates/maplibre-native-support/Cargo.toml`
  - `src/lib.rs`, `status.rs`, `diagnostics.rs`, `memory.rs`, `loader.rs`,
    `handle.rs`
- New safe crate:
  - `bindings/rust/crates/maplibre-native/Cargo.toml`
  - `src/lib.rs`, `error.rs`, `runtime.rs`, `map.rs`, and later `render.rs`,
    `resource.rs`, `geo.rs`, `camera.rs`
  - `tests/*.rs`
- Later integration:
  - top-level `mise.toml` Rust tool pin/task dependency
  - `hk.pkl` for `cargo fmt`/`clippy`/tests
  - CI workflows
  - examples and docs, after the API shape stabilizes

## Constraints and invariants

- Generate raw declarations from `include/maplibre_native_c.h`; do not hand-edit
  generated bindings.
- Keep `sys` raw details internal to safe public APIs. Public Rust should not
  expose raw sys pointers except narrow unsafe backend interop
  (`NativePointer`).
- Never panic on native statuses. Convert statuses to `Result`, preserving raw
  status and copied diagnostics.
- Capture thread-local diagnostics immediately after non-OK C status.
- Thread-affine handles are `!Send + !Sync`; `ResourceRequestHandle` is the
  documented `Send` exception.
- No internal thread dispatch for owner-thread-affine handles.
- Parent handles stay alive while child native validity depends on them;
  `MapProjectionHandle` is the documented exception.
- Native destroy failures leave the wrapper live; successful close makes later
  close no-op.
- `Drop` must not panic and must avoid double release. Prefer explicit close for
  correctness; if `Drop` destroys, failures can only be recorded/logged.
- Callback trampolines must catch panics and never unwind through C.
- Resource provider PASS_THROUGH means the binding must not retain, complete, or
  release the request handle.
- Texture frame backend pointers are borrowed and valid only until release; tie
  any exposed pointer access to frame lifetime.

## Non-goals for the first slice

- Full Java FFM surface parity.
- Resource provider/transform callback implementation.
- Render sessions, texture frames, and backend-specific GUI examples.
- Full JSON/GeoJSON tree APIs.
- Thread dispatch/executor abstraction for wrong-thread calls.
- Compatibility shims for ABI versions other than current `0`.
- Publishing metadata/docs.rs polish before local tests are reliable.

## Validation

Initial targeted checks after scaffolding exists:

```bash
mise run //:ensure-native-library
mise run //bindings/rust:build
mise run //bindings/rust:test
cargo fmt --all --check
cargo clippy --workspace --all-targets -- -D warnings
```

Repository-wide checks before handoff/merge:

```bash
mise run test
mise run fix
```

Specific tests to include early:

- `maplibre-native-sys` smoke call to `mln_c_version()` and supported backend
  mask.
- Status diagnostic capture using an invalid C call such as bad network status,
  mirroring Java `StatusAndMemoryTest`.
- Embedded-NUL rejection for public null-terminated string inputs.
- Runtime create/run/poll/close and close idempotence/released-handle
  pre-dispatch errors.
- Map create/close and runtime parent-retention/live-child close behavior.
- Compile-time trait assertions for `!Send + !Sync` handles and `Send` resource
  request handle once implemented.

## Main risks

- **Loading model:** Direct `extern` linkage is much simpler but may not satisfy
  docs that promise exact-path runtime loading via
  `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`. `libloading` better matches Java but
  changes the sys/support architecture substantially.
- **bindgen/libclang/C23:** The C headers use C23 features and fixed-underlying
  enums; a sufficiently new libclang and correct clang args may be required.
- **Toolchain integration:** The repo currently has no Rust tool pin, Cargo
  workspace, Rust formatter/lint hook, or Rust CI.
- **Drop semantics:** Native destroy can fail due wrong thread/live children;
  Rust `Drop` cannot return errors.
- **Auto-traits:** Raw pointer wrappers can accidentally become `Send`/`Sync`;
  use `PhantomData<Rc<()>>` and static trait tests.
- **Callback lifetime/replacement:** Dropping closure state while native
  callbacks are in flight is high risk; defer until foundations are mature.
- **Forward compatibility:** Preserve unknown status/enum/flag raw values where
  the C ABI may grow.

## Unresolved questions needing approval

1. Should Rust use direct bindgen `extern` dynamic linkage or a `libloading`
   function table for exact runtime library loading?
2. Should the Cargo workspace live entirely under `bindings/rust`, or should the
   repo have a root Cargo workspace?
3. Should `Cargo.lock` be committed for this library workspace? Given the repo’s
   tool-pinning culture, committing it is likely safer.
4. Where should Rust and libclang be provisioned: top-level `mise.toml`, pixi,
   system/Xcode, or a combination?
5. When should Rust tests become part of top-level `mise run test` and CI?
6. Should `Drop` attempt native destroy for thread-affine handles, or only
   leak-report and require explicit close? Rust docs imply best-effort destroy
   is acceptable because handles are `!Send`, but this still needs a policy
   decision.
7. What public close shape is preferred: consuming `close(self)`, mutable
   `close(&mut self)`, or idempotent interior-state `close(&self)`?

## Compact implementation-ready meta-prompt for next worker/planner

Goal: Add the initial Rust binding foundation for MapLibre Native FFI without
attempting full API parity. Create Rust crate scaffolding, generated
`maplibre-native-sys` bindings from `include/maplibre_native_c.h`, basic support
status/diagnostic helpers, and the smallest safe public smoke API/tests needed
to prove bindgen, linking/loading, and diagnostic conversion.

Context/evidence: Follow `docs/src/content/docs/development/bindings-rust.md`
for the three-crate split, dynamic loading expectation, `!Send + !Sync` handles,
status/error rules, and materializer/callback boundaries. Follow
`docs/src/content/docs/development/bindings.md` for deterministic release,
parent retention, diagnostics capture, and testing through the C ABI. Mirror
Java FFM patterns for generated raw layer, loader/version check, `Status.check`,
`HandleState`, out-pointer helpers, and tests, but translate them to Rust
ownership/RAII.

Success criteria: `cargo test` or `mise run //bindings/rust:test` builds the
native C library dependency, generates/compiles sys bindings with layout tests,
calls `mln_c_version()`, captures diagnostics from a failing C status, and
exposes at most a tiny safe API such as `c_version()`/backend mask/network
status. Raw sys pointers are not exposed through the safe crate. Generated
bindings are not hand-edited.

Hard constraints: Use `include/maplibre_native_c.h` unless maintainers
explicitly approve another header. Capture diagnostics immediately on failure.
Do not panic on native statuses. Preserve raw statuses. Keep thread-affine
handle design `!Send + !Sync` when handles are introduced. Do not implement
callbacks/render sessions in the first foundation slice unless explicitly
rescoped.

Suggested approach: Start under `bindings/rust` with sys/support/public crates.
Implement bindgen `build.rs` with allowlists and link search from
`$MLN_FFI_BUILD_DIR`. Add support `ErrorKind`, `Error`, `Result`, and
`check(status)`. Add C string/out-pointer helpers only as needed for tests.
Decide or escalate the runtime loading model before investing deeply: direct
dynamic linkage for speed, or `libloading` for exact env-path semantics.

Validation: Run `mise run //:ensure-native-library`, the new Rust build/test
task, `cargo fmt --all --check`, and
`cargo clippy --workspace --all-targets -- -D warnings` if configured. Before
final merge, run `mise run test` and `mise run fix`.

Stop/escalation rules: Escalate before choosing direct linkage vs `libloading`,
root vs nested Cargo workspace, lockfile policy, Rust/libclang provisioning,
top-level test integration, and `Drop` destruction policy. Stop after foundation
tests pass; do not broaden into callbacks/rendering/full map parity in the same
slice.
