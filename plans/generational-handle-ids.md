# Generational handle ids for the C API (issue #417)

## Context

`validate_map_live_locked` looks a handle up in `map_registry()`, an
`unordered_map<mln_map*, ...>` keyed on the pointer value
(`src/map/map.cpp:87`). Maps come from `std::make_unique`, so the allocator can
hand the same address to a later map. A caller holding a stale `mln_map*`
therefore either gets a correct rejection or silently binds to a different map.
Attach is the reachable case: a render session attaches from a thread that does
not own the map, so the map can be closed and recreated concurrently.

A bare pointer handle cannot be identity-validated — the caller has no second
value to prove which object it means. So today every binding invents its own
mitigation, and they disagree:

- Zig maintains eight parallel slot tables with index+generation packing.
- Rust (`MapAddress`) and Swift (`MapAttachRef.withLiveMap`) hold a lock across
  the native attach.
- Go narrows the window with borrow counting.
- **Kotlin and .NET release their lock before the native call — a genuine
  residual race, not just untidiness.**
- Dart carries a bare address across isolates with no liveness state at all.

Two handle types are worse than the issue describes: `mln_wake_source`
(`src/runtime/runtime.cpp:63`) and `mln_resource_request_handle`
(`src/resources/custom_resource_provider.cpp:25`) have **no registry at all**
and only null-check. Both are documented any-thread. A stale handle there is a
dereference of freed memory.

Widening the handle to a generational id fixes all of it in one place, makes the
bug deterministically testable for the first time, and lets every binding delete
its bespoke mitigation.

**Outcome:** all 13 public opaque handles become `typedef uint64_t`, validated
against a per-kind slot table with an embedded generation and kind tag. Stale,
wrong-kind, and never-existed handles report `MLN_STATUS_INVALID_ARGUMENT` with
a distinguishing diagnostic instead of misbinding or dereferencing freed memory.

Breaking C ABI change, which the prerelease policy permits
(`docs/.../c-conventions.md:26-28`). Full cleanup is in scope.

## Decisions already made

| Decision                | Value                                               | Why                                                                                                                                                                                  |
| ----------------------- | --------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| C spelling              | `typedef uint64_t mln_map;`                         | Only shape that is boring in all ten generators. A struct forces `CValue<T>` in Kotlin/Native cinterop; a C23 enum forces a closed `CEnum` whose `byValue()` throws on real handles. |
| Null sentinel           | `#define MLN_HANDLE_NULL ((uint64_t)0)` in `base.h` | Live handles carry a nonzero kind, so 0 can never collide.                                                                                                                           |
| Stale/wrong-kind status | `MLN_STATUS_INVALID_ARGUMENT`                       | No new enum value; the thread-local message distinguishes the five cases.                                                                                                            |
| Type safety             | Runtime kind tag + per-binding nominal types        | C loses compile-time distinction; bindings restore it, two of them in the generator.                                                                                                 |
| Adapter tokens          | Deleted; `_wait` promoted to the public request API | The handle id _is_ the token.                                                                                                                                                        |

## Bit layout

```
bits 63..56  kind        (8)   1..255, 0 never appears in a live handle
bits 55..36  index       (20)  1,048,575 live handles per kind
bits 35..0   generation  (36)  ~6.9e10 reuses of one slot
```

The free list is LIFO, so a tight create/destroy loop pounds one slot — 36 bits
is the safety budget that makes that unreachable (32 bits would not be). On
generation exhaustion the slot is **retired permanently**, giving an absolute
never-reused guarantee rather than a probabilistic one. Index exhaustion throws
`HandleTableExhausted`, which `status_boundary` already turns into
`MLN_STATUS_NATIVE_ERROR`.

## Native design

### New: `src/handles/handle_table.{hpp,cpp}`

Register in the explicit source list in `cmake/mln_c_api.cmake`. The `.cpp`
holds the type-independent kind-name table and diagnostic classifier so message
strings are not instantiated 13 times.

```cpp
namespace mln::core {

enum class HandleKind : std::uint8_t { Runtime = 1, Map = 2, /* ...12 total */ };

template <typename Object> struct HandleTraits;  // specialized once per type

template <typename Object>
class HandleTable {
 public:
  auto mutex() const -> std::mutex&;              // for check-and-act callers

  auto insert(std::shared_ptr<Object>) -> std::uint64_t;
  auto insert_locked(std::shared_ptr<Object>) -> std::uint64_t;

  auto resolve(std::uint64_t) const -> Object*;        // borrows; sets diagnostic
  auto resolve_locked(std::uint64_t) const -> Object*;
  auto try_resolve(std::uint64_t) const noexcept -> Object*;  // no diagnostic

  auto lease(std::uint64_t) const
    -> std::shared_ptr<Object> requires(HandleTraits<Object>::leasable);

  auto remove(std::uint64_t) -> std::shared_ptr<Object>;
  auto remove_locked(std::uint64_t) -> std::shared_ptr<Object>;

 private:
  struct Slot { std::uint64_t generation = 1; std::shared_ptr<Object> object; };
  mutable std::mutex mutex_;
  std::vector<Slot> slots_;
  std::vector<std::uint32_t> free_indices_;
};

template <typename Object> auto handle_table() -> HandleTable<Object>&;
}
```

Slots hold `shared_ptr` because the two genuinely any-thread kinds need strong
references handed out. `leasable` is a **compile-time trait**, true only for
`WakeSourceObject` and `ResourceRequestObject`. It is false for `RuntimeObject`
specifically so a lease cannot keep the runtime alive past `destroy_runtime`'s
reset and run the run-loop join on a MapLibre worker thread — a `requires`
clause is cheaper than a comment. Use explicit specializations, not macros
(`cppcoreguidelines-macro-usage` is enabled with `WarningsAsErrors: "*"`).

**Invariant to state in the header and re-check at each step:** no entry point
holds two handle-table mutexes at once. This is true today (`destroy_map` calls
`release_runtime_map` outside the map lock) and per-type tables preserve it only
if it stays true.

**Diagnostics** — five distinguishable messages, all returning
`INVALID_ARGUMENT`: null; not a valid handle; wrong kind (names both kinds);
never created by this process; stale (the object it named was destroyed). The
last row is what closes #417, and all five beat today's uniform
`"map is not a live handle"`.

### Resolve discipline

`resolve()` borrows a pointer that outlives the table lock — the **same** window
`validate_map()` has today, and correct for exactly the same reason: only the
owner thread can retire the handle. So:

- **Plain `resolve()`**: the ~96 `validate_map` sites, ~30 `validate_runtime`,
  `validate_map_projection`, `validate_render_session` and its texture variants.
- **Lock held across the operation** (`scoped_lock(table.mutex())` +
  `*_locked`): `destroy_map`, `destroy_runtime`,
  `retain_runtime_map`/`release_runtime_map`, `map_attach_render_target_session`
  / `map_detach_render_target_session` (already correct — preserve verbatim),
  `map_post_set_size`, `map_post_trigger_repaint`, `erase_render_session`, the
  platform-context leases, and every value-snapshot reader that already locks
  across its read.
- **`try_resolve()` / `try_lease()`** wherever code is reachable from a MapLibre
  worker thread or a deferred callback, so it does not clobber the thread-local
  diagnostics of an unrelated entry point on the same stack:
  `finish_still_image_request`, the three `*_for_platform_context` lookups,
  `invoke_resource_transform`.

No lease/guard RAII type. It would just be `scoped_lock` + `resolve_locked`, and
the two places wanting a strong reference across an unlock are the two
`leasable` kinds where `shared_ptr` already is the guard.

### Call-site shape

Parameter takes the `_handle` suffix; the resolved object keeps the old name, so
function bodies are untouched — two changed lines per call site.

