---
title: "C# Binding Notes"
description: Language-specific implementation notes for C# bindings.
---

Tracking issue:
[Add C# bindings](https://github.com/maplibre/maplibre-native-ffi/issues/48).

The C# binding targets `net10.0` and exposes a low-level .NET API while keeping
source-generated native imports private. The .NET 10 LTS baseline keeps modern
interop, trimming, and NativeAOT behavior in scope without carrying older
runtime-specific paths.

Package and namespace names:

```text
MapLibre.Native          NuGet package
MapLibre.Native          public namespace
MapLibre.Native.Interop  internal native import layer
```

Use source-generated `LibraryImport` declarations in an internal partial native
class. Keep signatures blittable and explicit, with `unsafe` pointer signatures
for opaque handles, structs, string views, out parameters, and callback thunks.
Use `DllImport` only for signatures the source generator cannot express.

Public APIs expose `RuntimeHandle`, `MapHandle`, `MapProjectionHandle`, and
`RenderSessionHandle` classes with explicit `Close()` or `Destroy()` methods.
These methods translate native status and same-thread diagnostics into .NET
exceptions. `IDisposable` is useful for deterministic user cleanup, but
finalizers and `SafeHandle` release paths are not the primary owner-thread
destruction mechanism.

Status-returning calls throw a stable `MapLibreNativeException` hierarchy.
Preserve status categories such as invalid argument, invalid state, wrong
thread, unsupported, and native error in public exception data.

Callbacks use static unmanaged thunks, preferably `UnmanagedCallersOnly`, and
binding-owned callback state stored through `GCHandle` or a registry. Callback
thunks catch managed exceptions and convert them to the documented C callback
behavior. Callback state stays live for the native registration scope and is
safe for calls from MapLibre worker, network, logging, or render-related
threads.

Use `Span<T>`, `ReadOnlySpan<T>`, `NativeMemory`, and scoped unsafe blocks for
temporary ABI storage. Reject embedded `NUL` for null-terminated strings, encode
strings as UTF-8, and copy borrowed native output before its borrow window ends.

Represent backend handles with an immutable `NativePointer` value around
`IntPtr`. It transfers no ownership and exposes no memory access.

Publish RID-specific NuGet native assets under `runtimes/{rid}/native/`, with
managed assemblies under `lib` or `ref` for `net10.0`. Keep package layout
compatible with JIT and NativeAOT consumers.
