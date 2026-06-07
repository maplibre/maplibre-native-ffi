using Maplibre.Native.Internal.C;

namespace Maplibre.Native.Map;

/// <summary>Map creation options.</summary>
public sealed class MapOptions
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