```cpp
// src/map/map.cpp
auto validate_map(mln_map handle, MapObject*& out_map) -> mln_status {
  const std::scoped_lock lock(handle_table<MapObject>().mutex());
  return validate_map_locked(handle, out_map);
}

auto map_request_still_image(mln_map map_handle) -> mln_status {
  MapObject* map = nullptr;
  const auto status = validate_map(map_handle, map);
  if (status != MLN_STATUS_OK) { return status; }
  // ...body unchanged...
  map->map->renderStill([map_handle](std::exception_ptr error) -> void {
    finish_still_image_request(map_handle, error);   // was: capture `map`
  });
}
```

Out-parameter rather than `std::expected` because `cmake/mln_c_api.cmake:60`
sets `CXX_STANDARD 20`, and it gives the smaller diff anyway.

### Struct renaming

`mln_<name>` → `mln::core::<PascalName>Object` for all 12 C++ structs
(`MapObject`, `RuntimeObject`, `RenderSessionObject`, …). The `Object` suffix
avoids `mln::core::Map` sitting beside `mbgl::Map` inside
`struct MapObject { std::unique_ptr<mbgl::Map> map; }`, and avoids colliding
with the existing `RenderSessionKind`/`RenderSessionScheduler`. All move out of
the global namespace into `mln::core`, which they occupied only because they
were the C types.

**Do not script this as a prep commit.** A `sed` of `\bmln_map\b` cannot tell
the C parameter type from the C++ struct type — they are the same token until
the typedef changes, and it would silently rewrite public signatures. Change the
typedef and the struct name together and let the compiler enumerate the sites:
once `mln_map` is `uint64_t`, every surviving `mln_map*` that meant "pointer to
object" is a hard error.

### Hard spots

**`platformContext` round-trip.** `runtime.cpp:2683`
`options.withPlatformContext(runtime)` stashes the pointer in mbgl's
`ResourceOptions` (a `void*`), consumed much later from arbitrary threads via
`resource_loader.cpp` → `lease_resource_transform_state` (`runtime.cpp:115`).
Store the id instead:
`reinterpret_cast<void*>(static_cast<std::uintptr_t>(runtime_handle))`, guarded
by `static_assert(sizeof(void*) >= sizeof(std::uint64_t))` — every preset in
`CMakePresets.json` is 64-bit, and the assert is the tripwire if that changes.
Resolve with `try_resolve` under the lock and copy the member out; never lease
the runtime. Side benefit: mbgl caches file sources against `ResourceOptions`,
so a recycled runtime address could previously alias a dead runtime's cached
file source. A never-reused id cannot.

**Deferred captures.** Rule: anything stored beyond the current entry point
stores an id and re-resolves. Two deliberate exceptions, justified by lock
ordering rather than cost: `request->onCancel`
(`custom_resource_provider.cpp:350`) captures the `shared_ptr` directly, because
a table-mutex acquisition on a MapLibre thread holding mbgl's own locks would
add a new lock-ordering edge; and `HeadlessObserver`/`HeadlessFrontend` keep raw
`RuntimeObject*`/`MapObject*` since their lifetimes are strictly dominated.

**`mln_runtime_event.source`** (`runtime.h:419-428`) becomes `uint64_t source`,
documented as "the `mln_map` for map-originated events, the `mln_runtime` for
runtime-originated events; `source_type` selects the meaning". Every binding's
cast at this field disappears — an ergonomic win falling out of all handles
sharing one C type. Internally `QueuedRuntimeEvent::source` and `::map`
(`runtime.hpp:117-118`) **merge into one field**; the coalescing compare at
`runtime.cpp:2831` and the erase predicate at `:2893` become integer compares,
so a recycled map can no longer coalesce against a dead map's events.
`RuntimeObject::event_maps` and `::map_loading_failures` rekey to `mln_map`.

### Deletions this unlocks

- `ResourceRequestObject`'s hand-rolled refcount: `std::atomic_size_t refs{2}`,
  `release()`, `provider_callback_in_flight`, `provider_release_deferred`, and
  the deferral branch in `release_resource_request`. Three `shared_ptr` copies
  (table slot, `onCancel` lambda, in-flight invocation) replace a manual count,
  two bools, and a protocol. The per-handle mutex stays — it guards
  `cancelled`/`completed`, not lifetime.
- The six `find_*_locked` helpers (`find_style_id_list_locked`,
  `find_offline_region_snapshot_locked`, `find_offline_region_list_locked`,
  `find_feature_query_result_locked`, `find_feature_extension_result_locked`)
  and the inline lookup in `geojson.cpp:843` all collapse into `resolve_locked`.
- The `"handle already exists"` `std::logic_error` guards at `runtime.cpp:607`
  and `:627` — they exist only because pointer keys can collide.
- `const T*` vs `T*` registry-key duplication, since a handle carries no
  constness.

### Adapter cleanup

`src/c_api/callback_adapter.cpp:68-72` maintains a mutex, condvar,
`unordered_map<uint64_t, mln_resource_request_handle*>`, and a counter purely to
give pure-FFI hosts a stable integer for a pointer. The handle now **is** that
integer.

- **Delete**
  `mln_adapter_resource_request_token_{create,cancelled,complete,release}` and
  the whole token table. Callers use
  `mln_resource_request_{cancelled,
  complete,release}` directly.
- **Promote** `..._token_wait` to
  `mln_resource_request_wait_until_retired(mln_resource_request_handle)` in
  `runtime.h`. Re-assessed: the _behavior_ is justified — blocking teardown
  drain that only native can provide synchronously — but it belonged on the
  request handle, not on an adapter token. Implement on
  `ResourceRequestObject`'s existing mutex plus a condition variable.
- **Fix** `mln_adapter_handle_leak_token_create(const char*, void* handle)`,
  which currently _discards_ its handle argument (`callback_adapter.cpp:205`).
  Take `uint64_t` and report it — a leak message naming a stable id beats one
  naming a recycled address.

## Binding design

### Restore nominal typing — the highest-value piece

Every generated raw layer will spell all 12 handles as the same integer, so all
static type safety evaporates in eight bindings simultaneously. The kind tag
makes that a runtime `INVALID_ARGUMENT` rather than corruption, but a binding
that only find-and-replaces ships an API where `runtime.pump(mapId)` compiles.

Two bindings restore it **in the generator**, which is far cheaper than wrapping
hundreds of call sites:

- **rust**: add `.new_type_alias("^mln_(runtime|map|...)$")` to
  `bindings/rust/crates/maplibre-native-sys/build.rs:33-47`. bindgen emits
  `#[repr(transparent)] pub struct mln_map(pub u64);`, so
  `NativeHandleState<T>`, `native_guard!`, and `PhantomData<fn() -> T>` all keep
  working. Transitively saves python.
- **dotnet**: extend `scripts/generate-clangsharp.rsp` with
  `--remap mln_map=MlnMap` targeting hand-written
  `readonly struct MlnMap { public readonly ulong Value; }`. Keeps
  `NativeHandleState<T> where T : unmanaged` and 232 call sites type-checked.
  Verify remap ordering against the existing `uint64_t=ulong`.

The other six wrap in the internal support layer (Zig gets `enum(u64)` for free;
Go gets defined types over `~uint64` at zero cost; Swift/Kotlin/Dart need
explicit wrapper types — Swift most urgently, since `NativeMap.create` and
`metalSurfaceAttach` would otherwise both take a bare `UInt64`).

### Cross-cutting rules

1. Every null check becomes `id == 0`. Audit each one; a missed one turns a
   failed create into a live-looking handle 0.
2. Close-once, leak reporting, parent retention, and owner-thread/isolate checks
   stay binding-owned. What goes is _identity proof_.
3. Delete every "hold a lock across the native call so the address cannot be
   reused" mechanism.
4. Kotlin's and .NET's lock-released-before-the-call becomes sound. Delete the
   comment debt; do **not** add a lock.
5. Bindings must not reimplement generation/kind/staleness validation
   (`CLAUDE.md`: "Bindings do not reimplement native validation").
6. Leak reports print the id, not an address.

### Per-binding

