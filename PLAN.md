> **Temporary planning document.** This is the plan that guided
> [#441](https://github.com/maplibre/maplibre-native-ffi/pull/441), committed
> per [AI_POLICY.md](./AI_POLICY.md) so reviewers can follow the reasoning.
> Remove it before merge. It records the plan as written up front; where the
> implementation reached a better answer, see **Divergences** at the end rather
> than trusting the body.

# Fill five MapLibre Native API gaps in the C API and bindings

## Context

Five MapLibre Native capabilities are unreachable (or only awkwardly reachable)
through this project's C API, so no binding exposes them:

1. **Layer source-layer setter** — reachable today only via
   `mln_map_set_layer_property(map, id, "source-layer", …)`, which **silently
   succeeds and does nothing** when the layer type's source is not
   `LayerTypeInfo::Source::Required`
   (`third_party/maplibre-native/src/mbgl/style/layer.cpp:188-199`). There is no
   getter at all: `Layer::getProperty` is implemented per concrete layer type
   and only handles paint/layout properties, so
   `mln_map_get_layer_property(…, "source-layer", …)` returns a null snapshot.
   The same asymmetry applies to the rest of the `mbgl::style::Layer` base class
   (`source`, `minzoom`, `maxzoom`, `visibility`).
2. **Style image content insets** — `mln_map_set_style_image()` calls the
   four-argument `mbgl::style::Image` constructor and defaults away `stretchX`,
   `stretchY`, `content`, `textFitWidth`, and `textFitHeight`, so nine-patch
   sprites and `icon-text-fit` are unreachable.
3. **Ambient cache size** — `setMaximumAmbientCacheSize` is called exactly once,
   lazily, at first database access, from a creation-time option, **with its
   error swallowed** (`src/runtime/runtime.cpp:927-931`). It cannot be changed
   for the life of the runtime and failures are invisible.
4. **Offline tile count limit** — **out of scope, see below.**
5. **`synchronousUpdate` on GeoJSON options** — missing from
   `mln_geojson_source_options`. Reachable today only by hand-writing a source
   JSON document for `mln_map_add_style_source_json()`, because MapLibre's
   `Converter<GeoJSONOptions>` already parses the key
   (`third_party/maplibre-native/src/mbgl/style/conversion/geojson_options.cpp:104-110`).

All five are straightforward — none needs new machinery. The cost is fan-out:
eight bindings (dart, dotnet, go, kotlin, python, rust, swift, zig), two
committed generated files, and a jextract allow-list. (`bindings/java-ffm` and
`bindings/java-jni` are untracked stale build output — ignore them.)

## Decisions taken

- **Style images**: add all five constructor arguments (stretch, content, and
  both `TextFit` values), not just content insets. Content without stretches is
  not usable nine-patch, and `textFit*` is the same constructor and the same ABI
  break.
- **Ambient cache size**: add a runtime setter **and remove** the creation-time
  `MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE` option, rather than keeping two ways
  to set one value. The ABI is unstable (`mln_c_version()` returns 0) and
  AGENTS.md prefers breaks over redundant paths.
- **Layer accessors**: add typed set/get for the whole `mbgl::style::Layer` base
  class, not just source-layer. `mln_map_set_layer_filter` /
  `mln_map_get_layer_filter` are the existing precedent for a typed carve-out of
  a base-`Layer` property that `setProperty` can also reach, so this completes a
  pattern rather than adding a redundant one.
- **Offline tile count limit: not doing it.** Noted below so the reasoning
  survives.

## Not doing: offline tile count limit

`DatabaseFileSource::setOfflineMapboxTileCountLimit`
(`third_party/maplibre-native/include/mbgl/storage/database_file_source.hpp:248`)
is dropped from scope. Two reasons worth recording:

- It applies only to _canonical_ Mapbox-hosted tile URLs —
  `OfflineDatabase::exceedsOfflineMapboxTileCountLimit`
  (`platform/default/src/mbgl/storage/offline_database.cpp:1454-1457`) gates on
  `util::mapbox::isCanonicalURL(tileServerOptions, resource.url)`. For an
  ordinary MapLibre style with plain HTTPS tile URLs it does nothing.
- It is a `const void` call with no callback and no error channel, so it fits
  neither the offline `_start`/operation-id family nor an honest synchronous
  status.

`MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED` stays as-is; only
the setter is skipped.

---

## Work item 1 — GeoJSON `synchronousUpdate`

Cheapest of the five; land it first as the template for the fan-out.

**C API** (`include/maplibre_native_c/style.h`):

- Add `MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE = 1U << 11U` to
  `mln_geojson_source_option_field` (bit 11 is free).
- Add `bool synchronous_update;` to `mln_geojson_source_options` after
  `cluster`, documented as "Defaults to false."

> ABI note: the new `bool` lands in existing tail padding, so `sizeof` stays 72
> and the `size` check cannot distinguish old from new callers. That is correct
> here — the field mask is the presence signal and old callers leave bit 11
> clear.

**Implementation** — four sites in `src/map/map.cpp`, all adjacent to existing
`cluster` handling:

- `effective_geojson_source_options` (507) — copy through when the bit is set.
- `validate_geojson_source_options` (573) — add the bit to `known_fields`.
- `to_native_geojson_source_options` (664) —
  `native.synchronousUpdate = options.synchronous_update;`
- `geojson_source_options_default` (3157) — seed from
  `mbgl::style::GeoJSONOptions{}`.

## Work item 2 — Typed `mbgl::style::Layer` base accessors

**New enum** in `style.h`, matching mbgl ordinals (peer style enums such as
`mln_style_tile_scheme` are 0-based):

```c
typedef enum mln_style_layer_visibility : uint32_t {
  MLN_STYLE_LAYER_VISIBILITY_VISIBLE = 0,
  MLN_STYLE_LAYER_VISIBILITY_NONE = 1,
} mln_style_layer_visibility;
```

**New functions**, placed next to `mln_map_set_layer_filter` /
`mln_map_get_layer_filter` (`style.h:1363-1402`) and following their exact doc
and status shape:

```c
mln_status mln_map_set_layer_source_layer(mln_map, mln_string_view layer_id,
                                          mln_string_view source_layer);
mln_status mln_map_copy_layer_source_layer(mln_map, mln_string_view layer_id,
                                           char* out_text, size_t text_capacity,
                                           size_t* out_text_size, bool* out_found);
mln_status mln_map_set_layer_source_id(mln_map, mln_string_view layer_id,
                                       mln_string_view source_id);
mln_status mln_map_copy_layer_source_id(mln_map, mln_string_view layer_id,
                                        char* out_text, size_t text_capacity,
                                        size_t* out_text_size, bool* out_found);
mln_status mln_map_set_layer_min_zoom(mln_map, mln_string_view layer_id, double min_zoom);
mln_status mln_map_get_layer_min_zoom(mln_map, mln_string_view layer_id, double* out_min_zoom);
mln_status mln_map_set_layer_max_zoom(mln_map, mln_string_view layer_id, double max_zoom);
mln_status mln_map_get_layer_max_zoom(mln_map, mln_string_view layer_id, double* out_max_zoom);
mln_status mln_map_set_layer_visibility(mln_map, mln_string_view layer_id, uint32_t visibility);
mln_status mln_map_get_layer_visibility(mln_map, mln_string_view layer_id, uint32_t* out_visibility);
```

Contract details that must be right:

- **String getters use the copy shape, not a borrowed view.**
  `Layer::getSourceLayer()` and `getSourceID()` return `std::string` _by value_
  (`third_party/maplibre-native/include/mbgl/style/layer.hpp:123-124`), so a
  borrowed `mln_string_view` would dangle. Mirror
  `mln_map_copy_style_source_attribution` (`style.h:401-422`) exactly —
  capacity, required-size-out, `out_found`, and `MLN_STATUS_INVALID_ARGUMENT`
  when capacity is too small for present text. Do **not** copy the
  `mln_map_get_style_layer_type` borrowed-view shape; that one is only safe
  because layer type names are static.
- **`set_layer_source_layer` and `set_layer_source_id` reject wrong layer
  types.** This is the whole point of the typed setters: check
  `layer->getTypeInfo()->source != mbgl::style::LayerTypeInfo::Source::Required`
  and return `MLN_STATUS_INVALID_ARGUMENT` with a thread error, instead of
  mbgl's silent `Log::Warning` no-op. Note `Layer::setSourceLayer` /
  `setSourceID` themselves have no such gate — only `setProperty` does — so the
  check belongs in our layer.
- **Zoom is unbounded by default.** `Layer::Impl` defaults to
  `∓std::numeric_limits<float>::infinity()`
  (`third_party/maplibre-native/src/mbgl/style/layer_impl.hpp:58-59`). Use
  `double` at the ABI (consistent with
  `mln_style_tile_source_options.min_zoom`), cast to `float` for mbgl, and
  document that ±infinity means unbounded.
- Reject unknown `visibility` values with `MLN_STATUS_INVALID_ARGUMENT`.

**Implementation**: `src/map/map.cpp` next to `map_set_layer_filter` (5577) and
`map_get_layer_filter` (5613); prototypes in `src/map/map.hpp:223-236`; one
`status_boundary` thunk each in `src/c_api/map.cpp` alongside 612-648.

## Work item 3 — Style image stretch, content, and text fit

**New public types** in `style.h`:

```c
/** One stretchable interval along an image axis, in image pixels. */
typedef struct mln_image_stretch { float from; float to; } mln_image_stretch;

/** Content-box insets in image pixels. */
typedef struct mln_image_content { float left, top, right, bottom; } mln_image_content;

typedef enum mln_style_image_text_fit : uint32_t {
  MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK = 0,
  MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY = 1,
  MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL = 2,
} mln_style_image_text_fit;
```

**Extend `mln_style_image_option_field`** (`style.h:94-98`) with
`_STRETCH_X = 1U << 2U`, `_STRETCH_Y = 1U << 3U`, `_CONTENT = 1U << 4U`,
`_TEXT_FIT_WIDTH = 1U << 5U`, `_TEXT_FIT_HEIGHT = 1U << 6U`.

**Extend `mln_style_image_options`** (`style.h:229-237`) with pointer+count
arrays and scalars, per the C-conventions array rule (borrowed for the call,
copied before return):

```c
const mln_image_stretch* stretch_x;  size_t stretch_x_count;
const mln_image_stretch* stretch_y;  size_t stretch_y_count;
mln_image_content content;
uint32_t text_fit_width;   /* one of mln_style_image_text_fit */
uint32_t text_fit_height;
```

**Symmetric readback.** `mln_map_get_style_image_info` already exists, so
leaving it half-blind would violate the "complete domain" binding invariant. Add
to `mln_style_image_info` (`style.h:239-249`) the fixed-size parts —
`bool has_content; mln_image_content content;`,
`bool has_text_fit_width; uint32_t text_fit_width;` (same for height), and
`size_t stretch_x_count; size_t stretch_y_count;`. The variable-length arrays
get one copy function following `mln_map_copy_style_image_premultiplied_rgba8`
(`style.h:825-848`) — capacity in, required count out, `out_found`:

```c
mln_status mln_map_copy_style_image_stretches(
  mln_map map, mln_string_view image_id,
  mln_image_stretch* out_stretch_x, size_t stretch_x_capacity, size_t* out_stretch_x_count,
  mln_image_stretch* out_stretch_y, size_t stretch_y_capacity, size_t* out_stretch_y_count,
  bool* out_found);
```

**Implementation** — all in the existing helper cluster in `src/map/map.cpp`:

- `validate_style_image_options` (1034) — new mask bits; reject non-finite
  stretch/content floats, `from > to`, null array with non-zero count, and
  unknown `text_fit_*` enum values.
- `effective_style_image_options` (1062) — pass the new fields through.
- `map_set_style_image` (4458) — build `mbgl::style::ImageStretches` vectors and
  `std::optional<ImageContent>` / `std::optional<TextFit>`, then call the
  nine-argument `mbgl::style::Image` constructor
  (`third_party/maplibre-native/include/mbgl/style/image.hpp:35-43`).
- `style_image_info_from_native` (1171) — read back via `getStretchX()`,
  `getStretchY()`, `getContent()`, `getTextFitWidth()`, `getTextFitHeight()`.
- `mln_style_image_options_default` (`map.cpp:3195-3225`) and the new copy
  function's core + thunk (`src/c_api/map.cpp:339-347` is the neighbour).

## Work item 4 — Ambient cache size setter, replacing the creation-time option

**Remove** from `include/maplibre_native_c/runtime.h`:

- the `mln_runtime_option_flag` enum (27-29) — it has exactly one member and
  becomes empty, which is not valid C;
- `mln_runtime_options.maximum_cache_size` (286-295).

Keep `mln_runtime_options.flags`, documented as "No flags are currently defined;
must be zero." The validation at `src/runtime/runtime.cpp:885-905` collapses
from a `known_flags` mask to `options->flags != 0`.

**Add** a new async operation following
`mln_runtime_run_ambient_cache_operation_start` (`runtime.h:769-788`) exactly:

```c
typedef enum mln_offline_operation_kind : uint32_t {
  /* … existing 1-11 … */
  MLN_OFFLINE_OPERATION_SET_MAXIMUM_AMBIENT_CACHE_SIZE = 12,
} mln_offline_operation_kind;

MLN_API mln_status mln_runtime_set_maximum_ambient_cache_size_start(
  mln_runtime runtime, uint64_t size, mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;
```

**Implementation** in `src/runtime/runtime.cpp`, modelled on
`run_ambient_cache_operation_start` (1154-1217):

- Validate runtime and `out_operation_id`; resolve
  `database_source_for_runtime`.
- Call
  `schedule_registered_offline_operation(live,
  MLN_OFFLINE_OPERATION_SET_MAXIMUM_AMBIENT_CACHE_SIZE,
  MLN_OFFLINE_OPERATION_RESULT_NONE, out_operation_id, …)`
  and inside the schedule lambda call
  `database->setMaximumAmbientCacheSize(size, callback)` with the standard
  `complete_from_exception` adapter. The mbgl callback type is
  `std::function<void(std::exception_ptr)>` — identical to
  `resetDatabase`/`packDatabase`/`clearAmbientCache`, so this drops in with no
  new machinery and the native error finally reaches the host.
- Delete the `has_maximum_cache_size` block from `database_source_for_runtime`
  (913-934) and the two `RuntimeObject` fields
  (`src/runtime/runtime.hpp:145-146`) and their option plumbing
  (`runtime.cpp:1043-1051`, `2699-2703`,
  `find_maximum_cache_size_for_platform_context` at `2729-2744` and its caller
  in `src/resources/resource_loader.cpp:343-350`).

> Check `find_maximum_cache_size_for_platform_context` carefully during
> implementation — it feeds `resource_loader.cpp`, so removing the stored size
> may mean that call site drops out entirely rather than being rewired.

Prototype in `src/runtime/runtime.hpp` near 203; thunk in
`src/c_api/runtime.cpp` near 99-108.

---

## Binding fan-out (shared across all four work items)

Every binding follows the same three-layer shape, so each item repeats one
mechanical pattern. Representative paths per binding — the same files carry all
four changes:

| Binding | Option/struct materializer                                                                                                                                                            | Public type                                                |
| ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------- |
| Rust    | `bindings/rust/crates/maplibre-native-core/src/style.rs` (`NativeGeoJsonSourceOptions::new`, 132)                                                                                     | same file, `GeoJsonSourceOptions` (103)                    |
| Kotlin  | `src/nativeMain/.../internal/struct/StyleStructs.kt`, `src/jvmMain/.../internal/loader/NativeAccess.kt` (hand-written bit + offset constants), `src/androidMain/.../map/MapHandle.kt` | `src/commonMain/.../style/GeoJsonSourceOptions.kt`         |
| Swift   | `Sources/MaplibreNative/Support/StyleStructs.swift`                                                                                                                                   | `Sources/MaplibreNative/Style.swift`                       |
| Go      | `bindings/go/style.go` (struct, `Equal`, `Clone`, `With…` builder, `newC…`)                                                                                                           | same file                                                  |
| Python  | `bindings/python/src/lib.rs` (Rust pyo3 side)                                                                                                                                         | `python/maplibre_native/style.py`, `map.py`, `_native.pyi` |
| Dart    | `lib/src/runtime/runtime_native_conversions.dart`                                                                                                                                     | `lib/src/style/style.dart`                                 |
| .NET    | `src/Maplibre.Native/Internal/Struct/StyleStructs.cs`                                                                                                                                 | `src/Maplibre.Native/Style/StyleTypes.cs`                  |
| Zig     | `bindings/zig/src/map.zig`                                                                                                                                                            | `bindings/zig/src/values.zig`                              |

Cross-cutting obligations:

- **Regenerate committed artifacts**: `bindings/dotnet/.../Generated/*.g.cs` via
  `bindings/dotnet/scripts/generate-clangsharp.sh`, and
  `bindings/dart/lib/src/internal/c/maplibre_native_c.g.dart` via
  `mise run //bindings/dart:ffigen`. Both have CI drift gates.
- **Kotlin jextract allow-list**: every new function, enum constant, and struct
  must be added to `bindings/kotlin/src/jextract/maplibre-native-c.includes`
  (alphabetical), or the JVM binding silently loses them.
- **Rust `-sys` and Python** are bindgen-generated and need no manual edit.
- **Strict test enumerations that will fail if not updated** — these are the
  ones that bite:
  - `bindings/rust/crates/maplibre-native-core/src/style.rs:537-599` asserts the
    _exact OR_ of all option mask bits.
  - BND-070 value-semantics tests, which require "one case per option type
    mutates each field in turn":
    `bindings/kotlin/src/commonTest/.../OptionsValueSemanticsTest.kt`,
    `bindings/dotnet/tests/.../OptionsValueSemanticsTests.cs`,
    `bindings/go/equal_test.go`.
  - `bindings/swift/Tests/MaplibreNativeTests/StyleTests.swift` and
    `bindings/kotlin/src/nativeTest/.../StyleStructsTest.kt` field-mask
    materialization tests.
  - Every binding's `RuntimeOptions` test, once `maximum_cache_size` is removed.
- **C ABI tests**: extend `src/c_api/tests/style_values_abi.c` (GeoJSON and
  image option masks — its unknown-bit case uses `1U << 31U`, so new low bits
  are safe) and `src/c_api/tests/resources_abi.c` (new ambient-cache-size
  operation).
- **Docs**:
  `docs/src/content/docs/guides/add-data-and-style-at-runtime.mdx:41-45`
  enumerates the GeoJSON option categories in prose and has no mention of
  nine-patch images. `docs/src/content/docs/guides/download-offline-regions.mdx`
  covers the ambient-cache change. The Doxygen C reference and rustdoc pick new
  fields up automatically.

## Suggested sequencing

Four independent changes; land them separately, smallest first, so the first one
establishes the fan-out rhythm:

1. GeoJSON `synchronousUpdate` — 1 field, ~26 production sites.
2. Ambient cache size setter + option removal — touches `RuntimeOptions` in all
   eight bindings, so it is the most disruptive per line but conceptually small.
3. Layer base accessors — 10 new C functions, no struct changes.
4. Style image stretch/content/text-fit — largest: new types, extended option
   and info structs, plus a new copy function.

## Verification

Per change, in this order:

```bash
mise run fix                          # formatters/linters, stages touched files
mise run test                         # builds native lib + C API tests
mise run //bindings/rust:test         # fastest full binding suite
```

Then the remaining binding suites (`mise tasks --all` lists them per binding),
and for the generated-artifact bindings confirm the drift gates pass:

```bash
mise run //bindings/dart:ffigen-check
```

End-to-end behavior checks that actually exercise the new paths:

- **synchronousUpdate**: add a GeoJSON source with the flag set through a public
  binding API and confirm the source loads and renders — reuse the existing
  cluster fixture in `bindings/python/tests/render_backend_helpers/runtime.py`
  or `bindings/zig/tests/geojson.zig`.
- **Layer accessors**: load a style, set then read back source-layer, source-id,
  min/max zoom, and visibility on a symbol layer; assert
  `MLN_STATUS_INVALID_ARGUMENT` when setting source-layer on a background layer
  (the silent-no-op case this work item exists to fix); assert an unset zoom
  reads back as ±infinity.
- **Style images**: set a nine-patch image with stretches and content, read back
  through `mln_map_get_style_image_info` + `mln_map_copy_style_image_stretches`,
  and assert an undersized stretch buffer reports the required count and fails
  without losing caller ownership (BND-166's shape). Render it via
  `mise run //examples/zig-readback:run` to confirm the sprite actually
  stretches.
- **Ambient cache size**: create a runtime with a `cache_path`, call the setter,
  pump, and assert an `MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED` event with
  kind `SET_MAXIMUM_AMBIENT_CACHE_SIZE` and `result_status == MLN_STATUS_OK`;
  extend `bindings/kotlin/src/nativeTest/.../RuntimeOfflineTest.kt` and the
  equivalent Rust test at `crates/maplibre-native/src/runtime.rs:1062-1095`.

---

## Divergences from this plan

Recorded after implementation. The body above is the original plan; these are
the places the code deliberately went elsewhere.

**Copy-out getters became size probes.** The plan said to mirror
`mln_map_copy_style_source_attribution` exactly, which returns
`MLN_STATUS_INVALID_ARGUMENT` when the caller's capacity is too small. That does
not work for the layer getters: a missing layer returns the same status, so a
caller sizing a buffer cannot tell the two apart. Attribution escapes this only
because its length also arrives through `mln_style_source_info`, and layers have
no equivalent struct. A null buffer with zero capacity is therefore a size probe
that returns `MLN_STATUS_OK`. This was then applied to the whole copy-out
family, including `texture_read_premultiplied_rgba8`, and written into
[C API Conventions](./docs/src/content/docs/development/c-conventions.md).

**The layer copy getters dropped `out_found`.** The plan carried it over from
the attribution signature. The layer family already reports a missing layer as
`MLN_STATUS_INVALID_ARGUMENT` (`get_layer_filter`, `set_layer_property`), and
`out_found` belongs to the source/existence queries, so the parameter was
redundant.

**Rust needed a borrow-holding options wrapper.**
`style_image_options_to_native` returned a plain value. Once the native struct
points at caller-owned stretch arrays, it has to return a
`NativeStyleImageOptions` that keeps them alive for the call, matching
`NativeTileSourceOptions`.

**`mln_runtime_option_flag` was deleted, not just emptied.** Removing its single
member would have left an empty enum, which is not valid C. `flags` stays on
`mln_runtime_options`, reserved and required to be zero.

**`find_maximum_cache_size_for_platform_context` disappeared entirely,** as the
plan flagged it might. Its only caller applied the stored size in the
`DatabaseFileSource` factory; with no stored size, that block and four
now-unused includes went with it.

**Kotlin needed a JVM-specific test.** `NativeAccess.downcall()` resolves
symbols reflectively by string, so a missing jextract allow-list entry fails at
call time rather than compile time. Native-only tests would not have caught it.

**Not every strict test enumeration needed updating.** The plan listed Kotlin's
`StyleStructsTest` and Dart's `foundational_values_test` as exhaustive; both are
representative spot checks. Conversely, .NET's `PublicApiSurfaceTests` asserts
an exact public-type list, which the plan did not mention — adding a type trips
it, adding a field does not.
