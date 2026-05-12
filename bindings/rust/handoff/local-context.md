# Local context: implementing Rust bindings

## Current state

- There is **no first-party Rust binding scaffold yet** in the repo.
  `find Cargo.toml` only finds vendored/third-party crates under `third_party/`;
  no root Cargo workspace or `bindings/rust` crate exists.
- Rust binding conventions already define the intended crate split and semantics
  in `docs/src/content/docs/development/bindings-rust.md`.
- Existing Java FFM binding is the best local implementation model for a direct
  C binding: it has public concept packages, generated/raw C declarations hidden
  internally, materializer helpers, handle state, status/diagnostic conversion,
  callback state, native-library loading, C-ABI tests, and a LWJGL example.

## High-value files and evidence

### Binding architecture docs

- `docs/src/content/docs/development/bindings.md:17-78`
  - Shared binding design: protect host programs; preserve C API model; keep
    wrappers regular; separate layers into internal C layer, internal support
    layer, and public binding layer.
  - Owned long-lived native objects use `*Handle` names (`RuntimeHandle`,
    `MapHandle`, `MapProjectionHandle`, `RenderSessionHandle`).
- `docs/src/content/docs/development/bindings.md:86-150`
  - Handle lifetime and threading: deterministic release; later release is
    no-op; wrong-thread destroy returns language wrong-thread error;
    finalizers/Drop must be treated carefully; parent validity must be
    preserved; owner-thread model for runtime/map/projection/session; statuses
    capture thread-local diagnostics immediately.
- `docs/src/content/docs/development/bindings.md:152-238`
  - Type mapping and data ownership: descriptors hide `size`/field masks; enums
    map explicitly; native pointer values are opaque/borrowed; UTF-8 strings;
    reject embedded NUL for null-terminated inputs; copied snapshots/results;
    scoped borrows for texture frames.
- `docs/src/content/docs/development/bindings.md:241-305`
  - Callback lifetimes: state strongly retained for native scope, callbacks may
    arrive on arbitrary MapLibre worker/network/log/render threads, catch host
    failures, resource provider request handling is exactly once.
- `docs/src/content/docs/development/bindings.md:307-322`
  - Binding tests focus on adaptation: wrappers, ownership, copying, callbacks,
    threading errors, error mapping, with small tests around real C calls.

### Rust-specific conventions

- `docs/src/content/docs/development/bindings-rust.md:14-49`
  - Target architecture:
    - `maplibre-native-sys`: generated unsafe declarations for public C ABI.
    - `maplibre-native-support`: shared glue above sys: status conversion,
      diagnostics, descriptor materializers, callback trampolines, build/link
      utilities.
    - `maplibre-native`: public safe Rust crate.
  - Generate sys via `bindgen` from `include/maplibre_native_c.h`;
    generation/compilation/layout tests are the Rust bindability check. Do not
    hand-edit generated bindings.
  - Public modules group C API concepts (`runtime`, `map`, `render`). Generated
    types, raw pointers, field masks, callback trampolines stay internal.
  - Native library is loaded dynamically at runtime: first
    `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`, then system library search path.
- `docs/src/content/docs/development/bindings-rust.md:51-71`
  - Public owned values are plain Rust structs; mutable option structs use
    `Default` and builder-style setters; field masks derive from `Option<T>` or
    explicit setters and stay internal; public enums are `#[non_exhaustive]`;
    output enum drift uses `Unknown(raw)` variants; bit masks use `bitflags`;
    JSON/GeoJSON preserve integer width, object member order, and duplicate
    keys.
- `docs/src/content/docs/development/bindings-rust.md:73-97`
  - Thread-affine handles use `PhantomData<Rc<()>>` to be `!Send + !Sync`;
    `ResourceRequestHandle` is `Send`; `MapProjectionHandle` remains `!Send`; no
    internal thread dispatch; child handles hold parents strongly except
    projection; safe Rust should prove thread-affine `Drop` on owner thread;
    explicit `close` returns `Result<()>`.
- `docs/src/content/docs/development/bindings-rust.md:100-120`
  - `pub type Result<T> = std::result::Result<T, Error>`; never panic on native
    status; `Error` stores mapped kind, raw `mln_status`, copied diagnostic;
    handle-creating functions null-init out pointers and wrap only successful
    non-null handles; C presence booleans become `Result<Option<T>>` /
    `Result<bool>`.
