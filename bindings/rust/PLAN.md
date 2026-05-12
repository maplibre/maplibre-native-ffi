# Rust bindings implementation plan

## Goal

Build first-party Rust bindings over the public C ABI until they reach rough
feature maturity with the Java bindings. The finished scope for this project
includes:

- generated unsafe C declarations in `maplibre-native-sys`;
- reusable internal glue in `maplibre-native-support`;
- an idiomatic safe Rust crate, `maplibre-native`, that preserves the C API
  model while using Rust ownership, `Result`, RAII, lifetimes, and explicit
  unsafe boundaries;
- tests that exercise real C ABI calls and prove Rust-specific lifetime,
  threading, error, callback, and rendering invariants;
- CI matrix entries for the Rust binding and Rust example once they are
  buildable;
- a Rust test app that opens a window and renders a map through Vulkan.

Out of scope for this PR/project: publishing metadata, API reference generation,
standalone user documentation, docs.rs polish, and broad release packaging.

## Reference artifacts

The subagent handoff artifacts are copied into this directory for reference:

- [`handoff/final-handoff-plan.md`](handoff/final-handoff-plan.md)
- [`handoff/external-reference.md`](handoff/external-reference.md)
- [`handoff/local-context.md`](handoff/local-context.md)
- [`handoff/implementation-strategy.md`](handoff/implementation-strategy.md)

Primary project docs:

- [`docs/src/content/docs/development/bindings.md`](../../../docs/src/content/docs/development/bindings.md)
- [`docs/src/content/docs/development/bindings-rust.md`](../../../docs/src/content/docs/development/bindings-rust.md)
- [`docs/src/content/docs/development/bindings-java-ffm.md`](../../../docs/src/content/docs/development/bindings-java-ffm.md)
- [`docs/src/content/docs/development/bindings-java-jni.md`](../../../docs/src/content/docs/development/bindings-java-jni.md)

Use the Java FFM binding as the local reference for maturity, generated raw
declarations, loader/version checks, status conversion, handle state, memory
helpers, callbacks, rendering, and tests. Translate those patterns to Rust
ownership and RAII rather than copying Java's synchronized object model.

## Missing API parity backlog

These public C and Java FFM surfaces remain to reach the branch's parity target.
Implement them on this branch unless a later decision explicitly narrows Rust's
scope.

Runtime and process-global APIs:

- [x] `RuntimeOptions`, including asset path, cache path, and maximum cache
      size.
- [x] Runtime creation with explicit options.
- [x] Ambient cache operations.
- Process-global logging callbacks, callback clearing, log severity values, and
  async severity mask configuration.

Map and style APIs:

- Style source removal and source existence checks.
- Style source type, source info, attribution, and related copied source output
  types.
- Style image add/remove/query APIs and image metadata/value types.
- Image source APIs for URL, coordinates, and image updates.
- Remaining layer/source helpers that Java FFM exposes over the C style API.

Render session and query APIs:

- Feature state set, get, and remove on `RenderSessionHandle`.
- `FeatureStateSelector` and related selector materialization.
- Rendered feature query geometry and rendered/source query option types.
- Copied queried feature and feature-extension result types.
- Rendered feature, source feature, and feature extension query methods on
  `RenderSessionHandle`.

Keep this backlog aligned with `include/maplibre_native_c/*.h` and
`bindings/java-ffm/src/main/java/org/maplibre/nativeffi/**` as those APIs grow.

## API polish backlog before review

Polish the Rust surface fully on this branch before external review. Treat these
items as part of the branch's completion criteria, not as minimal follow-up
fixes.

Handle lifecycle shape:

- Revisit every public destructive or one-shot operation, including runtime,
  map, projection, render session, texture frame, and resource request APIs.
- Prefer consuming operations such as `close(self) -> Result<()>` and
  `complete(self, response) -> Result<()>` where the operation logically ends
  the handle's useful life.
- Preserve retry-after-native-destroy-failure semantics with an explicit
  pattern, such as returning the still-live handle in a close error, when the
  native C API leaves ownership with the caller on failure.
- Keep successful cleanup internally idempotent for `Drop` and shared internal
  state, even when the public explicit operation consumes the wrapper.

Frame backend pointer lifetimes:

- Replace frame-derived bare `NativePointer` returns with a lifetime-bearing
  type, such as `FrameNativePointer<'frame>`.