| Binding    | Deleted                                                                                                                                                                             | Reshaped                                                                                                                                                                                                                                                                                                                                        |
| ---------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **rust**   | All of `MapAddress` (`map.rs:52-105`) + its two `unsafe impl`s; `map_ids`/`next_map_id` (`runtime.rs:35-36`); `unsafe impl Send/Sync for WakeSource`                                | `NativeHandleState.address: Cell<Option<usize>>` → `id: Cell<Option<NonZeroU64>>`; `MapId` _becomes_ the native id; `MapAttachRef { id }`. Keep `WakeSource` non-`Copy` so it cannot double-destroy.                                                                                                                                            |
| **swift**  | `MapAttachRef.withLiveMap`; duplicate `nativeAddress` field (`Map.swift:62`); the `@unchecked Sendable` rationale on `NativeHandleBox`                                              | ~203 `OpaquePointer` → id wrappers; `registry` rekeyed; **public `RuntimeEventSource.map(NativePointer)` → `MapId`** (see below)                                                                                                                                                                                                                |
| **zig**    | Five slot tables + free lists + spin locks (`map.zig:36-44,1723-1812`, `projection.zig`, `render.zig`, `runtime.zig:56-115`); the two O(n) pointer scans at `runtime.zig:1640-1663` | Handles become `enum(u64)` over the native id; per-handle state moves to `AutoHashMapUnmanaged(u64, *State)`. **Keep** the texture-frame tables and `nextHandleGeneration` — those are binding-invented kinds C does not have.                                                                                                                  |
| **kotlin** | All six `AddressPointer` classes in androidMain + their 12 call sites                                                                                                               | `HandleStateCore.address: Long` → `handleId: Long` (the whole commonMain change); jvmMain `MemorySegment` → `long`; nativeMain `HandleState<T : CPointed>` loses its type parameter, so add `value class` wrappers there                                                                                                                        |
| **dotnet** | `GeneratedLayoutTests.OpaqueHandlesArePointerSizedAtCallBoundary`                                                                                                                   | `nint address` → `ulong id`; `T* Pointer` → `T Handle`; `Dictionary<nint,…>` → `<ulong,…>`. **Add a `generate-check` drift task** mirroring `bindings/dart/mise.toml:38-49` and wire `:build` to it.                                                                                                                                            |
| **go**     | All four `struct{ _ byte }` phantom types (`native_handles.go`); `nativeAddress`; `nextMapID`; ~150 `(*C.mln_map)(unsafe.Pointer(ptr))` casts in `map.go` alone                     | `State[T any]{ptr *T}` → `State[T ~uint64]{id T}`; `Borrow` **stays** but its doc drops "keeps it live" for "serializes against release"                                                                                                                                                                                                        |
| **dart**   | `ResourceRequestToken` class entirely; `transfer()`; `_checkResourceRequestToken`; `ResourceRequestHandle._ownerIsolateHash`                                                        | `MapAttachRef` becomes `extension type MapAttachRef(int _id)`; token methods move onto `ResourceRequestHandle`, which is now natively cross-isolate. **Keep** `_checkOwnerIsolate` and its diagnostic (#412 explains a Dart-specific hazard a status cannot). **Keep** `_leakToken` a real `Pointer<Void>` — only the _argument_ becomes an id. |
| **python** | The address-round-trip idiom at six sites and its SAFETY comments — a `u64` newtype is `Send`, so `py.detach` closures capture the handle directly                                  | `WakeSource.address` → `id`; `_MapHandle.address()` → `id()`; wire key `source_address` → `source_id`                                                                                                                                                                                                                                           |

### Address-keyed registries (present in all eight)

They do two jobs. _Identity proof_ is deleted everywhere — a stale id never
collides, so a hit is always the right map, and the weak-ref re-validation and
`closed` re-checks go. _Host-object resolution_ survives, because C returns an
id, not a host reference.

The rule: **a binding that exposes an opaque public map identity value deletes
its table and counter (rust, go, zig); a binding that exposes the public map
wrapper keeps the table, rekeyed on the id (swift, dotnet, kotlin, dart,
python).**

### Public APIs leaking a raw address

`NativePointer` means "borrowed backend-native address" and is used for real
Metal/Vulkan/GL handles. Reusing it for map identity conflates a GPU device
pointer with a MapLibre object.

- Swift `Runtime.swift:84-96` → `case map(MapId)` where
  `MapId: Hashable, Sendable` has **no public initializer from `UInt64`**.
- Python `_MapHandle.address()` → private `id()`, plus a public read-only
  `MapHandle.id` mirroring rust `MapId` / go `MapID`.

The distinguishing rule, which must go in the spec, is **constructibility, not
visibility**: an identity value may expose its integer read-only and has no
public constructor from an integer; a handle carries the same integer privately
and can never be built from one in the safe API; `NativePointer` is a third,
disjoint concept. Without this written down, someone adds `MapId(rawValue:)`
"for testing" and reopens handle forgery in every binding at once.

## Documentation

**`c-conventions.md`** — new "Handles" section, plus revisions at the lines that
assume pointers: `:69-71` (null → 0), `:73-76` (`*out_handle` non-null →
nonzero), `:47-52` (disambiguate handle vs. backend `void*`), `:82-88` (the
map-pointer publication paragraph), `:128-129`, `:142-148`, `:172-176`,
`:203-207` (where "generation" already appears for frames and will now collide
with handle generations — distinguish them).

**`binding-specification.md`** — the governing spec. Key changes:

- **Owned handles** (`:128-140`): "private native identity" → "the private
  native handle id"; replace the pointer-or-table-ID paragraph with the
  identity-value rule above.
- **New subsection "Stale and mismatched handles"**: the C API validates every
  id and reports `INVALID_ARGUMENT` with a distinguishing diagnostic; bindings
  surface it through their ordinary invalid-argument error and rely on C for id
  validity, generation, and kind checking.
- **Handle copying** (`:179-184`): append that public handle types keep their
  integer payload private even where the language's ordinary value-class or
  extension-type syntax would publish it.
- **Event polling step 6** (`:512-514`): "never creates a public handle from the
  native source pointer" → copy the source id, resolve any wrapper from
  binding-owned state keyed by it.
- **Transferability** (`:553`) — **a real semantic change, flag it in review**.
  "Both are transferable and MUST NOT be shareable" existed so two threads could
  not race attach through one shared reference. An attach reference is now a
  copied integer that most languages derive as shareable. Replace with wording
  that permits the language's ordinary safe conformance — this is what licenses
  deleting rust's two `unsafe impl`s and Swift's `@unchecked Sendable`.
- **Resource providers** (`:430-434`): a host that moves a handled request
  between execution contexts passes the request id itself, so bindings expose
  one request handle type rather than a separate transferable token.
- **New conformance rows**: BND-045 (stale id replayed after a same-kind handle
  is created), BND-047 (wrong-kind id), BND-049 (a moved id reports
  _wrong-thread_, not stale), BND-196 (attach through a reference to a released
  map). Reword BND-086 and BND-151; fence BND-173 so nobody tries to solve frame
  scope with C validation. Widen the "Cross-thread public handle use" gate —
  every binding can now move an id through an internal seam.

Follow CLAUDE.md's Prose rules (positive for guidance, negative only for real
prohibitions) and Specification rules (standalone, testable, add rather than
restate).

**`docs/snippets/c/*.c`** — 9 real C consumers compiled by hygiene via
`hk.pkl:52-57` (glob covers `include/**/*.h`, so a header change trips it
first). 22 pointer-typed handle declarations; the `= NULL` initializations need
`MLN_HANDLE_NULL`.

**`docs/doxygen/Doxyfile:21`** — `TYPEDEF_HIDES_STRUCT = YES` exists for the
current `typedef struct X X;` idiom and becomes misleading. Revisit.

## Sequencing

One branch, one PR, commits per phase. **Known property to state up front:**
`uint64_t` and `struct T*` are ABI-incompatible, so binding suites are red
between the native commits and their own binding commit. CI is green at the
branch tip. The gate for each native commit is `mise run test`; the gate for
each binding commit is that binding's test task.

