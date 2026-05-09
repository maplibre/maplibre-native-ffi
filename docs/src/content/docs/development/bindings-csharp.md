---
title: "C# Binding Conventions"
description: Language-specific implementation conventions for C# bindings.
---

Resources:

- Tracking issue:
  [#48](https://github.com/maplibre/maplibre-native-ffi/issues/48)
- [.NET native interoperability](https://learn.microsoft.com/en-us/dotnet/standard/native-interop/)
- [Source-generated P/Invokes](https://learn.microsoft.com/en-us/dotnet/standard/native-interop/pinvoke-source-generation)
- [ClangSharp](https://github.com/dotnet/ClangSharp)

The C# binding targets `net10.0` and exposes a low-level .NET API over a
generated native import layer.

The .NET 10 LTS baseline gives the binding current source-generated interop
tooling, improved code generation for small structs and temporary adapter
values, and a clear NativeAOT and trimming compatibility target.

Generate the internal native import layer from the public C headers with
ClangSharp. Treat successful generation and compilation as the header
bindability check for this path. Verbose generated declarations are fine because
they stay private; the public C# layer is handwritten.

Public handle types are classes. They implement `IDisposable` for deterministic
cleanup and expose explicit `Close()` or `Destroy()` methods that report native
status through exceptions. Use finalizers and `SafeHandle` release paths for
leak reporting. Use them for cleanup only for native resources whose release
function is documented as thread-independent and infallible.

Callbacks use static unmanaged thunks with binding-owned callback state stored
through `GCHandle` or a registry. Use `UnmanagedCallersOnly` for C-callable
static methods when the signature supports it. Callback state stays live for the
native registration scope and is safe for calls from MapLibre worker, network,
logging, or render-related threads.

Use `Span<T>`, `ReadOnlySpan<T>`, `NativeMemory`, and scoped unsafe blocks for
temporary ABI storage. Represent backend handles with an immutable
`NativePointer` value around `IntPtr`.

Publish RID-specific NuGet native assets under `runtimes/{rid}/native/`, with
managed assemblies under `lib` or `ref` for `net10.0`. Keep package layout
compatible with JIT and NativeAOT consumers.