- Tie `FrameNativePointer<'frame>` to the active texture frame handle so safe
  Rust code cannot store the pointer beyond frame release.
- Use consuming or mutable frame release APIs where needed so the borrow checker
  prevents release while a frame pointer borrow is still live.
- Keep GPU waiting and synchronization with the caller; the Rust API should
  encode pointer lifetime without requiring callback-scoped GPU waits.

Module structure:

- Split large Rust modules before adding the remaining parity APIs.
- Align the public/internal module split with the Java FFM package structure
  where it maps cleanly to Rust: map, style, render, query, resource, runtime,
  logging, geometry, JSON/GeoJSON, values, and internal support.
- Keep tests colocated with the submodule or concern they exercise so new parity
  work does not keep growing monolithic test modules.

## Decisions

- Use normal direct Rust FFI linkage.
  - Generate raw `extern "C"` declarations with `bindgen`.
  - Link to `maplibre-native-c` from Cargo build metadata.
  - Do not build a `libloading` function table unless a later requirement
    changes the loading model.
- Use a root Cargo workspace for Rust packages in this monorepo.
  - Place binding crates under `bindings/rust/crates/`.
  - Add Rust examples to the same root workspace.
- Commit `Cargo.lock`.
- Use Rust from `mise.toml`.
  - Follow the [mise Rust backend docs](https://mise.jdx.dev/lang/rust.html).
  - The mise Rust backend uses rustup. The default Rust profile is fine because
    it includes `rustfmt` and `clippy`; specify components only if the profile
    changes.
- Use pixi to provide `libclang` for `bindgen`, similar to the Zig setup.
  - Prefer running Cargo through mise, not inside pixi.
  - Point `bindgen` at pixi's `libclang` with task environment such as
    `LIBCLANG_PATH` when needed.
- Add Rust formatting and linting early through the repository-wide check/fix
  flow.
  - Formatting should route through `dprint.jsonc`, using the
    [dprint exec plugin](https://github.com/dprint/dprint-plugin-exec/blob/main/README.md)
    to call `rustfmt` if needed.
  - Linting should integrate with `hk.pkl`, so `mise run check` and
    `mise run fix` remain the contributor entrypoints.
  - Use Rust tools such as `rustfmt` and `clippy` from the mise-managed Rust
    toolchain.
- Add CI matrix entries early through `.github/config/variants.toml`.
  - Add the Rust binding entry as soon as it is buildable and testable.
  - Add the Rust Vulkan map example entry when the example builds.
- Use Rust RAII for native handles.
  - Make thread-affine handles `!Send + !Sync`, so safe Rust keeps creation,
    use, and `Drop` on the owner thread.
  - Let `Drop` destroy still-live handles.
  - Keep `Drop` non-panicking.
  - Provide explicit consuming `close(self) -> Result<()>` so callers can
    observe cleanup errors.

## Target crate layout

Use this shape unless implementation pressure reveals a better one:

```text
Cargo.toml
Cargo.lock

bindings/rust/mise.toml
bindings/rust/crates/maplibre-native-sys/
bindings/rust/crates/maplibre-native-support/
bindings/rust/crates/maplibre-native/

examples/rust-map/
```

Root workspace sketch:

```toml
[workspace]
resolver = "3"
members = [
  "bindings/rust/crates/maplibre-native-sys",
  "bindings/rust/crates/maplibre-native-support",
  "bindings/rust/crates/maplibre-native",
  "examples/rust-map",
]

[workspace.package]
edition = "2024"
license = "BSD-2-Clause"
repository = "https://github.com/maplibre/maplibre-native-ffi"
```

## Milestone 1: workspace, tooling, formatting, linting, and raw sys crate

Create the root Cargo workspace, local Rust build/test mise tasks,
repository-wide formatting/linting integration, and `maplibre-native-sys`.

Requirements:

- Generate from `include/maplibre_native_c.h`.
- Keep generated bindings out of hand-edited source.
- Allowlist public symbols, types, and constants, such as `mln_*` and `MLN_*`.
- Enable layout tests where practical.
- Link to the existing `maplibre-native-c` dynamic library.
- Use the project native-library task output, such as `$MLN_FFI_BUILD_DIR`, to
  find the library during local tests.
- Configure `bindgen` to find pixi-provided `libclang` while Cargo still runs
  through mise.
- Add local mise tasks for Rust build and test.
- Use the default mise Rust profile for `rustfmt` and `clippy` unless the
  project later switches to a minimal profile.
- Add Rust formatting to `dprint.jsonc`, using dprint-plugin-exec to route
  `*.rs` files to `rustfmt`.
- Add Rust lint/check steps to `hk.pkl`, so `mise run check` and `mise run fix`
  remain the lint/fix entrypoints.

Tests:

- Call `mln_c_version()`.
- Call `mln_supported_render_backend_mask()`.
- Verify the generated bindings compile on the supported development platforms.

CI:

- Add a `.github/config/variants.toml` binding entry once this milestone has a
  reliable build/test task.

## Milestone 2: support crate and error model

Create `maplibre-native-support` for reusable glue below the public API.

Implement:

- `ErrorKind`;
- `Error`;
- `Result<T>`;
- `check(status)`;
- immediate diagnostic capture from `mln_thread_last_error_message()` after
  non-OK statuses;
- ABI version validation for current version `0`;
- null out-pointer helpers;
- non-null pointer wrapping helpers;
- UTF-8 and `CString` helpers that reject embedded NUL for null-terminated C
  strings;
- `mln_string_view` materialization helpers;
- small RAII guards for native result, snapshot, and list handles as those APIs
  come online.

Map native statuses to stable Rust error kinds. Preserve raw status values and
copied diagnostics, including unknown future statuses.

Tests:

- Verify invalid native calls produce the expected `ErrorKind`, raw status, and
  diagnostic.
- Verify diagnostics are copied before another C call can replace the
  thread-local diagnostic.
- Verify embedded-NUL rejection for public string inputs.

## Milestone 3: minimal public safe crate

Create `maplibre-native` with a small safe public surface that proves the stack.

Expose:

- `c_version()`;
- supported render backend mask wrapper;
- `network_status()`;
- `set_network_status()`.

Keep raw `sys` pointers out of the safe public API.

Tests:

- Exercise real C calls through the safe crate.
- Verify status and diagnostic conversion through the public API.
- Mirror Java FFM tests where they fit, but use Rust ownership and
  `Result<T, Error>`.

## Milestone 4: handle foundations

Add `RuntimeHandle`, `MapHandle`, and the shared internal handle pattern.

Handle requirements:

- Store native pointers privately, preferably as `NonNull<_>`.
- Use `PhantomData<Rc<()>>` or an equivalent marker to make thread-affine
  handles `!Send + !Sync`.
- Use RAII: `Drop` calls the native destroy function for still-live handles.
- Keep `Drop` non-panicking.
- Add consuming `close(self) -> Result<()>` for explicit, fallible cleanup.
- Preserve parent validity while child handles are live.
- Leave a wrapper live if explicit native destroy fails.
- Make successful close idempotent internally, even though consuming close
  prevents ordinary reuse.

Implement first:

- runtime creation;
- runtime run/poll/event drain primitives available in the C API;
- runtime close/drop;
- map creation;
- map close/drop;
- parent retention from map to runtime.

Tests:

- Runtime create/run/poll/close.
- Map create/close.
- Double-close behavior for explicit close paths where applicable.
- Compile-time assertions that thread-affine handles are `!Send + !Sync`.
- Parent-retention behavior while child handles are live.

## Milestone 5: copied value types and descriptors

Add Rust-owned values and descriptor materializers for the common C concepts.

Implement:

- geometry and coordinate values such as latitude/longitude, projected
  coordinates, screen points, and edge insets;
- camera values and camera option descriptors;
- map creation descriptors;
- field-mask-backed descriptors using `Option<T>` fields or explicit setters;
- closed C enum domains as Rust enums with explicit raw conversions;
- forward-compatible output enums with `Unknown(raw)` variants where the C ABI
  may grow;
- public bit masks with `bitflags` when the mask is user-visible.

Rules:

- Public callers set semantic fields, not ABI `size` or mask fields.
- Materializers fill `size`, masks, temporary strings, and temporary arrays
  internally.
- Borrowed native data becomes owned Rust values before native storage is
  released.

Tests:

- Descriptor defaults and field presence.
- Explicit raw enum conversions.
- Unknown enum/status preservation where applicable.
- Real C calls using descriptors.

## Milestone 6: map operations and projection snapshots

Expand safe map APIs after descriptors are stable.

Implement:

- style URL and style JSON setters;
- camera getters/setters and camera updates exposed by the C API;
- size, scale, and viewport-related map operations;
- map projection creation and destruction;
- `MapProjectionHandle` as a standalone snapshot that does not retain
  `MapHandle` after creation but remains thread-affine.

Tests:

- Style setter success and error propagation.
- Camera and projection round trips through real C calls.
- `MapProjectionHandle` drop/close behavior.
- Compile-time `!Send + !Sync` assertions for projection handles.

## Milestone 7: runtime events and copied outputs

Add runtime event polling and other copied output APIs.

Implement:

- owned `RuntimeEvent` values;
- copied event payloads;
- `RuntimeEventPayload::Unknown` for future payloads;
- source-map identification with copied metadata or Rust-assigned IDs;
- internal RAII guards for native event/result/list handles.

Rules:

- Public events must be independent of the next native poll.
- Release native result/list/snapshot handles after copying, even on copy
  failure.
- Do not expose free-floating borrowed native views.

Tests:

- Polling returns owned values.
- Event payload copying survives subsequent native calls.
- Unknown payloads preserve raw diagnostics where available.

## Milestone 8: JSON, GeoJSON, and style value trees

Add owned Rust value trees for JSON, GeoJSON, and style data once simple
descriptors are proven.

Requirements:

- Preserve integer width.
- Preserve object member order.
- Preserve duplicate keys where the C API requires it.
- Apply Rust-side depth limits before native materialization.
- Materialize native descriptor graphs only for the call boundary.
- Copy native snapshots/results back into independent Rust values.

Tests:

- Round trips for representative JSON/GeoJSON values.
- Duplicate-key and object-order preservation.
- Depth-limit failures.
- Cleanup of partially materialized native graphs on error.

## Milestone 9: resource transform callbacks

Add runtime-scoped resource transform callbacks.

Requirements:

- Store callback state for the native lifetime that can invoke it.
- Require callback state to be `Send + Sync + 'static` when native may call from
  worker, network, logging, or render-related threads.
- Copy request URLs before invoking user code.
- Keep replacement URL storage alive until native consumes it.
- Catch panics with `catch_unwind` and convert them to the C callback's
  documented behavior.
- When replacing a callback, install the new native descriptor before dropping
  old state. If native installation fails, close replacement state and keep
  previous state active.

Tests:

- Callback installation and clearing.
- URL replacement.
- Panic containment.
- Replacement rollback on native installation failure where practical.
- Runtime teardown releases callback state once safe.

## Milestone 10: resource provider callbacks

Add resource provider callbacks and handled request ownership.

Requirements:

- Copy borrowed request data into owned `ResourceRequest` before user code can
  retain it.
- Return pass-through immediately for non-handled requests.
- For pass-through, do not retain, complete, or release the native request
  handle.
- Add `ResourceRequestHandle` as the documented `Send` exception where the C API
  permits completion from any thread.
- Enforce one-shot completion.
- Release the C request handle exactly once on completion, explicit release, or
  drop.
- Catch panics and convert failures to the documented C callback behavior.

Tests:

- Inline completion.
- Deferred completion from another thread where supported.
- Pass-through does not retain or release handled state.
- Double completion fails safely.
- Drop releases an uncompleted handled request exactly once.
- `ResourceRequestHandle` is `Send` and only as `Sync` if explicitly justified.

## Milestone 11: custom geometry source callbacks

Add map/style-scoped custom geometry source callbacks after map and callback
foundations are reliable.

Requirements:

- Store callback state for the map/style scope.
- Track active upcalls.
- Delay state release until in-flight callbacks finish.
- Catch panics and convert failures to the C callback contract.
- Hand work back to the map owner thread before calling thread-affine map APIs
  from callback-driven code.

Tests:

- Callback registration and removal.
- Style replacement or source removal releases state after active calls finish.
- Panic containment.
- Owner-thread constraints are documented and enforced where safe Rust can
  enforce them.

## Milestone 12: render target descriptors and sessions

Add rendering APIs after map lifetime and descriptor patterns are stable.

Implement:

- render target descriptors as Rust value types;
- `NativePointer` as a borrowed opaque address value;
- Vulkan surface or texture attachment APIs needed by the Rust map example;
- other surface attachment APIs represented by the C ABI when they are needed
  for parity;
- session-owned texture attachment APIs;
- `RenderSessionHandle` with parent retention to `MapHandle`.

Rules:

- Backend-native handles are borrowed. Passing them transfers no ownership and
  grants no memory access.
- `RenderSessionHandle` represents one attached target for one map.
- Single-session violations surface as native `InvalidState` errors.
- Public safe APIs remain free of raw `sys` pointers.
- Unsafe backend interop accessors must be narrow and must document caller
  obligations.

Tests:

- Attach/detach lifecycle for Vulkan targets.
- Parent retention from render session to map.
- Invalid-state propagation for conflicting sessions.
- `NativePointer` construction and reconversion stay limited to documented
  unsafe APIs.

## Milestone 13: texture readback and frame handles

Add texture readback and session-owned frame APIs.

Implement:

- `read_premultiplied_rgba8_into(&mut [u8]) -> Result<TextureImageInfo>`;
- convenience readback returning an owned image;
- explicit texture frame handle acquisition and release;
- safe copied metadata accessors;
- scoped unsafe `NativePointer` accessors for backend handles.

Rules:

- Safe Rust borrowing should prevent reentrant session calls through the same
  session while a frame handle is live.
- Backend pointers must not outlive the frame handle.
- Frame-derived backend pointers use lifetime-bearing wrappers rather than bare
  `NativePointer` values.
- Frame release happens on explicit consuming close/drop.
- Resize, render update, detach, and session destruction must reject or be
  impossible while a frame is acquired.

Tests:

- Readback into caller-owned buffers.
- Convenience copied image readback.
- Frame pointer lifetime constraints.
- Nested frame acquisition and reentrant session calls fail or are prevented.
- Frame close/drop releases native state exactly once.

## Milestone 14: Vulkan map test app

Add a Rust test app that opens a window and renders a map. This is the practical
parity target for the Rust binding, similar in spirit to the Java LWJGL map
example.

Initial scope:

- Support Vulkan first.
- Use idiomatic Rust crates for windowing and Vulkan integration.
- Evaluate lightweight options before implementing, such as `winit` plus Vulkan
  crates, SDL3 Rust bindings if mature enough, or a small framework that reduces
  boilerplate without hiding the native handles the C ABI needs.
- Keep the app small and focused: create a window, initialize Vulkan objects,
  create runtime/map/render session handles, render a visible map, process
  events, and shut down cleanly.
- Add the example to the root Cargo workspace.
- Add a `.github/config/variants.toml` example entry once it builds reliably in
  CI.

Tests/checks:

- Build the example in CI for Linux Vulkan first, matching available runners and
  native backend support.
- Run locally with a short timeout when a GUI smoke check is useful.
- Keep runtime GUI execution separate from ordinary non-GUI binding tests.

## Validation strategy

Local Rust validation:

```bash
mise run //:ensure-native-library
mise run //bindings/rust:build
mise run //bindings/rust:test
mise run check
mise run fix
```

Repository validation before broader integration or merge:

```bash
mise run test
mise run fix
```

Add tests at the milestone where each invariant appears. Prefer small tests
around real C calls. When C ABI tests already prove native behavior, Rust tests
should prove the Rust adaptation: ownership, copying, diagnostics, status
mapping, callback lifetime, thread-affinity, and safe public API shape.

## CI variant integration

Use `.github/config/variants.toml` for CI matrix integration.

Add entries when the corresponding task is reliable:

```toml
[bindings.rust]
task = "//bindings/rust:test"
requires = { platform = ["linux", "macos"] }

[examples.rust-map]
task = "//examples/rust-map:build"
requires = { platform = ["linux"], backend = ["vulkan"] }
```

Adjust platforms to match actual native library, Vulkan, and runner support.
Keep CI build checks separate from GUI runtime smoke checks unless the runner
can support the GUI path reliably.

## Risks and guardrails

- `bindgen` needs `libclang`; provide it through pixi and point the Rust task to
  it.
- The C headers may require a recent Clang because they use modern C syntax.
- Direct dynamic linkage uses platform loader behavior. That is the approved
  loading path.
- `Drop` cannot return errors. Use explicit `close(self) -> Result<()>` when
  callers need cleanup errors.
- Unsafe Rust can violate thread and lifetime invariants. Keep unsafe blocks
  small and document their invariants.
- Raw pointer wrappers can accidentally become `Send` or `Sync`. Add
  compile-time trait assertions.
- Callback state and render frame lifetimes are high-risk. Implement them only
  after handle and status behavior is tested.
- Resource provider pass-through has a strict ownership rule: do not retain,
  complete, or release pass-through requests.
- Future C ABI growth should preserve unknown raw status, enum, flag, and event
  values where public Rust values expose them.
- The Vulkan test app may require iteration on the Rust window/Vulkan crate
  stack. Choose crates that expose or preserve the native handles required by
  the C ABI.

## Implementation-ready meta-prompt

```text
Goal: Implement the Rust bindings for MapLibre Native FFI up to rough maturity with the existing Java bindings. Build in milestones from root Cargo workspace and sys generation through safe public handles, descriptors, events, callbacks, render sessions, texture frames, CI variant entries, and a Vulkan Rust map test app. Keep each milestone buildable and tested.

Approved decisions:
- Use normal direct Rust FFI linkage, not libloading.
- Use a root Cargo workspace with Rust crates under bindings/rust/crates and the Rust map example in the same workspace.
- Commit Cargo.lock.
- Use Rust from mise.toml. Use pixi-provided libclang for bindgen, but avoid running Cargo inside pixi; configure LIBCLANG_PATH or equivalent env in Rust mise tasks.
- Add formatting and linting early through the repository-wide flow: route Rust formatting through dprint.jsonc and Rust lint/check steps through hk.pkl, so contributors use mise run check and mise run fix.
- Add .github/config/variants.toml entries as soon as the Rust binding and Rust map example have reliable build/test tasks.
- For Rust-owned handles, design for RAII: thread-affine handles are !Send + !Sync so Drop can call native destroy in safe Rust. Drop must not panic. Also provide consuming close(self) -> Result<()> for fallible explicit cleanup.
- Documentation, API reference generation, publishing metadata, and release packaging are out of scope for this project.

Context/evidence: Follow docs/src/content/docs/development/bindings-rust.md for the three-crate split, direct sys generation, !Send + !Sync handles, status/error rules, materializers, callbacks, and render target boundaries. Follow docs/src/content/docs/development/bindings.md for deterministic release, parent retention, diagnostics capture, callback ownership, data copying, scoped borrows, and testing through the C ABI. Use the Java bindings as the maturity target and local reference for generated raw layer, loader/version check, Status.check, HandleState, out-pointer helpers, callbacks, render sessions, examples, and tests, but translate them to Rust ownership, RAII, lifetimes, and narrow unsafe APIs.

Success criteria: The finished project provides maplibre-native-sys, maplibre-native-support, and maplibre-native; generated sys bindings from include/maplibre_native_c.h; status and diagnostic conversion; safe process-global APIs; thread-affine RAII handles; descriptors and copied values; runtime events; callbacks with panic containment and correct ownership; render sessions and texture frame APIs with scoped unsafe backend interop; CI variant entries; and a Rust Vulkan map test app. Raw sys pointers stay out of the safe public API except for narrow documented unsafe interop through NativePointer-style values.

Hard constraints: Use include/maplibre_native_c.h unless maintainers approve another header. Capture diagnostics immediately on failure. Do not panic on native statuses. Preserve raw statuses and unknown future values. Keep thread-affine handles !Send + !Sync. ResourceRequestHandle is the documented Send exception. Do not unwind through C callbacks. Do not retain, complete, or release pass-through resource provider requests. Keep borrowed backend handles scoped and non-owning.

Suggested approach: Build in milestones: workspace/sys/tooling, support errors and diagnostics, minimal safe API, handle foundations, descriptors and values, map/projection APIs, runtime events, JSON/GeoJSON, resource transform callbacks, resource providers, custom geometry callbacks, render sessions, texture frames, Vulkan map example, and CI variant integration as tasks become reliable. Add tests at each milestone before broadening scope.

Validation: Run mise run //:ensure-native-library, the Rust build/test tasks, and the repository-wide mise run check / mise run fix flow during development. Add variants.toml entries when tasks are reliable. Before final merge of integrated work, run mise run test and mise run fix.
```
