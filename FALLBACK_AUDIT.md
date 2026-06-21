# Unnecessary fallbacks and guards — triage report

Audit of first-party code only (`third_party/` excluded). About 50 unique
findings, deduplicated across six parallel subagent reports. Goal: flag the
"codex anti-patterns" — silent fallbacks, redundant guards, and over-permissive
defaulting — where a clean assumption or a hard failure is more correct per the
project's stated conventions (see `AGENTS.md`: prefer clean assumptions or clean
failure over silent fallbacks; the environment is defined by `mise`).

Headline: the anti-patterns concentrate in three places:

1. Enum converters in `src/` with silent `default:` returns after exhaustive
   switches.
2. Pervasive redundant re-validation of value domains in the Java bindings.
3. Soft `:-` defaults for env vars that `mise.toml` marks `required = true`.

## Status

- **Tier 1 — addressed in the PR that introduced this file.** All Tier 1 items
  were verified claim-by-claim against the current tree and either resolved or
  explicitly deferred. Notes on deferrals:
  - **Java bindings (F1–F5, J1–J7) and Kotlin-native (K1):** the audit was
    performed against an in-flight refactor tree that has since been reverted.
    Only build artifacts remain under `bindings/java-ffm/`,
    `bindings/java-jni/`, and `bindings/kotlin/`; the corresponding source files
    are not present on `main`. These findings will need to be re-audited if/when
    those bindings land.
  - **`src/resources/custom_resource_provider.cpp:344` try/catch (F13):** the
    subagent flagged this as wrapping a `noexcept` function, but the outer try
    also wraps construction of `CustomProviderInvocation{...}` which copies a
    `mbgl::Resource` and can therefore throw `bad_alloc`. Claim rejected; the
    try/catch stays.
  - **`src/map/map.cpp:435/440/447` tile scheme ternaries (F4):** idiomatic
    binary-enum converters gated by an upstream validator. Left as-is.
- **Kotlin bindings:** the audit's Java-ffm/java-jni findings (F1–F5, J1–J7,
  plus `closeQuietly`) were re-checked against the new
  `bindings/kotlin/src/{jvmMain,androidMain}` sources after the Java ports were
  moved to Kotlin. The redundant value-domain validation (dimension, scale,
  cache-size, tile-size) did _not_ carry over — the Kotlin sources only have
  binding-owned safety checks (`NativeBuffer` allocation size, offline-operation
  id nonzero). The `closeQuietly` pattern is present in three files
  (`MapHandle.kt`, `RuntimeHandle.kt`, `LogCallbackState.kt`, mirrored across
  jvmMain and androidMain). All current usages are in legitimate cleanup paths
  (replaced-state teardown after the C-level swap already succeeded,
  failure-cleanup inside a `catch` that's already propagating, bulk child close
  on parent close). Tightening them to report rather than swallow requires a
  logging-design decision (logger, level, surface) and is deferred.

## Still pending (Tier 2/3 items not yet resolved)

- `src/map/map.cpp:2765-2794` (5 helpers) and
  `src/render/render_session_common.cpp:901/969/1176/1240` — double null-checks
  on `map_native` after `validate_map`/`validate_live_attached_render_session`.
  Hot render path; removing both layers needs a careful call-site trace.
- `src/map/map.cpp:155` — `to_c_source_type` returns `UNKNOWN` after exhaustive
  switch. Defensible as a forward-compat catch-all; could go either way.
- `src/render/vulkan/vulkan_texture_backend.cpp:362,428` — silent `{}` return on
  Vulkan alloc/map failure loses the diagnostic. Throw instead (readback path
  will convert) — needs verification of the catch path.
- `.github/actions/setup-ci-deps/action.yml:183` — `MISE_ENV:-$(mise exec ...)`
  fallback silently produces malformed sccache keys for non-build jobs. Needs
  confirmation that sccache isn't intentionally best-effort there.
- `examples/{lwjgl-map,dotnet-map}` VulkanContext — Wayland warn-and-return
  contradicts the "targets Wayland" comment. Behavior change for examples.
- Kotlin `closeQuietly` bulk-close paths (see Java/Kotlin notes above). The
  failure-cleanup `addSuppressed` hazard is resolved; the bulk-close paths still
  swallow silently, which is intentional (one bad child shouldn't abort sibling
  teardown).

