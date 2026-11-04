// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// New OpenGL textures for a borrowed ring, for
/// <c>RenderSessionHandle.SetOpenGlBorrowedTextureTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para>The members mean what they mean in
/// <c>OpenGlBorrowedTextureDescriptor</c>, and the textures follow its rules.
/// The ring keeps its depth, context, and texture target, so give it as many
/// textures as it attached with, in the same share group. The session uses the
/// textures until a replacement or release of the target completes, a detach or
/// release of the session completes, or an abandon of it returns.</para>
/// <para>See <c>mln_open_gl_borrowed_texture_replacement</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct OpenGlBorrowedTextureReplacement
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenGlBorrowedTextureReplacement(
        LogicalSize LogicalSize,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<OpenGlBorrowedTexture> Textures
    )
    {
        this.LogicalSize = LogicalSize;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
    }

    public required LogicalSize LogicalSize { get; init; }
    public required uint PhysicalWidth { get; init; }
    public required uint PhysicalHeight { get; init; }
    public required ImmutableArray<OpenGlBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<OpenGlBorrowedTexture> TexturesStorage { get; init; }
}
