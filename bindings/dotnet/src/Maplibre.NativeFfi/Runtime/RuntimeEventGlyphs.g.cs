// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload for <c>RuntimeEventType.MapGlyphsRequested</c>,
/// <c>RuntimeEventType.MapGlyphsLoaded</c>, and
/// <c>RuntimeEventType.MapGlyphsError</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_glyphs</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RuntimeEventGlyphs
{
    public RuntimeEventGlyphs(
        ImmutableArray<string> FontStack,
        uint First,
        uint Last,
        bool FontFace
    )
    {
        this.FontStack = FontStack;
        this.First = First;
        this.Last = Last;
        this.FontFace = FontFace;
    }

    /// <summary>
    /// Font names of the font stack, in the style's order.
    /// </summary>
    public ImmutableArray<string> FontStack
    {
        get => FontStackStorage.AsImmutable();
        init => FontStackStorage = ValueArray.From(value);
    }
    internal ValueArray<string> FontStackStorage { get; init; }

    /// <summary>
    /// First glyph of the range.
    /// </summary>
    public uint First { get; init; }

    /// <summary>
    /// Last glyph of the range, inclusive.
    /// </summary>
    public uint Last { get; init; }

    /// <summary>
    /// Whether the range stands for a whole font file that the style's
    /// font-faces names. MapLibre then loads the whole file, and first and last
    /// do not describe the glyphs it holds.
    /// </summary>
    public bool FontFace { get; init; }
}
