using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;

namespace Maplibre.NativeFfi.Runtime;

/// <summary>Runtime creation options.</summary>
/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record RuntimeOptions
{
    /// <summary>Filesystem root for <c>asset://</c> URLs.</summary>
    public string? AssetPath { get; set; }

    /// <summary>Cache database path.</summary>
    public string? CachePath { get; set; }

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
    }

    internal mln_runtime_options Value;

    public void Dispose()
    {
        assetPath.Dispose();
        cachePath.Dispose();
    }
}