**Phase 0 — de-risk the unknown generator.** Confirm JavaCPP maps
`typedef uint64_t mln_map;` to `long` and not to a generated `Pointer` subclass.
It is the only generator whose behavior could not be confirmed statically and it
has the slowest loop (`mise run //bindings/kotlin:androidBuild opengl x86_64`).
If it guesses wrong, `MaplibreNativeCConfig.java` needs
`infoMap.put(new Info("mln_map").cast().valueTypes("long"))` per type.

**Phase 1 — infrastructure.** `handle_table.{hpp,cpp}`, CMake registration,
`MLN_HANDLE_NULL`. Nothing uses either.

**Phase 2 — pilot: `mln_json_snapshot`.** Smallest self-contained type (one
struct, one registry, three entry points). Proves traits, table, resolve,
diagnostics, and the C ABI test end to end. Land `src/c_api/tests/handles_abi.c`
here — note `cmake/mln_tests.cmake:58-83` fatal-errors unless `main.c` gains a
matching `run_handles_abi_tests()`.

**Phase 3 — remaining native types**, in ascending risk: value snapshots
(`mln_style_id_list`, both offline types, both query result types) →
`mln_wake_source` (first `leasable`; deletes raw new/delete) →
`mln_resource_request_handle` (second `leasable`; deletes the refcount) →
`mln_map_projection` (proves the thread-affine shape on two entry points) →
`mln_render_session` (`RenderSessionObject::map` becomes an id, closing the
render-thread → map-owner-thread window) → **`mln_map` and `mln_runtime`
together**, which cannot be split because `mln_runtime_event.source` is a union
of both and `retain_runtime_map`/`resource_options_for_runtime`/`event_maps` all
straddle the boundary.

**Phase 4 — header and adapter finalization.** `runtime.h` event source,
`callback_adapter.h` token removal and
`mln_resource_request_wait_until_retired`, then regenerate and commit the two
checked-in generated layers (`//bindings/dotnet:generate`,
`//bindings/dart:ffigen`).

**Phase 5 — bindings. Zig first, alone.** It already implements the target model
and owns the headless smoke test, so if the C design has a flaw it surfaces
before seven other bindings are half-migrated. Then in parallel: **rust →
python** (hard sequence — python is PyO3 over the same crates), **go**,
**dotnet**, **kotlin** (all three backends move together), **swift**. **Dart
last** — biggest raw reference count, the only adapter consumer, and **not in
CI** (`ci/workflow.toml:47-49`), so the most behavior-changing part of the
migration is the least automatically covered.

**Phase 6 — docs, spec, and conformance annotations.** Cross-check each new row
against tests that actually exist; the `BindingSpecTest("BND-040")` attributes
in dotnet and `// Spec coverage:` comments in rust are the audit trail.

## Verification

**Per native commit**

```bash
mise run build && mise run test          # C ABI suite (Unity, ctest name c-api)
```

New coverage in `src/c_api/tests/handles_abi.c`: handle values never repeat
across a create/destroy loop (the direct #417 regression); stale handle after
destroy; wrong-kind handle; zero handle; forged `0xdeadbeefdeadbeef` rejected
without crashing; `mln_thread_last_error_message()` distinguishes all five.
Extend `runtime_wake_abi.c` (signal a wake source destroyed on another thread)
and `resources_abi.c` (double release, post-release complete) — both are UB
today. `render_thread_abi.c` already exercises cross-thread attach and should
stay green unchanged.

**Per binding commit**

```bash
mise run //bindings/<name>:test
mise run //examples/zig-readback:run          # headless smoke, no display
```

Each binding gains BND-045 (the case that was untestable with pointers: create A
→ capture its id through an internal test seam → close A → create B, which often
lands at A's old address → replay A's id → expect invalid-argument, and B still
works), BND-047, BND-049, and BND-196.

**Binding-specific gates**

```bash
mise run //bindings/dart:ffigen-check     # Dart is not in CI; run locally
mise run //bindings/dart:analyze && mise run //bindings/dart:test
mise run //bindings/kotlin:androidBuild opengl x86_64   # JavaCPP mapping
mise run //bindings/kotlin:jvmTest        # + new C_LONG == JAVA_LONG assertion
```

**Whole-tree**

```bash
mise run fix                    # hygiene; runs //docs:check-snippets on include/**
mise run //docs:build           # Doxygen over include/, rust + zig API refs
mise run //examples/zig-map:run:owned-texture   # GUI; brief timeout or background
```

## Critical files

- `include/maplibre_native_c/base.h` — the 13 typedefs (`:61-69`) and
  `MLN_HANDLE_NULL`; also `query.h:19-20`, `style.h:23`
- `include/maplibre_native_c/runtime.h:419-446` — event source
- `include/maplibre_native_c/callback_adapter.h` — token removal, leak token,
  queued request handle field
- `src/handles/handle_table.{hpp,cpp}` — new
- `src/map/map.cpp` — `MapObject`, the four `validate_map*` helpers, ~126 entry
  points, the still-image capture at `:3397`
- `src/runtime/runtime.cpp` — `RuntimeObject`, `platformContext` (`:2683`), the
  event queue, wake sources, offline tables
- `src/resources/custom_resource_provider.cpp` — `ResourceRequestObject`,
  refcount deletion, `onCancel` capture
- `src/render/render_session_common.{hpp,cpp}` — `RenderSessionObject`, the
  session registry, `validate_*` helpers
- `src/c_api/callback_adapter.cpp` — token table deletion
- `bindings/rust/crates/maplibre-native/src/map.rs` — `MapAddress` deletion
- `bindings/dart/lib/src/runtime/runtime_render_handles.dart` — `MapAttachRef`,
  `ResourceRequestToken`
- `docs/src/content/docs/development/binding-specification.md`,
  `.../c-conventions.md`

## PR

Per `CLAUDE.md` and `AI_POLICY.md`: follow `.github/pull_request_template.md`
with **Summary** and **Test plan** at one sentence each, and fill in the **AI
assistance** section. This plan file is committed on the branch as the planning
doc referenced from **Context**.

---

# EXECUTION LOG (live — update as work lands)

## Committed

- `805a97a3` — **native layer complete**. All 13 handle types are `uint64_t`
  generational ids. Full C ABI suite passes, including 4 new `handles_abi.c`
  tests (`a_released_map_handle_never_names_a_later_map` is the #417
  regression). Phase 4 adapter cleanup included.
- `0ffb6ecb` — C doc snippet null-handle comparison.

## Design deltas from the plan above (adopted during implementation)

- `RuntimeObject` gained a `self` field holding its own handle, so internal
  helpers that hold the object can reach its id without a reverse lookup (needed
  by `resource_options_for_runtime`, `schedule_registered_offline_operation`).
- `MapObject::runtime` is an `mln_runtime` **id**, not a pointer — no stored
  pointer can go stale.
- `HeadlessFrontend` takes `mbgl::util::RunLoop&` at construction rather than
  resolving the runtime handle in `setObserver()`; mbgl calls that from the map
  constructor.
- `mln_render_session`'s C++ object is named `mln_render_session_object` (not
  `RenderSessionObject`) because it stays in the global namespace alongside the
  existing `RenderSessionKind`/`RenderSessionScheduler` in `mln::core`.
- A `live_runtime_threads` set mirrors owner threads, because the handle table
  deliberately has no iteration API and `create_runtime` must reject a thread
  that already owns one.

## Phase 0 finding (carry forward)

JavaCPP needs **no** `Info` entries. `mln_offline_operation_id` is already
`typedef uint64_t` in the same headers and flows through as Java `long` by value
(`OfflineOperationHandle.kt:49`) and `long[]` as an out-param
(`RuntimeHandle.kt:73-82`). Still verify once with
`mise run //bindings/kotlin:androidBuild opengl x86_64`.

## Remaining

