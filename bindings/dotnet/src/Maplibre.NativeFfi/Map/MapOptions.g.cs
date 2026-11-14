// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for <c>RuntimeHandle.CreateMap()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_map_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record MapOptions
{
    /// <summary>
    /// The map's first size, and its scale factor. Render sessions of the map
    /// render at its scale factor. <c>MapHandle.ResizeAsync()</c> changes both,
    /// or <c>RenderSessionHandle.ResizeAsync()</c> while a render session is
    /// attached.
    /// </summary>
    public required LogicalExtent InitialExtent { get; init; }
    public MapMode MapMode { get; init; }

    /// <summary>
    /// Whether the map decodes MapLibre Tiles (MLT) that use FastPFOR integer
    /// encoding. Without it, such tiles fail to load.
    /// </summary>
    public bool FastPforEnabled { get; init; }

    /// <summary>
    /// The events that the map reports. Without it, the map reports them all.
    /// </summary>
    public RuntimeEventFlag? EventMask { get; init; }

    /// <summary>
    /// Whether the map keeps tiles in memory after they leave the view, so they
    /// show again without loading. Without it, the map keeps them.
    /// </summary>
    public bool? TileCacheEnabled { get; init; }

    /// <summary>
    /// Fonts for the text that a style's own fonts don't draw, such as scripts
    /// that need complex shaping, like Myanmar or Devanagari. A JSON array of
    /// font files, each an object with a <c>url</c> and an optional
    /// <c>unicode-range</c>, as in a font face of a style's <c>font-faces</c>,
    /// which every font stack uses after the style's own font faces. Empty for
    /// the system's fonts, unless <c>system_fonts_enabled</c> is false.
    /// </summary>
    public string FallbackFontFaces { get; init; } = "";

    /// <summary>
    /// Whether the map falls back to the system's fonts for CJK and the scripts
    /// that need complex shaping when <c>fallback_font_faces</c> is empty:
    /// fonts that the platform's font matcher picks for the user's preferred
    /// languages. Without it, the map does. The first map that uses them waits
    /// for them to be found. In the browser, the browser draws CJK glyphs in
    /// its sans-serif font instead, where <c>fallback_font_faces</c> doesn't
    /// cover them.
    /// </summary>
    public bool? SystemFontsEnabled { get; init; }
}
