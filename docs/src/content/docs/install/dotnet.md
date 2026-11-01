---
title: .NET
description: Add the .NET package to a project.
---

## Install

Add the binding and one runtime from
[NuGet](https://www.nuget.org/packages/Maplibre.NativeFfi). It needs .NET 10 or
later. The runtime holds the native library for your
[backend](/maplibre-native-ffi/install/#choose-a-backend):
`Maplibre.NativeFfi.Runtime.OpenGL`, `.Metal`, or `.Vulkan`.

```bash frame="terminal"
dotnet add package Maplibre.NativeFfi
dotnet add package Maplibre.NativeFfi.Runtime.Vulkan
```

## Windows with MinGW

The runtimes above use MSVC on Windows. For MinGW, add
`Maplibre.NativeFfi.Runtime.OpenGL.MinGW` or
`Maplibre.NativeFfi.Runtime.Vulkan.MinGW` instead.

## Android

Initialize the library once per process, before you create a runtime, with the
calling thread's JNI environment and an Android `Context`:

```csharp title="MainActivity.cs"
MaplibreNative.InitAndroid(
    NativePointer.FromBorrowedAddress(jniEnv),
    NativePointer.FromBorrowedAddress(context));
```

## Snapshots

Download the packages from the
[snapshot release](https://github.com/maplibre/maplibre-native-ffi/releases/tag/unstable-native-snapshot)
into a directory, and add the directory as a package source:

```bash frame="terminal"
gh release download unstable-native-snapshot --repo maplibre/maplibre-native-ffi --pattern '*.nupkg' --dir packages
```

```xml title="nuget.config"
<configuration>
  <packageSources>
    <add key="maplibre-snapshot" value="packages" />
  </packageSources>
</configuration>
```

A snapshot is a prerelease of the next version, so add both packages with
`--prerelease`:

```bash frame="terminal"
dotnet add package Maplibre.NativeFfi --prerelease
dotnet add package Maplibre.NativeFfi.Runtime.Vulkan --prerelease
```