## Tier 1 — high-value, low-risk

### Build / scripts

- `.mise/tasks/sync-submodules:5` —
  `repo_root="${MLN_FFI_REPO_ROOT:-$(git rev-parse ...)}"`; mise always sets it
  (`mise.toml:77`). Use `:?` fail-fast.
- `bindings/dotnet/tests/Maplibre.Native.Tests.csproj:2` and
  `examples/dotnet-map/...csproj:2` — `Condition="Exists(...)"` silently skips
  the props import that `:ensure-native-library` guarantees. Drop the
  `Condition`.
- `scripts/run-with-timeout.ps1` — entire 145-line file is an orphaned HACK with
  zero callers in the repo. Delete.
- `bindings/dotnet/scripts/generate-clangsharp.sh:31-39` — `if command -v clang`
  plus a hardcoded `/Library/Developer/CommandLineTools/.../clang/21` fallback.
  mise pins `conda:clang-tools`. Drop both.
- `cmake/mln_options.cmake:18-27` — per-OS default for `MLN_FFI_RENDER_BACKEND`
  that mise declares required. Delete the default block; keep the
  unsupported-backend `FATAL_ERROR`.
- `cmake/mln_target.cmake:86-99` and `cmake/mln_artifact_metadata.cmake:23-30` —
  `DEFINED ENV{MLN_FFI_DEPENDENCY_*}` guards for env vars mise unconditionally
  sets. Inline the append.

### C/C++ (`src/`)

- `src/map/map.cpp:1296, 1002, 2088-2186 (8 sites), 435/440/447` — silent
  `default:` / trailing `return` after exhaustive switches in
  `to_native_map_mode`, `to_c_render_mode`, viewport/lod/tile-scheme converters.
  Replace with `assert(false)` or `__builtin_unreachable`. _(Verified note:
  `to_native_*` over `uint32_t` still require `default:` to satisfy clang-tidy
  `bugprone-switch-missing-default_case`; the fix is
  `default: assert(false); return X;`.)_
- `src/runtime/runtime.cpp:281, 319` — same pattern for
  `OfflineRegionDownloadState` and `Response::Error::Reason`.
- `src/runtime/runtime.cpp:857` — `database_source_for_runtime` null-checks a
  runtime every caller has already validated. Drop.
- `src/resources/resource_loader.cpp:71` and
  `src/resources/custom_resource_provider.cpp:344` — `try/catch(...)` around
  functions declared `noexcept`. Dead. _(F13 rejected — see Status.)_

### Bindings

- `bindings/swift/Sources/MaplibreNative/Runtime.swift:245` —
  `ResourceErrorReason(rawValue:) ?? .other` violates binding-spec §Type Mapping
  and is the only Swift enum lacking `unknown(UInt32)`. Add the case (BND-062
  already covers other enums).
- `bindings/rust/crates/maplibre-native-core/src/options.rs:47` —
  `MapMode::from_raw(...).unwrap_or(Continuous)`. Every other Rust enum
  preserves `Unknown(u32)`; this is the asymmetric outlier.
- `bindings/dotnet/src/Maplibre.Native/Internal/Struct/RenderStructs.cs:13` —
  silently swaps `default` extent for `256x256@1.0`. No such contract exists.
  Remove.
- ~16 redundant native re-validation sites across `java-ffm`, `java-jni`,
  `dotnet`, `kotlin-native` (dimension/scale/cache-size/tile-size checks the C
  API already rejects with `MLN_STATUS_INVALID_ARGUMENT`). Java bindings are the
  worst offender. See bindings report J1–J7, F1–F5, D2–D4, K1.

## Tier 2 — medium confidence or medium risk (review before removing)

- `cmake/mln_rust.cmake:4-10`, `bindings/zig/build.zig:147`,
  `bindings/zig/build.zig:486-491` and `examples/zig-map/build.zig:32-39` —
  defensive `DEFINED ENV` / `len != 0` checks for values mise always sets.
- `examples/rust-map/mise.toml` — `:-unset` defaults for required
  `MLN_FFI_RENDER_BACKEND`.
- `bindings/swift/mise.toml:5-8` — `MLN_FFI_SYSTEM_ROOT` fallback re-runs
  `xcrun` that the iOS mise config already set.