1. **Bindings** (none done; all currently broken by construction — `uint64_t`
   and `struct T*` are ABI-incompatible, so the branch tip is not green until
   all eight land):
   - **zig** — IN PROGRESS. `map.zig` registry converted to
     `AutoHashMapUnmanaged(c.mln_map, *MapState)`; slot table, free list,
     `mapHandle`/`mapHandleIndex`/`mapHandleGeneration` deleted; `MapHandle` is
     `enum(c.mln_map)`. STILL TO DO: `runtime.zig` has 4 more slot tables
     (runtime, resource request, wake source, offline operation),
     `projection.zig` 1, `render.zig` 1 for sessions (KEEP the texture-frame
     tables — those are binding-invented kinds the C API has no handle for, so
     `nextHandleGeneration` survives scoped to `render.zig`). Also delete the
     two O(n) pointer scans at `runtime.zig:1645,1659`.
   - rust → python (hard sequence), go, dotnet, kotlin (3 backends together),
     swift, dart last.
   - Regenerate + commit `//bindings/dotnet:generate` and
     `//bindings/dart:ffigen`.
2. **BND-196** conformance row (attach through a reference to a released map)
   and per-binding BND-045/047/049 tests.
3. `concepts.md` / `overview.md` / guides handle wording; Doxyfile
   `TYPEDEF_HIDES_STRUCT` revisit.

## Zig conversion recipe (established on map.zig + projection.zig — repeat for render.zig, runtime.zig)

Per module with a slot table:

1. Delete `const NativeX = opaque {};`, the `XRegistrySlot` struct, the
   `x_free_list`, and the `xHandle`/`xHandleIndex`/`xHandleGeneration` trio.
2. `var x_registry: std.ArrayList(XRegistrySlot)` becomes
   `std.AutoHashMapUnmanaged(c.mln_x, *XState)`. Keep the spin lock — it guards
   the map.
3. Drop `native: ?*NativeX` from `XState`; the handle is the key.
4. `pub const XHandle = enum(u128)` becomes `enum(c.mln_x)`; its value IS the
   native handle, so `@intFromEnum(handle)` is the C argument and
   `@enumFromInt(native)` builds it.
5. `registerXState(state)` takes `(native, state)` and does `put`.
6. `xLease` / `beginXClose` collapse to a single `registry.get(...)`;
   `finishXClose` to `registry.fetchRemove(...)`.
7. `errdefer { if (x) |h| _ = c.mln_x_destroy(h); }` becomes
   `errdefer _ = c.mln_x_destroy(x);` — destroy takes the null handle.

Binding-owned state that STAYS: `closing`, `active_leases` /
`attached_render_sessions`, `diagnostic_store`, custom-geometry source lists.
Those are close-once and borrow-tracking, not identity.

**Regex caution:** the bulk `?*c.mln_X` → `c.mln_X` and `.?`-stripping passes
are efficient but can silently change behavior where an optional unwrap was load
bearing. Re-read each converted module before trusting it; `//bindings/zig:test`
plus `//examples/zig-readback:run` are the real gate.

## Zig: DONE (commit 5908f12c)

Compiles, `//bindings/zig:test` fully passes, and `//examples/zig-readback:run`
renders a real 512x512 Vulkan frame. The `.?`-stripping worry is resolved — the
suite is green.

## Zig status as of last checkpoint (historical)

All six C-backed slot tables are gone (map, projection, render session, runtime,
resource request, wake source). `OfflineOperationHandle` and the
owned-texture-frame tables correctly KEEP their generations — binding-invented
kinds with no C handle. Both O(n) `@intFromPtr` address scans in `runtime.zig`
are now `registration.native == source` equality tests.

Error count trajectory: 147 → 86 → 74 → 83 (regex regression, reverted) → 72 →
46 → (in flight). Remaining errors are all the same shape: a value that used to
be `?*c.mln_X` is now a plain `u64`, so `orelse` / `if (x) |h|` / `@ptrCast`
must become an explicit `== 0` test.

**Lesson worth carrying:** the bulk regex passes cost more than they saved past
the first sweep. Site-by-site fixing moved the count down monotonically; the
scripted passes oscillated and introduced a parameter-shadowing bug. Fix the
remaining sites by reading them.

After it compiles, the real gate is:

```
mise run //bindings/zig:test
mise run //examples/zig-readback:run     # headless smoke
```

and a read-through of the `.?`-stripped sites, where a regex could have removed
a load-bearing optional unwrap without failing to compile.

## Rust: DONE (commit a4d2c6f4)

201 tests pass, clippy clean. Deleted: `MapAddress` + 2 unsafe impls,
`WakeSource`'s 2 unsafe impls, `next_map_id`/`map_ids`, both address-keyed
lookups. `MapAttachRef` derives Send+Sync. Added `NativeHandle` trait and
`OutHandle<T>` in core (python inherits both).

**Open follow-up:** `source_for_event` now resolves through `map_states`, which
only holds maps with a live wrapper, where the deleted `map_ids` held every map
until unregistered. So an event for a map whose Rust wrapper was dropped reports
`UnknownMap` rather than its id. The reworded BND-086 says to expose the copied
source id regardless — so this should likely return `MapId::new(raw.source)`
unconditionally. Not covered by the suite: the existing test uses a
never-registered synthetic id, which exercises a different path.

## Rust: historical notes

`maplibre-native-sys/build.rs` has a `.new_type_alias(...)` for all 12 handle
types. **Verified working** — bindgen emits
`#[repr(transparent)] pub struct
mln_map(pub u64)`, so the compile-time
distinction the opaque struct pointers gave us is preserved at zero ABI cost.
This is the cheap win the plan predicted; do the same via `--remap` for dotnet.

That change alone leaves `maplibre-native-core` with ~20 `E0308` mismatched-type
errors, all in `handle.rs`. The next step:

`NativeHandleState<T>` (`maplibre-native-core/src/handle.rs:67`) stores
`address: Cell<Option<usize>>`. It should store the handle value. Because `T` is
now a bindgen newtype over `u64` with a public `.0`, the tidy shape is a small
trait in core:

```rust
pub trait NativeHandle: Copy {
    fn to_raw(self) -> u64;
    fn from_raw(raw: u64) -> Self;
}
```

implemented per sys newtype with a `macro_rules!` (Rust has no macro ban — the
`cppcoreguidelines-macro-usage` rule is C++ only). Then:

- `address: Cell<Option<usize>>` -> `handle: Cell<Option<NonZeroU64>>`
- `as_ptr()` -> `handle()`, `as_non_null()` deleted, `restore_address_for_retry`
  -> `restore_handle_for_retry`
- `NativeHandleLeak.address: usize` -> `id: u64`; leak reports print the id
- `StatusDestroy<T>`/`InfallibleDestroyFn<T>` lose the `*mut`
- Then **delete** `crates/maplibre-native/src/map.rs:52-105` — the whole
  `MapAddress` type, its `RwLock`, both `unsafe impl`s, `with_live`,
  `is_retired`, `retire_with`, and `MapState.address`. Its doc comment at :52-59
  names this exact bug.
- `runtime.rs:35-36` `map_ids`/`next_map_id` delete; `MapId` becomes the native
  handle. Keep `map_states` (it drives
  `release_detached_custom_geometry_sources`), rekeyed on `MapId`.
- `lib.rs:294` `assert_not_impl_any!(MapAttachRef: Sync)` -> `assert_impl_all!`.

## Phase 5 progress

| Binding | Commit   | Gate                             |
| ------- | -------- | -------------------------------- |
| zig     | 5908f12c | tests + headless Vulkan render   |
| rust    | a4d2c6f4 | 201 tests, clippy clean          |
| python  | 7cb492ca | 131 tests, ty type checker clean |
| go      | 2f4ceaac | all packages, go vet clean       |
| dotnet  | 61ec7f09 | 189 tests, generate-check green  |

Remaining: kotlin (IN PROGRESS), swift, dart (last; not in CI).

### Kotlin design (settled; nativeMain compiles clean)

Verified generator behaviour, all three backends:

- **cinterop (nativeMain)**: every handle becomes `ULong`.
- **jextract (jvmMain)**: `long` / `C_LONG` — confirmed by reading
  `build/generated/sources/jextract/.../MapLibreNativeC.java`.
- **JavaCPP (androidMain)**: expected `long` (Phase 0 finding); not yet built.

Restored nominal typing with **one** set of value classes in **commonMain**
(`internal/lifecycle/NativeHandles.kt`): a
`sealed interface NativeHandle { val
raw: Long; val isNull }` plus 12
`@JvmInline value class Native*` types. All three backends share them, so
`runtime.pump(mapId)` fails to compile everywhere. `import kotlin.jvm.JvmInline`
is required in common code.

nativeMain-only helpers in `internal/lifecycle/NativeHandleValues.kt`:

- `NativeHandle.rawHandleValue: ULong` — the value cinterop wants.
- 12 `xHandle(value: ULong)` builders.
- `ULong.asHandle(name, build)` — reads an out-parameter and rejects the null
  handle.

`HandleState<T : CPointed>` became `HandleState<T : NativeHandle>`;
`HandleStateCore.address` became `handleId`. Handle out-parameters are
`alloc<ULongVar>()` with `.value = 0uL`.

**Struct helpers keep injectable functions typed over `ULong`** (they mirror the
cinterop signature) while their leading handle parameter is the value class; the
body converts with `.rawHandleValue`. Same for `RuntimeHandle.destroyer`,
`ResourceRequestHandle.completer/cancellationChecker/releaser`, and the
`createForTesting` creator.

`ValueStructs.jsonSnapshotHandle` was renamed `readJsonSnapshot` and now takes a
non-null handle, checking `.isNull` itself — a null snapshot means the value is
absent, so `asHandle` (which rejects null) is wrong there.

`liveMaps` is rekeyed on `nativeHandleId()`, and `unregisterMap`'s identity
re-check collapsed to a plain `remove`.

#### Naming collisions hit while doing this (three in one file set)

- `.c` as the extension name collided with the `internal.c` **package path**, so
  `\.c\b` matched a dozen import lines. Renamed to `.cValue`.
- `.cValue` collided with **`kotlinx.cinterop.cValue`**. Renamed to
  `.rawHandleValue`, which is free.
- `jsonSnapshotHandle` existed as both a lifecycle builder and a `ValueStructs`
  member; importing one shadowed the other.

Error trajectory (nativeMain): 310 -> 131 -> 87 -> 69 -> 37 -> 16 -> 0.
nativeTest: 88 -> 23 -> 11 -> 0, via a `SyntheticHandles` object in
**commonTest** (shared by all three backends).

#### jvmMain: the compiler gives NO help here

`NativeAccess` calls native through
`MethodHandle.invokeWithArguments(Object...)`, which is **untyped**. jvmMain
compiled clean against the new headers while being completely wrong — every
handle would have reached native as a boxed `MemorySegment`. Do not trust a
green jvm compile; `jvmTest` is the real gate.

The fix is one conversion point, not 179 call-site edits:

```kotlin
private fun MethodHandle.invokeNative(vararg args: Any?): Any? =
  invokeWithArguments(args.map { if (it is NativeHandle) it.raw else it })
```

then a plain `.invokeWithArguments(` -> `.invokeNative(` rename. Call sites pass
the typed handle and cannot get it wrong.

Other jvmMain changes:

- 18 handle out-parameters go from `ValueLayout.ADDRESS`/`MemorySegment.NULL` to
  `ValueLayout.JAVA_LONG`/`0L`. **`outValue` is NOT one of them** — it holds an
  `mln_json_value*`.
- The function descriptors need no change: they come from jextract, which
  already emits `C_LONG`.
- `ResourceProviderState`'s upcall `MethodType` must change its third parameter
  to `Long::class.javaPrimitiveType`; `findVirtual` resolves at run time, so a
  mismatch throws rather than failing to compile.
- The event `source` field reads with `ValueLayout.JAVA_LONG`.

**The value classes paid for themselves here**: blanket-renaming
`snapshot:
MemorySegment` and `result: MemorySegment` assigned three private
helpers the wrong kind (`offlineRegionSnapshot(snapshot: NativeJsonSnapshot)`,
`offlineRegionList(list: NativeStyleIdList)`,
`featureExtensionResult(result: NativeFeatureQueryResult)`). The type checker
caught all three; with bare integers they would have shipped.

#### androidMain (JavaCPP)

Converted but **not yet verified** —
`//bindings/kotlin:androidBuild opengl
x86_64` rebuilds MapLibre Native for the
ABI from scratch (~532 targets), so it is slow. Changes made:

- The `AddressPointer` `Pointer` subclasses and the `map()`/`runtime()`/
  `projection()`/`renderSession()`/`resourceRequestHandle()` wrappers that
  existed only to turn a Long into a generated Pointer type are deleted. **One
  `AddressPointer` remains in each of `MapHandle.kt` and
  `RenderSessionHandle.kt`** — `descriptor.user_data(AddressPointer(0))` and
  `pointerOrNull` are genuine `void*`/backend pointers.
- Handle out-parameters go from `PointerPointer<...>` to `LongPointer`, and
  `.put(0, null as Pointer?)` to `.put(0, 0L)`. **`outValue` stays a
  `PointerPointer`** — it holds an `mln_json_value*`.
- `requireLiveAddress` -> `requireLiveHandle`, `nativeAddress` ->
  `nativeHandleId`, `handleAddress` -> `handleId`, `sourceAddress` ->
  `sourceId`.

androidMain does **not** use the commonMain value classes; JavaCPP call sites
take the raw `Long` directly. Revisit only if the android surface proves
error-prone.

### Kotlin gates

- `mise run //bindings/kotlin:test` — **PASSING** (jvmTest + linuxX64Test both
  ran and succeeded).
- `mise run //bindings/kotlin:androidBuild opengl x86_64` — pending.

## NATIVE GAP FOUND LATE — OpenGL-only sources were never compiled

`mise run build` uses the **Vulkan** host preset, so
`src/render/opengl/*_session.cpp` was never compiled during Phases 2-4 and never
converted. The Android OpenGL build was the first thing to touch it.

`opengl_texture_session.cpp`'s local `validate_frame` helper still took
`mln_render_session_object*` while its callers passed the `mln_render_session`
id. Fixed by giving it the id and an out `live` pointer, matching
`vulkan_texture_session.cpp`.

**The gate for this class of gap is `mise run build linux-x64-egl`** — it
compiles the OpenGL backend on the host. Run it before trusting the native layer
as complete. Metal sources still have no host gate on Linux; a macOS build would
be needed for those.

## Swift (in progress)

Swift's C importer maps every handle to `UInt64`. Same approach as Kotlin:
`Sources/MaplibreNative/Support/NativeHandles.swift` defines
`protocol NativeHandle: Hashable, Sendable { var raw: UInt64; init(raw:) }` plus
12 structs. `NativeHandleState` and `NativeHandleBox` became generic over
`Handle: NativeHandle`. `NativeHandleLeak.address: UInt` -> `handle: UInt64`.

### Swift toolchain in this sandbox

`mise`'s Swift 6.3.1 needs `libxml2.so.2`; the host has `.so.16` and there is
**no sudo**, so `mise bootstrap` cannot fix it. Working recipe:

```
mise exec ubi:mamba-org/micromamba-releases[exe=micromamba] -- \
  micromamba create -y -p /tmp/swiftdeps/env2 -c conda-forge "libxml2=2.12"
LD_LIBRARY_PATH=/tmp/swiftdeps/env2/lib mise run //bindings/swift:build
```

(conda-forge's _current_ libxml2 is also soname 16 — the `=2.12` pin is what
provides `.so.2`.)

### Swift naming collision

