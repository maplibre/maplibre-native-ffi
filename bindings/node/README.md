# MapLibre Native FFI for Node.js

Low-level Node.js bindings for the MapLibre Native C FFI.

The package preserves the C API concepts: runtimes, maps, projections, render
sessions, resource callbacks, offline operations, events, and explicit handle
release. Public code uses the JavaScript wrapper API exported by the package
root and concept subpaths. The generated native add-on entrypoint remains an
internal support layer for the wrapper and tests.

## JSON and GeoJSON values

Structured JSON and GeoJSON APIs use JavaScript's native JSON value model:
`null`, booleans, numbers, strings, arrays, and plain objects. This is the
idiomatic low-level representation for Node callers, and it has the same
semantic limits as JavaScript JSON:

- Object member order follows JavaScript property order after `JSON.stringify`
  and `JSON.parse`.
- Repeated object member names cannot be represented in a JavaScript object.
- Numeric values use JavaScript `number`, so integer precision is limited to the
  safe integer range.

Use `MapHandle.setStyleJson(json: string)` when a style document must pass
through as raw JSON text without wrapper parsing or reformatting.
