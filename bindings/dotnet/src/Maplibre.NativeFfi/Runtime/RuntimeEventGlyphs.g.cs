// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The details of a glyph event.
/// </summary>
/// <remarks>
/// <para>The event covers the glyphs from <c>first</c> to <c>last</c>, both
/// included.</para>
/// <para>See <c>mln_runtime_event_glyphs</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
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
    /// The fonts that the glyphs come from, in order of preference.
    /// </summary>
    public ImmutableArray<string> FontStack
    {
        get => FontStackStorage.AsImmutable();
        init => FontStackStorage = ValueArray.From(value);
    }
    internal ValueArray<string> FontStackStorage { get; init; }
    public uint First { get; init; }
    public uint Last { get; init; }
    public bool FontFace { get; init; }
}
