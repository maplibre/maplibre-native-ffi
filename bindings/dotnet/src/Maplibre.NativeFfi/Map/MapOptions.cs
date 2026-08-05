using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Map;

/// <summary>Map creation options.</summary>
/// <remarks>
/// Compares and hashes by property value; keep an instance unmodified while it is a key in a
/// hash-based collection.
/// </remarks>
public sealed record MapOptions
{
    /// <summary>
    /// Initial logical width in pixels, replaced by the extent of the first attached render
    /// session.
    /// </summary>
    public uint? Width { get; set; }

    /// <summary>
    /// Initial logical height in pixels, replaced by the extent of the first attached render
    /// session.
    /// </summary>
    public uint? Height { get; set; }

    /// <summary>
    /// Device scale factor, fixed for the lifetime of the map, that selects sprites, glyphs, and
    /// raster tiles. Render targets carry their own scale factor for geometry, so attaching or
    /// resizing a session with a different one logs a warning and renders imagery chosen for this
    /// density.
    /// </summary>
    public double? ScaleFactor { get; set; }

    /// <summary>Map rendering mode.</summary>
    public MapMode? MapMode { get; set; }

    /// <summary>
    /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR encodings, fixed for
    /// the lifetime of the map. A map created with this <c>false</c> decodes every other MLT
    /// encoding and logs a tile parse warning for the FastPFOR ones.
    /// </summary>
    public bool? FastPforEnabled { get; set; }

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
        if (FastPforEnabled is { } fastPforEnabled)
        {
            options.fast_pfor_enabled = fastPforEnabled ? (byte)1 : (byte)0;
        }
        return options;
    }
}
