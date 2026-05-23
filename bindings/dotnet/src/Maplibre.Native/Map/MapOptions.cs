using Maplibre.Native.Internal.C;

namespace Maplibre.Native.Map;

/// <summary>Map creation options.</summary>
public sealed class MapOptions
{
    /// <summary>Initial logical width in pixels.</summary>
    public uint Width { get; set; }

    /// <summary>Initial logical height in pixels.</summary>
    public uint Height { get; set; }

    /// <summary>Device scale factor.</summary>
    public double ScaleFactor { get; set; } = 1.0;

    /// <summary>Map rendering mode.</summary>
    public MapMode MapMode { get; set; } = MapMode.Continuous;

    internal mln_map_options ToNative()
    {
        var options = NativeMethods.mln_map_options_default();
        options.width = Width;
        options.height = Height;
        options.scale_factor = ScaleFactor;
        options.map_mode = (uint)MapMode;
        return options;
    }
}