- `src/map/map.cpp:2765-2794` (5 helpers) and
  `src/render/render_session_common.cpp:901/969/1176/1240` — double null-checks
  on `map_native` after `validate_map`/`validate_live_attached_render_session`.
  Removing both layers is a hot-path simplification but worth a careful diff.
- `src/runtime/runtime.cpp:2479`, `src/map/map.cpp:4614` — redundant guards on
  validated handles / re-checking `removeLayer` after `getLayer`.
- `src/map/map.cpp:155` — `to_c_source_type` returns `UNKNOWN` after exhaustive
  switch (medium — defensible as a forward-compat catch-all).
- `src/render/vulkan/vulkan_texture_backend.cpp:362,428` — silent `{}` return on
  Vulkan alloc/map failure loses the diagnostic. Throw instead (readback path
  will convert).
- `.github/actions/setup-ci-deps/action.yml:183` — `MISE_ENV:-$(mise exec ...)`
  fallback silently produces malformed sccache keys for non-build jobs. Gate the
  step instead.
- `.mise/bin/windows-msvc-path.sh:20-21,101` — `command -v cmd.exe`/`cygpath` in
  a Git-Bash-only context.
- `examples/{lwjgl-map,dotnet-map}` VulkanContext — Wayland warn-and-return
  contradicts the "targets Wayland" comment. Throw.
- `examples/rust-map/src/vulkan.rs:323-331` and
  `examples/rust-map/src/opengl/platform_windows.rs:53-63` — redundant null
  guards in `Drop` for handles guaranteed non-null on constructed values.
- `bindings/java-ffm`, `bindings/java-jni` `closeQuietly(...)` catching broad
  `Exception` after callback teardown (F6/F7/F8, J8) — narrows the spec's
  "report native release failures" rule. Narrow the catch, don't broaden.

## Tier 3 — low confidence / stylistic (skip or batch)

- `scripts/run-ios-simulator-test.sh:26` (`|| true` on `simctl boot` —
  bootstatus surfaces real failures anyway).
- `examples/rust-map/Cargo.toml:10` — `default = ["vulkan"]` feature (only fires
  outside mise).
- `examples/{dotnet-map,lwjgl-map,swift-map,zig-map}` — scale-factor `?? 1.0` /
  `isFinite` fallback cascades. GLFW/SDL always return valid positive values on
  live windows; defensible belt-and-suspenders, but matches the codex pattern.
- `bindings/dotnet/.../ResourceTypes.cs:131`, `Offline/OfflineTypes.cs:42`,
  `Runtime/RuntimeTypes.cs:147` — null `byte[]` silently coerced to empty array.
- `.github/actions/setup-ci-deps/action.yml:195-207` — defensive `rust`
  reinstall papers over a mise-action cache bug; track upstream rather than
  remove.

## Clean areas (no findings)

- **Docs build** (`docs/`, `vite.config.ts`, `pnpm-workspace.yaml`,
  `docs-pages.yml`) — unusually disciplined, zero findings.
- **Public C headers** (`include/`) — required input validation correctly
  preserved.
- **Zig binding** — cleanest binding; proper `status.Error!T` propagation, no
  silent fallbacks.
- `.devcontainer/`,
  `.github/workflows/{ci,devcontainer-image,docs-pages,mirror-codeberg}.yml`,
  `ci/variants.toml`, `ci/subprojects/*` — declarative, no anti-patterns.
- `cmake/platform/*`, `cmake/render/*` — legitimate platform conditionals only.

## Category totals

| category                                  | count |
| ----------------------------------------- | ----- |
| silent fallback value                     | 14    |
| redundant native re-validation (bindings) | 16    |
| redundant null check / guard              | 11    |
| tool fallback                             | 6     |
| swallowed error / failure                 | 7     |
| dead branch / file                        | 4     |
| manual reimplementation                   | 1     |
| **total unique findings**                 | ~50   |

## Overlap notes

Three findings were independently reported by two agents (consistent reasoning,
not separate triage items): the dotnet `.csproj` `Condition` (build + examples),
`examples/swift-map/mise.toml` `:-metal` default (build + examples), and
`bindings/dotnet/scripts/generate-clangsharp.sh` (build + scripts).