- `docs/src/content/docs/development/bindings-rust.md:122-136`
  - Materialize C inputs at call boundary using stack/CString/Vec/arenas; RAII
    guards for snapshots/lists; public safe APIs have no raw sys pointers;
    `NativePointer` construction/reconversion is unsafe and limited to backend
    handle APIs.
- `docs/src/content/docs/development/bindings-rust.md:138-167`
  - Callback trampolines live in support; adapt C function pointers to Rust
    closures/trait objects; copy/wrap arguments; `catch_unwind`; callback state
    that may run on worker/network/log/render threads requires
    `Send + Sync + 'static`; resource request handle exactly-once release.
- `docs/src/content/docs/development/bindings-rust.md:170-193`
  - Render descriptors are Rust values; surface/borrowed texture descriptors
    store backend objects as `NativePointer`; session owns map strongly; texture
    readback has `read_premultiplied_rgba8_into(&mut [u8])` and convenience
    copied image; frame handles provide scoped unsafe backend-pointer access
    tied to frame lifetime.

### Java docs/patterns to mirror selectively

- `docs/src/content/docs/development/bindings-java-ffm.md:18-49`
  - Java FFM architecture mirrors the intended Rust split: generated C layer in
    `internal.c`, support packages for loading/memory/structs/status, public
    concept packages. It calls `NativeAccess.ensureLoaded()` before generated
    classes. Lookup order includes property/env/system path.
- `docs/src/content/docs/development/bindings-java-ffm.md:51-97`
  - Public process-global entry point, `AutoCloseable` handles, immutable copied
    records, mutable descriptors, status-to-exception conversion, leak-only
    cleaners.
- `docs/src/content/docs/development/bindings-java-ffm.md:99-116`
  - FFM memory categories and helpers: per-call arenas, callback/reusable
    storage, C string rejection, native pointer opacity, `NativeBuffer` for
    reusable off-heap byte storage.
- `docs/src/content/docs/development/bindings-java-ffm.md:118-164`
  - Callback and render session rules parallel Rust docs.
- `docs/src/content/docs/development/bindings-java-jni.md:25-42`
  - Future JNI bridge is explicitly expected to be Rust over shared Rust support
    crates; bridge code catches panics and translates statuses/diagnostics to
    Java exceptions.

## Existing Java FFM implementation patterns

### Build/module/generated C layer

- `settings.gradle.kts:1-5` includes only `:bindings:java-ffm` and
  `:examples:lwjgl-map` in Gradle.
- `build.gradle.kts:1-9` sets Java toolchain to Java 25 for all Java projects.
- `bindings/java-ffm/build.gradle.kts:1-20`
  - Uses `java-library` and `de.infolektuell.jextract` plugin `1.4.0`.
  - Generates from root `include/maplibre_native_c.h` with includes `include/`,
    header class `MapLibreNativeC`, target package
    `org.maplibre.nativeffi.internal.c`, whitelist file
    `src/jextract/maplibre-native-c.includes`.
  - Generated `internal.c` sources are not checked in under `src/main/java`;
    they are build output.
- `bindings/java-ffm/build.gradle.kts:31-45`
  - Tests use native library path from `MLN_FFI_BUILD_DIR` (default root
    `build/host`) and set JVM arg `--enable-native-access=ALL-UNNAMED`.
- `bindings/java-ffm/mise.toml:1-3`
  - Java binding build task depends on `//:ensure-native-library` and
    `:jextract:update-includes`, then runs Gradle build.
- `bindings/java-ffm/src/main/java/module-info.java:1-15`
  - Exports only public packages (`org.maplibre.nativeffi`, `camera`, `error`,
    `geo`, `json`, `log`, `map`, `offline`, `query`, `render`, `resource`,
    `runtime`, `style`); internals remain unexported.

### Native library loading

- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/Maplibre.java:24-36`
  - Root process-global entry point. `loadNativeLibrary()`,
    `loadNativeLibrary(Path)`, `cVersion()`, `supportedRenderBackends()` all
    ensure native access before calling generated C.
- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/loader/NativeLibrary.java:9-23`
  - Constants: library name `maplibre-native-c`, property
    `org.maplibre.nativeffi.library.path`, env
    `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`.
- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/loader/NativeLibrary.java:25-64`
  - Load-once with synchronized lock; lookup order: Java property, env var,
    `System.loadLibrary`.
- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/loader/NativeAccess.java:10-37`
  - Expected ABI version is `0`; `ensureLoaded()` loads once and validates
    ABI/symbol/native-access errors.
