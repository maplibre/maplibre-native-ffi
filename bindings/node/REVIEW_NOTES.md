# Review Findings

## Logged For Triage

- [ ] `node-required-seams`: several required failure and future-value seams are
      not yet explicit in Node tests
  - severity: medium
  - complexity: high
  - area: binding test suite
  - rationale: the suite covers broad real workflows, but does not yet expose
    deterministic seams for ABI mismatch before handle creation, unknown future
    status/event/payload values, native destroy failure, acquired snapshot copy
    failure, and every render-frame release failure path.
  - suggested next step: add internal Node or Rust test seams that exercise the
    same public error, cleanup, and copied-value behavior as the real native
    failure paths.

## Resolved

- `node-esm-conditions`: package exports rely on CommonJS through `default`
  - rationale: Node now has explicit `import` and `require` export conditions,
    ESM entrypoints, ESM declaration files, runtime smoke coverage, and
    TypeScript ESM import coverage.

## Invalidated

- `node-json-structured-semantics`: structured JSON and GeoJSON values use
  ordinary JavaScript values
  - rationale: the binding specification now allows language-native JSON value
    models when they are the idiomatic low-level representation; the Node API
    exposes JavaScript JSON values and keeps TypeScript types precise for the
    representable JavaScript model.

- `node-resource-provider-transfer`: handled resource requests cannot be
  completed from another worker
  - rationale: Node request handles are intentionally environment-local. This is
    spec-compliant for the current provider model; a transferable token becomes
    necessary only if the Node API claims cross-worker request completion. The
    private Node/Rust bridge now uses completion-token terminology so a future
    transferable capability can be added without weakening the local
    `ResourceRequestHandle`.

- `node-resource-provider-routing-dsl`: resource provider and transform route
  matchers add binding-level routing policy
  - rationale: the binding specification now allows declarative routing when it
    matches the host execution model. The Node route APIs preserve the same C
    pass-through, transform replacement, and handled-provider ownership
    decisions while fitting the single-threaded event-loop model.

- `node-abi-check-missing`: ABI mismatch is not checked before runtime handle
  storage
  - rationale: `create_native_runtime_handle()` materializes
    `core::runtime::NativeRuntimeOptions` before calling `mln_runtime_create()`,
    and `NativeRuntimeOptions::new()` calls `validate_abi_version()`, so ABI
    mismatch fails before a public runtime wrapper stores a native handle.

- `node-owner-thread-helper-required`: Node needs an owner-thread execution
  helper
  - rationale: ordinary Node calls keep stable native caller identity for a
    runtime/map lifecycle, and cross-thread calls surface the C wrong-thread
    status through the public error mapping; the binding does not currently ship
    a separate execution adapter.

- `node-raw-c-reproducibility`: raw C declarations are handwritten in the Node
  binding
  - rationale: the Node add-on uses the shared Rust `maplibre-native-sys`
    generated bindgen layer rather than handwritten Node-local C declarations.

- `node-empty-subpath-modules`: empty runtime subpaths are part of the supported
  package surface
  - rationale: the empty `camera`, `json`, `query`, and `style` subpaths were
    removed from package exports; type-only imports now come from the root or
    concept subpaths that expose runtime values.
