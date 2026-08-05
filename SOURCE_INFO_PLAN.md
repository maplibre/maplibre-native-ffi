# Reconstructible style source information

Expand style source inspection so that a host can reconstruct every native
source shape whose retained state is available through MapLibre Native. Keep
`mln_style_source_info` as the fixed-size snapshot, use copy-out calls for
strings, and use an owned string-list handle for tile URLs.

## C API contract

`mln_map_get_style_source_info()` remains the single fixed-metadata query. Add a
field mask and these fields to `mln_style_source_info`:

- source URL presence and byte length;
- parsed TileJSON presence and tile URL count;
- TileJSON minimum zoom, maximum zoom, scheme, and optional bounds;
- tile size when the native source exposes it;
- vector and raster encoding when the native source exposes them.

The field mask distinguishes an absent value from a present zero or default
value. Attribution keeps its existing presence flag and byte length. Existing
type, ID length, volatility, and attribution behavior stays unchanged.

The snapshot reports retained live state:

- A URL-backed tile source reports its original source URL immediately.
- A URL-backed tile source reports parsed TileJSON fields after MapLibre has
  loaded its description.
- An inline tile source reports parsed TileJSON fields immediately and has no
  source URL.
- GeoJSON and image sources report their retained URL when present.
- Source kinds that do not carry a URL or TileJSON leave those fields absent.

“Parsed TileJSON” means the normalized fields that MapLibre retains. The API
does not promise the original TileJSON document, unknown members that MapLibre
discarded, or byte-for-byte source JSON. A host rebuilds a URL-backed source
from its URL and rebuilds an inline tile source from its tile URL list and
reported options.

Add these helpers:

```c
MLN_API mln_status mln_map_copy_style_source_url(
  mln_map map,
  mln_string_view source_id,
  char* out_url,
  size_t url_capacity,
  size_t* out_url_size,
  bool* out_found
) MLN_NOEXCEPT;

MLN_API mln_status mln_map_get_style_source_tile_urls(
  mln_map map,
  mln_string_view source_id,
  mln_style_string_list* out_tile_urls,
  bool* out_found
) MLN_NOEXCEPT;
```

`mln_map_copy_style_source_url()` follows the existing attribution copy-out
contract. A source with no URL returns size zero while `out_found` remains true.

`mln_map_get_style_source_tile_urls()` returns an owned, immutable string-list
snapshot. A source with no parsed TileJSON returns an empty list while
`out_found` remains true. Add count, indexed-get, and destroy functions for
`mln_style_string_list`. Indexed strings remain borrowed until the list is
destroyed. The new list type stays separate from `mln_style_id_list`; migrating
existing ID APIs is outside this change.

The public header comments define every field’s applicability. Unknown future
source types preserve their raw type value and leave unsupported fields absent.

## C API implementation and tests

1. Extend the declarations in `include/maplibre_native_c/style.h` and the
   forwarding layer in `src/c_api/map.cpp`.
2. Add the string-list handle kind and implementation. Reuse the locking and
   lifetime behavior of the existing style ID list.
3. Extract URLs by native source subtype. Extract TileJSON from tile-source
   retained state without fetching resources or parsing the loaded style
   document again.
4. Populate default output data before a missing-source return. Validate output
   struct sizes, output handles, capacities, owner-thread access, and source IDs
   consistently with neighboring APIs.
5. Add raw C coverage in `src/c_api/tests/style_values_abi.c` for:
   - inline vector TileJSON with multiple tile URLs and every common field;
   - URL-backed vector state before and after its TileJSON description loads;
   - raster tile size and raster DEM encoding;
   - vector encoding, optional bounds, and present default or zero values;
   - GeoJSON and image source URLs;
   - source kinds without URLs or TileJSON;
   - missing sources, empty values, size probes, undersized buffers, invalid
     output handles, indexed list bounds, and list destruction;
   - enough returned data to call the matching URL or tile source adder under a
     new ID successfully.
6. Run `mise run test`. Run targeted formatting only on changed files.

If the pinned MapLibre headers do not expose retained TileJSON state, add the
smallest read-only upstream accessor needed by this repository. Do not mirror
source descriptors in FFI-owned state or infer them from the load-time style
document.

## Binding specification

Update `docs/src/content/docs/development/binding-specification.md` after the C
contract and C tests pass.

Specify that each binding:

- exposes source information as copied language-owned values;
- represents absent URL, TileJSON, bounds, tile size, and encodings explicitly;
- copies URL, attribution, and every tile URL before releasing native storage;
- preserves unknown source, scheme, and encoding enum values according to the
  binding’s existing unknown-enum convention;
- keeps native string-list handles internal and destroys them after copying;
- exposes parsed TileJSON as a nested value when that is idiomatic, while
  preserving the same operation boundary and absence semantics;
- returns source information that remains valid after source removal, style
  replacement, and map release.

Add a binding test requirement to the style domain. The public binding test must
inspect both URL-backed and inline tiled sources, copy multiple tile URLs,
distinguish absent fields, and retain the result after the map no longer owns
the source. Extend the native snapshot/list cleanup requirement to cover the new
string-list handle on success and copy failure.

Build the documentation site or run its repository task after the specification
change, then run targeted formatting.

## Bindings

Each binding updates generated raw declarations where applicable, public source
information values, map accessors, conversion support, and public tests. Public
APIs return copied values rather than the native list handle.

Work begins only after the C API and binding specification are stable.

1. Implement Rust first because the Python extension shares
   `maplibre-native-ffi-core`. Update the sys declarations, core conversion,
   safe map API, and Rust tests. Run `mise run //bindings/rust:test`.
2. Implement Python after Rust. Update the native wire value, Python value type,
   type stubs, map API, and package tests. Run
   `mise run //bindings/python:test`.
3. Implement Zig, including translated C declarations, owned copies, public
   values, and style-source tests. Run `mise run //bindings/zig:test`.
4. Implement Kotlin across common, JVM, Android, and native source sets.
   Regenerate JVM declarations, update native layouts and loaders, and exercise
   common public behavior on the configured host targets. Run
   `mise run //bindings/kotlin:test` and the targeted Android compile check.
5. Implement Swift support structs, public values, native calls, and tests. Run
   `mise run //bindings/swift:test`.
6. Regenerate .NET declarations, update public records and native conversion,
   and extend the public API surface and behavior tests. Run
   `mise run //bindings/dotnet:test`.
7. Implement Go C interop, copied values, map API, and tests. Run
   `mise run //bindings/go:test`.
8. Regenerate Dart FFI declarations, update public values and runtime accessors,
   and extend tests. Run `mise run //bindings/dart:test`.

After Rust is complete, Python remains sequential with Rust. Zig, Kotlin, Swift,
.NET, Go, and Dart may proceed in parallel agents in bounded waves. Each agent
owns one binding directory and reports generated-file changes and test results.
The primary agent integrates cross-binding naming and behavior.

## Completion

The work is complete when:

- the C API reports retained URL and parsed TileJSON state with documented
  presence semantics;
- C tests reconstruct representative URL-backed and inline tiled sources;
- the binding specification contains the copied-value and test requirements;
- all eight bindings expose equivalent source information through their public
  APIs;
- every affected binding test suite passes;
- generated declarations are reproducible;
- repository formatting and the relevant aggregate checks pass; and
- no native snapshot or list handle escapes a safe public binding API.