- Rust should implement the documented Rust lookup
  (`MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`, then system path) and ABI/symbol
  validation analogously, likely in `support` or `sys` runtime loader.

### Status and diagnostics

- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/status/Status.java:8-28`
  - `check(int)` maps native status to public status enum; on non-OK throws a
    typed exception with `captureDiagnostic()` from
    `mln_thread_last_error_message()`.
  - `released(typeName)` creates a binding-owned invalid-state error before
    native dispatch.
- `include/maplibre_native_c/diagnostics.h:1-22`
  - `mln_thread_last_error_message()` returns C thread-local diagnostic; must be
    read on same thread immediately after failure.
- Rust equivalent should have `ErrorKind`,
  `Error { kind, raw_status, diagnostic }`, `Status::check` or similar, and a
  released-state error path that does not call C.

### Handle lifecycle/parent retention

- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/lifecycle/HandleState.java:15-40`
  - Stores type name, native handle, parent references, released flag, leak
    report. Null handles rejected.
- `HandleState.java:42-68`
  - `requireLive()` rejects released wrappers; `closeOnce()` calls native
    destroy, marks released only on successful status, and then runs cleanup.
    Later close is no-op.
- `HandleState.java:70-97`
  - Cleaner reports leaked handles but does not destroy them.
- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/runtime/RuntimeHandle.java:39-55`
  - Runtime owns `HandleState` and live map registry; static `create` allocates
    null out pointer, calls `mln_runtime_create`, wraps returned handle.
- `RuntimeHandle.java:223-241`
  - `close()` uses `mln_runtime_destroy`; callbacks are closed after successful
    native destroy.
- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/map/MapHandle.java:50-76`
  - Map strongly retains runtime via field and `HandleState(..., runtime)`;
    creation calls `mln_map_create`, registers map with runtime.
- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/render/RenderSessionHandle.java:27-42`
  - Render session strongly retains map via field and `HandleState(..., map)`.
- Rust equivalent likely wants private handle structs that contain
  `NonNull<sys::mln_*>, PhantomData<Rc<()>>`, parent ownership/borrows,
  live/released state for explicit `close`, and `Drop` for best-effort
  owner-thread destruction where safe.

### Per-call memory/materializers

- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/memory/MemoryUtil.java:9-62`
  - Allocates C strings with embedded-NUL rejection, copies C strings/string
    views/bytes, allocates null-initialized pointer out params, null checks.
- `bindings/java-ffm/src/main/java/org/maplibre/nativeffi/internal/struct/CoreStructs.java:24-160`
  - Bidirectional materializers/readers for simple value structs (`LatLng`,
    `ScreenPoint`, arrays, etc.).
- `CoreStructs.java:162-176`
  - `stringView(String, Arena)` encodes UTF-8 bytes and fills `mln_string_view`
    data/size; empty string uses null+0.
- Rust equivalent: support crate should own C-string/string-view materialization
  (`CString` for null-terminated C APIs; byte slices for `mln_string_view`),
  out-pointer initialization, array backing storage, and snapshot RAII.

### Public API examples

- `Maplibre.java:38-57`
  - Process-global `networkStatus()` uses arena + int out param + status
    conversion; `setNetworkStatus()` validates Java null, lets C validate enum
    value.
- `MapHandle.java:78-96`
  - Public string APIs use `MemoryUtil.allocateCString`, so embedded NUL is
    rejected before native call.
- `RenderSessionHandle.java:44-126`
  - Static attach methods allocate out-session pointer, materialize descriptor,
    call native attach, return session handle.
- `ResourceRequestHandle.java:17-35`
  - Resource request handle is special: owns provider request reference while
    Java handles the request; not a thread-affine `HandleState` wrapper.
- `ResourceRequestHandle.java:45-66`
  - `complete()` enforces one-shot completion and releases only after callback
    decision ownership is finalized.
- `ResourceRequestHandle.java:86-119`
  - Provider decision logic: completion before callback returns forces HANDLE;
    PASS_THROUGH means Java must not retain/release native handle.

## C API surface and constraints most relevant to Rust bindings

- `include/maplibre_native_c.h:1-33`
  - Umbrella public header. Functions on thread-affine handles validate caller
    thread and return `MLN_STATUS_WRONG_THREAD`; status-returning functions
    clear diagnostics on entry; read `mln_thread_last_error_message()` after
    synchronous failures before another C call; C23 header.