`NativeMap`, `NativeRuntime`, `NativeJSONSnapshot`,
`NativeFeatureExtensionResult`, and `NativeResourceRequest` **already exist** as
the C-shim namespace enums in `Support/`. The handle types therefore take a
`Handle` suffix: `NativeMapHandle`, `NativeRuntimeHandle`, and so on. Enumerate
with `grep -rhoE "^(enum|struct|final class|class) Native\w+"` before adding any
new `Native*` name.

### Swift changes made

- `Support/NativeHandles.swift` — the protocol and 12 structs.
- `NativeHandleState`/`NativeHandleBox` generic over `Handle: NativeHandle`.
- `NativeHandleFactory.create` returns a generic `Handle` from a `UInt64`
  out-parameter.
- `NativeHandleLeak.address: UInt` -> `handle: UInt64`.
- Public `RuntimeEventSource.map(NativePointer)` -> `.map(MapId)`, with a new
  `public struct MapId { public let value: UInt64 }` whose initializer is
  **internal** — the constructibility rule from the spec.
- `MapHandle.nativeAddress` -> `mapId`; the registry rekeys on it.
- `MapAttachRef.withLiveMap` deleted; attaches now resolve the id via
  `mapHandle()` and hold no lock across the native call.
- Shim parameters keyed by name (`map`, `runtime`, `session`, `projection`,
  `snapshot`, `list`, `result`) take the handle type and pass `.raw` to C.
- `NativeResultGuard` and `NativeFeatureQueryResultReader` generic/typed.
- Handle out-parameters: `withTemporary(OpaquePointer?.none)` ->
  `withTemporary(UInt64(0))`.

**Inserting `.raw` at ~300 C call sites needs a balanced-paren scanner**, not a
regex — nested calls like `cString(arena, url)` break naive matching. The
working shape: iterate `re.finditer(r"\bmln_\w+\(")`, walk to the matching close
paren counting depth, and substitute handle-named identifiers only inside that
span. Guard with `.replace(".raw.raw", ".raw")`.

Swift error trajectory: 2091 -> 1525 -> 1093 -> (in flight). Swift reports each
error ~4x across compile passes, so divide by four for the real count.

## STATE AT LAST CHECKPOINT

Committed on the branch (newest first):

- `a18220cb` native OpenGL/Metal texture frame helpers
- `61ec7f09` dotnet
- `fa1bba6d` rust source_for_event follow-up
- `2f4ceaac` go, `7cb492ca` python, `a4d2c6f4` + `7ba3c9c7` rust, `5908f12c`
  zig, `0ffb6ecb` + `805a97a3` native

Uncommitted in the tree: **kotlin** (all three backends; jvm+native tested
green, android Kotlin compiles clean and the JavaCPP mapping is CONFIRMED — the
`MaplibreNativeC.mln_map` classes are gone, `typedef uint64_t` flows as `long`,
no `Info` entries needed) and **swift** (in flight).

Not started: **dart**. Its ffigen output HAS been regenerated
(`mise run //bindings/dart:ffigen`, 701 insertions / 1339 deletions) and shows
`typedef mln_map = ffi.Uint64; typedef Dartmln_map = int;` — so Dart handles are
plain `int` and need `extension type` wrappers per the plan.

Verified this session: `mise run test` (C API, 100%),
`mise run build linux-x64-egl` (OpenGL backend, clean),
`mise run //bindings/kotlin:test` (jvmTest + linuxX64Test),
`mise run //bindings/dotnet:test` (189).

### dotnet notes (carry forward)

- The `--remap mln_map=MlnMap` generator trick worked exactly as the rust
  `.new_type_alias` did. The 12 hand-written structs live in
  `Internal/C/Handles.cs` and share an `IMlnHandle { ulong Value }` interface,
  which is what lets `NativeHandleState<T> where T : unmanaged, IMlnHandle` read
  the id without a second constructor parameter.
- `NativeLeakReport.Address (nint)` became `Handle (ulong)`. Texture-frame leaks
  pass 0: those report a `NativeMemory.Alloc`'d descriptor, not a C API handle,
  and conflating the two is the exact thing this change sharpens.
- `SyntheticHandles.cs` implements the test-seam pattern for dotnet.
- BND-045/047/049 now have dotnet tests in `NativeHandleIdentityTests.cs`,
  reached through the internal `Handle` accessors. Copy this shape to the
  remaining bindings.
- **Rust follow-up resolved** (`fa1bba6d`): `source_for_event` now reports
  `MapId::new(raw.source)` whenever the event carries an id, keeping
  `UnknownMap` only for source == 0.

### Hook hazard (cost a confusing failure)

`git commit` runs hk, which **stashes all unstaged files** during the hook. A
background build running at the same time compiles a partially-restored tree and
fails with nonsense errors. Do not run a build and a commit concurrently.

### Recurring lesson (cost real time in zig, rust, python, go)

Renaming by pattern is only safe when the pattern captures a _concept_, not a
_spelling_. Cases that bit:

- `_address` matched both MapLibre handles and backend GPU pointers.
- `*mut T` / `*C.mln_X` meant "handle by pointer" AND "out-parameter".
- `state.handle()` was a method on one type and a field on another.
- `handle` as a parameter name shadows Go's imported `internal/handle` package.

Enumerate the identifiers first (`grep -oE ... | sort -u`), then rename. Prefer
scoping the edit to an `impl`/`class` block over a file-wide substitution.

### Test seams: the settled pattern

Replace fabricated pointers (`&mut value`, `0x1234 as *mut T`, `C.malloc(1)`)
with a synthetic handle of a distinct type whose **kind byte matches the type it
stands in for**, so a value reaching a diagnostic reads as an obviously
synthetic handle of the right kind rather than a plausible pointer or a
confusing wrong-kind message. Kind bytes: runtime 0x01, map 0x02, projection
0x03, render session 0x04, wake source 0x0b, resource request 0x0c.

---

# ALL EIGHT BINDINGS ARE COMMITTED

| Binding | Commit                       | Gate                                              |
| ------- | ---------------------------- | ------------------------------------------------- |
| native  | 805a97a3, 0ffb6ecb, a18220cb | `mise run test` 100%; `build linux-x64-egl` clean |
| zig     | 5908f12c                     | tests + headless Vulkan render                    |
| rust    | 7ba3c9c7, a4d2c6f4, fa1bba6d | 202 tests, clippy clean                           |
| python  | 7cb492ca                     | 131 tests, `ty` clean                             |
| go      | 2f4ceaac                     | all packages, vet clean                           |
| dotnet  | 61ec7f09                     | 189 tests, generate-check green                   |
| kotlin  | c20a7cae                     | jvmTest + linuxX64Test; androidBuild clean        |
| swift   | 4904d715                     | 84 tests                                          |
| dart    | b20f0f69                     | 58 tests, analyze + ffigen-check clean            |

Plus the BND-196 conformance row.

## Phase 6 status

