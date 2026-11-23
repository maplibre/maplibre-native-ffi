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
    /// Fonts for the text that a style's own fonts don't draw. A JSON array of
    /// font files, each an object with a <c>url</c> and an optional
    /// <c>unicode-range</c>, as in a font face of a style's <c>font-faces</c>.
    /// They shape, in order, the text that glyph PBFs can't shape, such as
    /// Myanmar or Devanagari, or all the text of a style without glyph PBFs,
    /// and draw the glyphs that come from local fonts: those a style's PBFs
    /// lack, and CJK with <c>local_ideographs_enabled</c>. Empty for the
    /// system's fonts alone.
    /// </summary>
    public string FallbackFontFaces { get; init; } = "";

    /// <summary>
    /// Whether the map draws from the system's fonts what the style's fonts and
    /// <c>fallback_font_faces</c> don't: fonts that the platform's font matcher
    /// picks for the font stack's names, or for the character and the user's
    /// preferred languages. It shapes with them the text that glyph PBFs can't
    /// shape, and all the text of a style without glyph PBFs. Without it, the
    /// map does. The first map that uses them waits for the matcher to load. In
    /// the browser, the browser draws those glyphs in the font stack's
    /// families, then its sans-serif font, one at a time.
    /// </summary>
    public bool? SystemFontsEnabled { get; init; }

    /// <summary>
    /// Whether CJK ideographs, kana and hangul come from local fonts
    /// (<c>fallback_font_faces</c>, then the system's) before glyph PBFs,
    /// rather than from a glyph PBF downloaded for every few characters.
    /// Without it, they do.
    /// </summary>
    public bool? LocalIdeographsEnabled { get; init; }
}