- `include/maplibre_native_c/base.h:31-49`
  - Fixed status enum values: OK `0`, invalid argument `-1`, invalid state `-2`,
    wrong thread `-3`, unsupported `-4`, native error `-5`.
- `base.h:52-66`
  - Opaque handles: `mln_runtime`, `mln_map`, `mln_map_projection`, offline
    snapshots/lists, JSON snapshot, resource request handle, render session.
- `base.h:68-78`
  - ABI version and supported render backend mask.
- `include/maplibre_native_c/map.h:142-149`
  - `mln_map_options` has `size`, dimensions, scale factor, map mode.
- `map.h:247-251`
  - `mln_string_view` is pointer+size; pointer may be null only when size is 0.
- `map.h:253-382`
  - JSON/geometry descriptor trees (`mln_geometry`, `mln_json_value`) use nested
    pointers/unions and require materializer backing storage/lifetimes.
- `map.h:853-869`
  - `mln_map_options_default()` and `mln_map_create(runtime, options, out_map)`.
- `map.h:931-968`
  - `mln_map_destroy`, `mln_map_set_style_url`, `mln_map_set_style_json`
    illustrate thread-affine map APIs and null-terminated string inputs.
- `include/maplibre_native_c/runtime.h:335-372`
  - Resource transform callback returns replacement URL via
    `mln_resource_transform_response`; copied on return, callback storage valid
    until next callback/thread/runtime teardown.
- `runtime.h:374-448`
  - Resource provider callback contract: request fields borrowed for callback
    duration; PASS_THROUGH means do not retain/complete/release handle; HANDLE
    lets provider complete inline or later; completion data copied;
    `mln_resource_request_complete()` may be called from any thread; provider
    releases handled handles exactly once.
- `runtime.h:469-494`
  - Runtime creation and resource provider install.
- `runtime.h:514-542`
  - Resource request complete/cancelled/release APIs.
- `runtime.h:563-602`
  - Resource transform install and runtime destroy.
- `runtime.h:647-649` plus `runtime.h:631-634`
  - Runtime event polling: message/payload string pointers valid until next poll
    or runtime destroy; binding must copy immediately.
- `include/maplibre_native_c/render_session.h:17-88`
  - Session resize/render/detach/destroy: owner-thread affine; invalid while
    texture frame acquired; destroy fails if frame acquired.
- `include/maplibre_native_c/render_session.h:99-197`
  - Feature-state APIs use JSON snapshots; `mln_json_snapshot_get()` borrows
    root until snapshot destroy; binding should RAII release after copying.
- `include/maplibre_native_c/texture.h:15-156`
  - Texture descriptors and frames include `size`, dimensions/scale, borrowed
    backend `void*` handles, generation/frame IDs, pixel format/layout metadata.
- `texture.h:212-356`
  - Texture attach methods create one render session per map; backend handles
    are borrowed and must remain valid/synchronized by caller; unsupported
    backend returns `MLN_STATUS_UNSUPPORTED`.
- `texture.h:358-384`
  - CPU readback copies premultiplied RGBA8 into caller-owned storage and fills
    `mln_texture_image_info`; too-small buffer returns invalid argument but
    still fills info.
- `texture.h:386-518`
  - Owned texture frame acquire/release APIs; returned backend pointers valid
    only until matching release; while acquired,
    resize/render/detach/destroy/second acquire return invalid state.

## Build/test/tooling context

- `docs/src/content/docs/development/overview.md:55-69`
  - Common commands: `mise run test`, `mise run build`, `mise run fix`, examples
    via mise.
- `docs/src/content/docs/development/overview.md:79-111`
  - Tool ownership: mise orchestrates; pixi supplies native build env; CMake
    builds native C/C++; language package managers own language dependencies;
    Cargo should own Rust dependencies.
- `mise.toml:19-20`
  - Monorepo config roots are `bindings/*`, `examples/*`, `docs`. A future
    `bindings/rust/mise.toml` should be picked up automatically.
- `mise.toml:22-43`
  - Pinned tools currently include Zig and Java but **not Rust/Cargo/bindgen
    tooling**. Adding Rust likely requires adding a Rust tool entry or relying
    on system Rust; better to pin Rust if adding build tasks.
- `mise.toml:54-64`
  - Env includes `MLN_FFI_BUILD_DIR` defaulting to `build/host`, plus
    `DYLD_LIBRARY_PATH`/`LD_LIBRARY_PATH` pointing there.
