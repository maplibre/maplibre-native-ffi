using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Internal.Memory;

namespace Maplibre.NativeFfi.Runtime;

/// <summary>Runtime creation options.</summary>
/// <remarks>
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection. Constructing an instance reads <see cref="EventMask" /> from the native
/// library, so a host that loads the library from an exact path does that first.
/// </remarks>
public sealed record RuntimeOptions
{
    /// <summary>Filesystem root for <c>asset://</c> URLs.</summary>
    public string? AssetPath { get; set; }

    /// <summary>Cache database path.</summary>
    public string? CachePath { get; set; }

    /// <summary>
    /// Runtime-originated event types this runtime queues, every event type this library
    /// reports by default. See <see cref="RuntimeHandle.SetEventMask" />.
    /// </summary>
    public RuntimeEventMask EventMask { get; set; } = DefaultEventMask();

    // Read from the C default rather than named, so a newer native library's default keeps
    // selecting event types this build does not declare. Those reach a host as unknown event
    // and payload domains.
    private static RuntimeEventMask DefaultEventMask()
    {
        NativeLibraryLoader.EnsureLoaded();
        return (RuntimeEventMask)NativeMethods.mln_runtime_options_default().event_mask;
    }

    internal unsafe NativeRuntimeOptions ToNative()
    {
        return new NativeRuntimeOptions(this);
    }
}

internal sealed unsafe class NativeRuntimeOptions : IDisposable
{
    private readonly NativeUtf8String assetPath;
    private readonly NativeUtf8String cachePath;

    internal NativeRuntimeOptions(RuntimeOptions options)
    {
        assetPath = NativeUtf8String.FromNullableString(
            options.AssetPath,
            nameof(options.AssetPath)
        );
        cachePath = NativeUtf8String.FromNullableString(
            options.CachePath,
            nameof(options.CachePath)
        );
        Value = NativeMethods.mln_runtime_options_default();
        Value.asset_path = assetPath.Pointer;
        Value.cache_path = cachePath.Pointer;
        Value.event_mask = (ulong)options.EventMask;
    }

    internal mln_runtime_options Value;

    public void Dispose()
    {
        assetPath.Dispose();
        cachePath.Dispose();
    }
}
