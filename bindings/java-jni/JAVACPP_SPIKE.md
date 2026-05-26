# JavaCPP Spike Record

Audience: Java JNI binding contributors and reviewers. Category: explanation.

## Purpose

Evaluate JavaCPP as the generated C/JNI bridge over the MapLibre Native C ABI
before replacing the previous generator stack.

## Scope Covered

- Runtime create, run-once, event polling, and close through the public C ABI.
- Status conversion and same-thread diagnostic reads after failing C calls.
- Opaque handle ownership for runtime, map, projection, render session, texture,
  surface, offline operation, snapshot, result, and list handles.
- Struct and descriptor mapping for camera, map, render target, resource, style,
  query, offline, and JSON/value shapes.
- Callback and lifecycle-sensitive paths for process-global logging, runtime
  resource provider/transform callbacks, custom geometry source callbacks, and
  resource request completion.

## Result

JavaCPP met the spike requirements and was suitable for broad rewrite. The
binding now keeps generated JavaCPP declarations under
`org.maplibre.nativejni.internal.javacpp`, wraps them with small internal Java
adapters, and preserves the curated public `org.maplibre.nativejni` API surface.

## Safety Notes

- C++ exceptions and panics stay contained by the C ABI boundary.
- Java callback trampolines catch Java exceptions and errors before returning to
  C callback paths.
- Native snapshots, query/offline results, and ID lists are copied into Java
  values and destroyed in cleanup paths.
- Java-owned handles preserve close/retry behavior for fallible destruction.
- Native library tests cover classpath loading, configured path loading, exact
  `System.load(Path)` loading, and bad exact-path rejection.
- JavaCPP boundary tests build a small test-only JavaCPP bridge that invokes
  Java callbacks from a native-created thread and repeats callback upcalls to
  exercise JavaCPP thread attachment and reference scoping.

## Maintained Code Measurement

Run the reproducible measurement with:

```sh
python3 bindings/java-jni/tools/measure_maintained_loc.py --baseline 1d53075
```

Current measurement from this branch:

```text
baseline_revision=1d53075
baseline_loc=11082
current_loc=5701
reduction_loc=5381
reduction_percent=48.6
```

The count excludes generated JavaCPP declarations and build output. It includes
handwritten Java JNI internal adapters/support, the workspace marker crate, and
binding-local build glue.

## Follow-up Risks

JavaCPP-generated C++ currently emits compiler deprecation warnings for
JavaCPP's generated support code on macOS (`sprintf` and deprecated
`char_traits<unsigned short>`). These warnings do not fail the current build. If
the project later promotes generated-code warnings to errors, adjust the JavaCPP
version or generated compiler flags in the Java JNI build task.