- `mise.toml:106-126`
  - Root `build` configures/builds CMake native lib; root `test` depends on
    build then runs Zig C ABI tests with
    `-Dcmake-artifact-dir=$MLN_FFI_BUILD_DIR` and optional render backend.
- `.mise/tasks/ensure-native-library:1-30`
  - Non-CI ensures native library by running root build. In CI, it checks for
    downloaded artifacts under `$MLN_FFI_BUILD_DIR` or `build/`.
- `CMakeLists.txt:1-25`
  - Native project `maplibre_native_c`; target `maplibre_native_c`; alias
    `maplibre_native_ffi::c`.
- `build.zig:23-30`
  - Existing Zig tests link by adding `include`, library path/RPATH to CMake
    artifact dir, and `linkSystemLibrary("maplibre-native-c")`.
- `dprint.jsonc:1-70`
  - Formatting orchestrated by dprint; existing languages include C/C++, CMake,
    Kotlin, Java, Zig, JSON/Markdown/etc. No Rust formatter entry yet.
- `hk.pkl:8-46`
  - Checks are dprint, actionlint, jvl, docs vp-check, ruff, ty. No Rust
    clippy/fmt check yet.

## Existing tests and validation patterns

- Java FFM tests exercise real C ABI calls and binding-owned invariants.
- `bindings/java-ffm/src/test/java/org/maplibre/nativeffi/MaplibreTest.java:17-72`
  - Loads native library once, verifies C version/backends, network status
    get/set, log callback install/clear, projected meters conversions.
- `bindings/java-ffm/src/test/java/org/maplibre/nativeffi/internal/status/StatusAndMemoryTest.java:15-51`
  - Verifies diagnostic capture from invalid native status, embedded NUL
    rejection, native pointer round trip without exposing memory segment.
- `bindings/java-ffm/src/test/java/org/maplibre/nativeffi/runtime/RuntimeHandleTest.java:43-63`
  - Runtime create/run/poll/close; close idempotence; released wrapper rejects
    before native dispatch.
- `RuntimeHandleTest.java:65-177`
  - Resource transform/provider callback install; install after map creation
    invalid; resource provider completes style request inline and async;
    one-shot completion checks.
- `RuntimeHandleTest.java:179-222`
  - Runtime event payload/message copied before later polls; wrong-thread
    runtime methods map to wrong-thread exceptions; wrong-thread close leaves
    handle live.
- `bindings/java-ffm/src/test/java/org/maplibre/nativeffi/map/MapHandleTest.java:29-75`
  - Map creation/close, native semantic validation, parent-live invariant
    (runtime close fails while map live), released map pre-dispatch checks.
- `MapHandleTest.java:77-99`
  - Null-terminated public style string inputs reject embedded NUL.
- `MapHandleTest.java:101-220+`
  - Debug/state helpers, camera commands, coordinate conversions, native
    validation for empty inputs, projection helpers.
- Suggested Rust initial tests should mirror these using `cargo test` against
  the built native library: load/version/backends, network status, status
  diagnostic capture, CString NUL rejection, runtime
  create/run/poll/close/idempotence, released wrapper errors, map parent
  retention, wrong-thread error if feasible, resource provider one-shot
  semantics once callbacks are implemented.

## Examples/integration patterns

- `examples/lwjgl-map/build.gradle.kts:1-84`
  - Example depends on Java FFM binding and native C library path; JVM args
    include native access and platform-specific Vulkan loader. Good pattern for
    example-specific native path setup.
- `examples/lwjgl-map/mise.toml:1-13`
  - Example build/run tasks depend on `//:ensure-native-library`; GUI examples
    are run through mise with args.
- `examples/lwjgl-map/src/main/java/.../Main.java:13-39`
  - Example checks `Maplibre.supportedRenderBackends()` before running; prints
    selected mode and native library path; then owns runtime/render loop.
- Rust examples should likely be non-GUI first (e.g., c-version/network/runtime
  smoke or texture readback) unless adding render-backend integration; use brief
  timeouts for GUI/interactive examples per AGENTS.md.

## Likely implementation approach

1. Add a Cargo workspace/crate layout (likely under `bindings/rust/`, or root
   workspace if preferred):
   - `maplibre-native-sys`: `bindgen` generated from
     `include/maplibre_native_c.h`, plus build/link/dynamic-loading strategy and
     layout tests.
   - `maplibre-native-support`: status/diagnostic conversion, C
     string/string-view/materializer helpers, RAII snapshot/list guards,
     callback trampolines, loader utilities shared with future bridge bindings.
   - `maplibre-native`: public safe API modules (`runtime`, `map`, `render`,
     `resource`, `geo`, `camera`, `json`, `style`, etc.) matching C concepts.
