using Maplibre.Native.Internal.C;

namespace Maplibre.Native.Map;

/// <summary>Map creation options.</summary>
/// <remarks>
/// Compares and hashes by property value; <c>with</c> returns an independent instance. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record MapOptions
{
    /// <summary>Initial logical width in pixels.</summary>
    public uint? Width { get; set; }

    /// <summary>Initial logical height in pixels.</summary>
    public uint? Height { get; set; }

    /// <summary>Device scale factor.</summary>
    public double? ScaleFactor { get; set; }

    /// <summary>Map rendering mode.</summary>
    public MapMode? MapMode { get; set; }

    internal mln_map_options ToNative()
    {
        var options = NativeMethods.mln_map_options_default();
        if (Width is { } width)
        {
            options.width = width;
        }
        if (Height is { } height)
        {
            options.height = height;
        }
        if (ScaleFactor is { } scaleFactor)
        {
            options.scale_factor = scaleFactor;
        }
        if (MapMode is { } mapMode)
        {
            options.map_mode = (uint)mapMode;
        }
        return options;
    }
}