- **BND-045/047/049 now covered in all eight bindings** (`7a3ac309`,
  `593ca577`). Each uses its own internal seam:
  - rust: `map.inner.native()`
  - go: helpers in `map.go` returning the binding error — Go forbids cgo in
    `_test.go` files
  - zig: `@intFromEnum(handle)`
  - python: `map_size_by_id_for_test` / `pump_runtime_with_map_id_for_test`
    `#[pyfunction]`s, plus `.pyi` stubs
  - dotnet: `NativeHandleIdentityTests.cs`
  - kotlin: `nativeHandle()` + `rawHandleValue` in `MapHandleTest.kt`
  - swift: `requireLiveHandle()` in `HandleIdentityTests.swift` (remember to add
    the file to `Package.swift`'s explicit source list)
  - dart: `MapAttachRef.mapIdForTesting`; wrong-thread runs via `Isolate.run`
- **Docs needed no change**: `concepts.md` and `overview.md` never described
  handles as pointers. `c-conventions.md` already has the `## Handles` section
  from Phase 4.
- **BND-049 is gated on capability.** Dart cannot express it: a handle must be
  closed before the test awaits another isolate (an isolate may resume on a
  different native thread), which leaves the id stale rather than live. The spec
  now names that boundary, matching the render-session rows' gate. The first
  attempt at this test was flaky, racing the stale and wrong-thread diagnostics.
- `mise run fix` passes clean over the whole tree with no changes.
- Metal sources are still unbuilt on Linux; a macOS CI run is their first gate.

## Final verification (all green)

`mise run test` (C API), `mise run build linux-x64-egl`,
`//examples/zig-readback:run`, and every binding's test task: rust 119, go all
packages, zig 167, python 134, dotnet 189, kotlin jvm+linuxX64, swift 87,
dart 60.

**Caution:** running several `mise run` tasks concurrently causes contention
that shows up as spurious failures (rust exit 101, go exit 1 with a truncated
log). Verify sequentially.

# RESUME HERE (historical checkpoint)

## Swift — DONE (commit 4904d715), 84 tests pass

Was: ~520 errors. Extra things learned finishing it:

- `Package.swift` lists test sources **explicitly**; a new test file must be
  added there or SwiftPM silently reports "found 1 file(s) which are unhandled"
  and every reference to it fails as "cannot find X in scope".
- The `.raw` scanner must also catch `requireLiveHandle()` / `mapHandle()` /
  `handle.requireLive()` results, including when split across lines.
- Inside `NativeHandleFactory.create { X in }` and
  `withTemporary(UInt64(0)) { X in }` the binder is the **out-parameter**, not a
  handle. Renamed those binders to `outHandle` so the distinction is visible.
- `let pointer = try mapNativeFailure {` appears for several kinds; a blanket
  rename to `session` corrupted the runtime and projection initialisers.

## Swift — historical notes (superseded)

`LD_LIBRARY_PATH=/tmp/swiftdeps/env2/lib mise run //bindings/swift:build`
(recreate that prefix with the micromamba recipe above if /tmp was cleared).

Trajectory 2091 -> 1525 -> 1093 -> 971 raw (divide by ~4 for real errors).

The remaining errors are ONE shape and need reading, not scripting:
`Support/NativeOffline.swift`, `NativeQuery.swift`, `NativeStyle.swift` read a
handle out-parameter and still treat it as an optional pointer —
`guard let snapshot = ...` on a value that is now a non-optional `UInt64`. Each
site should become:

```swift
let handle = NativeJSONSnapshotHandle(raw: outHandle.pointee)  // or the right kind
guard !handle.isNull else { ... }
```

and the local should be passed on as the typed handle, not `.raw`. Then
`Map.swift`/`CameraAdvanced.swift` pass `NativeMapHandle` straight to `mln_*` in
a few places the balanced-paren scanner missed — those just need `.raw`.

After it builds: `LD_LIBRARY_PATH=... mise run //bindings/swift:test`.

## Dart — DONE (commit 336dac6a). 58 tests, analyze + ffigen-check clean.

**All eight bindings are now committed.** Decision taken (user's call):
`ResourceRequestHandle` became an `extension type` over the id, so it is
sendable and crosses isolates directly. `isReleased` is gone — the C API is the
one-shot authority. The alias-race test now races two `complete()` calls,
because `release` is idempotent by design and cannot lose a race.

## Dart — details (historical)

`lib/` is **clean**. All 11 remaining are in
`test/maplibre_native_ffi_test.dart` and are the same blocked decision below.

Done:

- `native_handles.dart`: 12 `extension type const Native*(int raw)`.
  **`NativeHandle` must `implements Object`** or Dart refuses to treat a type
  parameter bounded by it as non-nullable.
- `NativeHandleState<H extends NativeHandle>` holds a **non-nullable** `H` plus
  a `bool _closed`. Dart will not promote `H?` through `||` when H's bound is an
  extension type, so the .NET flag shape is what works here too.
- Handle out-parameters allocate as `arena<Uint64>()` with `.value = 0`.
  **`outValue` in `runtime_native_conversions.dart` is NOT one** — it holds an
  `mln_json_value*` and keeps `nullptr`.
- `MapAttachRef` carries the map's id; `_pointer`/`pointerAddress` became
  `_handle`/`handleId`; the live-map table rekeys on the id.
- `ResourceRequestToken`, `transfer()`, `_checkResourceRequestToken`, and
  `ResourceRequestHandle._ownerIsolateHash` are deleted, and
  `waitUntilRetired()` (over `mln_resource_request_wait_until_retired`) is on
  the handle.
- `targets.dart` was reverted once: its `_pointer` is a real `Pointer<Uint8>`
  buffer, not a handle.

### BLOCKED — needs a decision

The deleted `ResourceRequestToken` existed so a host could complete a request
from **another isolate**. `ResourceRequestHandle` is now supposed to take that
role, but as a `final class` holding a `NativeLeakReporter` (a `Pointer<Void>`)
and `implements Finalizable`, **it cannot cross an isolate** — `Isolate.run`
refuses objects carrying native resources.

Two ways out, and they trade against the spec rule this plan added:

1. Make `ResourceRequestHandle` an `extension type` over the id. Trivially
   sendable and matches "the handle id _is_ the token". Costs the leak reporter
   and the local `_released` flag — though the C API already rejects a released
   id, so the flag is redundant for safety, and `isReleased` would have to go or
   change meaning.
2. Keep the class and add a public id + rebuild-from-id path. This **violates**
   the constructibility rule the spec now states ("a handle ... can never be
   built from an integer in the safe API").

The four affected tests are
`queued resource provider callbacks cross the
native C ABI`,
`transferred response validation preserves the live token`,
`transferred token aliases have one terminal winner`, and the
`_completeTransferredRequest` / `_completeTokenAlias` helpers.

## Dart — historical notes (superseded)

Done so far:

- ffigen regenerated and committed-ready (`typedef Dartmln_map = int`).
- `lib/src/internal/lifecycle/native_handles.dart` — new, 12
  `extension type const Native*(int raw) implements NativeHandle`.
- `lifecycle.dart` — `NativeHandleState<H extends NativeHandle>` holds
  `H?
  _handle`; `pointer` -> `handle`, `pointerAddress` -> `handleId`;
  `_createLeakToken` now takes `Uint64`.

Still to do (from `mise run //bindings/dart:analyze`, 257 errors):

- `runtime.dart` (~120), `runtime_render_handles.dart` (~50),
  `runtime_offline.dart`, `runtime_native_conversions.dart`,
  `runtime_resource_callbacks.dart`: every `Pointer<mln_X>` becomes the matching
  `NativeX`, and `Pointer<Pointer<mln_X>>` out-parameters become
  `Pointer<Uint64>`.
- Per the plan: delete `ResourceRequestToken`, `transfer()`,
  `_checkResourceRequestToken`, and `ResourceRequestHandle._ownerIsolateHash`;
  make `MapAttachRef` an `extension type MapAttachRef(int _id)`. **KEEP**
  `_checkOwnerIsolate` and its #412 diagnostic, and KEEP `_leakToken` a real
  `Pointer<Void>` — only its _argument_ became an id.
- `test/internal_support_test.dart` has a `_FakeNativeHandle` that must now
  implement `NativeHandle`.
- Gates (Dart is NOT in CI, so run all three locally):
  `mise run //bindings/dart:ffigen-check`, `:analyze`, `:test`.

## Kotlin — uncommitted but VERIFIED, ready to commit

`mise run //bindings/kotlin:test` passes (jvmTest + linuxX64Test).
`mise run //bindings/kotlin:androidBuild opengl x86_64` compiled all Kotlin
sources clean (COUNT 0) — the only failure was the native OpenGL C++, which is
now fixed and committed as `a18220cb`. **Re-run androidBuild to confirm, then
commit Kotlin.**

## Phase 6 remaining

- BND-196 row added to the spec. BND-045/047/049 rows already existed; dotnet
  has tests (`NativeHandleIdentityTests.cs`) — copy that shape to the other
  bindings.
- `concepts.md` / `overview.md` / guides handle wording.
- Doxyfile `TYPEDEF_HIDES_STRUCT` was already removed.
- `mise run fix` over the whole tree before the PR.