2. Decide dynamic loading mechanism early. Rust docs require runtime dynamic
   loading via env exact path then system search. Options include `libloading`
   with generated `bindgen` function-pointer wrappers (more work but matches
   runtime loading) or link-time sys bindings plus dynamic linker search path
   (simpler but less like docs). This is a design decision/risk.
3. Start thin and testable:
   - sys generation and compile/layout tests.
   - loader + `Maplibre::c_version`/supported backends/network status.
   - `Error`/`Result`/status mapping with diagnostic capture.
   - `RuntimeHandle` and `MapHandle` lifecycle + parent retention.
   - C-string/string-view materializers and a couple of value structs.
4. Add callbacks/resource provider only after handle/status foundations are
   solid; these are high-risk due arbitrary callback threads, panic containment,
   and exactly-once request ownership.
5. Add render sessions/texture frames after map/runtime because frame lifetimes
   should use Rust borrowing to prevent reentrant session calls while frame
   handle is live.

## Constraints and invariants

- Public C API is C23 and ABI version is currently `0`; do not build
  compatibility shims for future/old C ABI yet
  (`docs/src/content/docs/development/c-conventions.md:15-30`).
- Raw generated C/sys details must remain internal; public Rust crate should not
  expose raw sys pointers except narrow unsafe backend interop through
  `NativePointer`-style values.
- Status-returning calls must never panic on native statuses; capture
  thread-local diagnostic immediately after non-OK status and before any other C
  call.
- Thread-affine handles are `!Send + !Sync`; `ResourceRequestHandle` is `Send`;
  no internal thread dispatch.
- Parent retention: children keep parent alive while native validity depends on
  it; `MapProjectionHandle` is the documented exception.
- Finalizers/Drop are not a substitute for explicit close for
  fallible/thread-affine destruction; Rust docs nevertheless say safe `!Send`
  handles can drop on owner thread and `Drop` calls destroy while recording
  diagnostics on failure. Be careful with close-vs-drop double release
  semantics.
- Callback adapters must catch panics and never unwind through C.
- Resource provider PASS_THROUGH means binding must not retain, complete, or
  release the request handle.
- Texture frame backend pointers are borrowed and valid only until release; Rust
  APIs should tie pointers to frame lifetime.
- Tests should exercise the public C ABI through Rust where practical.

## Risks / open questions

- **Dynamic loading vs bindgen ergonomics:** bindgen naturally emits direct
  extern calls; docs require runtime dynamic loading. Need choose/implement an
  approach (`libloading`, generated wrappers, or reconsider docs). This is the
  biggest early architecture decision.
- **Toolchain pinning:** `mise.toml` currently has no Rust tool. Adding Rust
  tasks likely needs a pinned Rust/Cargo tool and possibly bindgen/libclang
  availability through pixi/mise.
- **C23/fixed-underlying enums and bindgen:** verify bindgen/libclang parses the
  public headers as intended. Sys compile/layout tests should reveal issues.
- **Crate placement/naming:** docs name crates with hyphens
  (`maplibre-native-sys`, etc.); directory/workspace naming and package
  publishing metadata are not established.
- **Build integration:** decide whether root `mise run test` should include
  Cargo tests immediately or whether Rust has a scoped task first. CI impacts
  need planning.
- **Generated files:** Java does not check in generated C declarations. Rust
  docs say do not hand-edit generated bindings, but do not explicitly say
  whether generated Rust should be checked in. Decide with maintainers.
- **Callback teardown/replacement:** Java has careful replacement ordering; Rust
  support must match to avoid dangling callback state if native install fails.
- **Drop error reporting:** Rust has no return from `Drop`; docs say record
  diagnostics. Need decide logging/storing mechanism and testability.

## Useful validation commands

- Native build/test baseline: `mise run test`.
- Formatting/lints baseline: `mise run fix` (or `mise run check` for checks
  only).
- Java FFM binding build/tests: `mise run //bindings/java-ffm:build`.
- Native library presence for language tests:
  `mise run //:ensure-native-library`.
- Future Rust scoped validation should likely be:
  `mise run //bindings/rust:build`, `mise run //bindings/rust:test`, or
  `cargo test` with
  `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH=$MLN_FFI_BUILD_DIR/$(system library name)`/library
  path configured.
