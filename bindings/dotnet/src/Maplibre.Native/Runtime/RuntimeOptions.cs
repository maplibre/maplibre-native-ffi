using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Memory;

namespace Maplibre.Native.Runtime;

/// <summary>Runtime creation options.</summary>
public sealed class RuntimeOptions
{
    /// <summary>Filesystem root for <c>asset://</c> URLs.</summary>
    public string? AssetPath { get; set; }

    /// <summary>Cache database path.</summary>
    public string? CachePath { get; set; }

    /// <summary>Maximum ambient cache size in bytes.</summary>
    public ulong? MaximumCacheSize { get; set; }

    internal unsafe NativeRuntimeOptions ToNative()
    {
        return new NativeRuntimeOptions(this);
    }
}

internal sealed unsafe class NativeRuntimeOptions : IDisposable
{
    private const uint MaximumCacheSizeFlag = 1u << 0;
    private readonly NativeUtf8String assetPath;
    private readonly NativeUtf8String cachePath;

    internal NativeRuntimeOptions(RuntimeOptions options)
    {
        assetPath = NativeUtf8String.FromNullableString(options.AssetPath, nameof(options.AssetPath));
        cachePath = NativeUtf8String.FromNullableString(options.CachePath, nameof(options.CachePath));
        Value = NativeMethods.mln_runtime_options_default();
        Value.asset_path = assetPath.Pointer;
        Value.cache_path = cachePath.Pointer;
        if (options.MaximumCacheSize is { } maximumCacheSize)
        {
            Value.flags |= MaximumCacheSizeFlag;
            Value.maximum_cache_size = maximumCacheSize;
        }
    }

    internal mln_runtime_options Value;

    public void Dispose()
    {
        assetPath.Dispose();
        cachePath.Dispose();
    }
}
