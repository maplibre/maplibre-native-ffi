// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL attachment options for a borrowed texture target.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct OpenglBorrowedTextureDescriptor
{
    public OpenglBorrowedTextureDescriptor()
        : this(new RenderTargetExtent(), 256, 256, default, default!, default) { }

    public OpenglBorrowedTextureDescriptor(
        RenderTargetExtent Extent,
        uint PhysicalWidth,
        uint PhysicalHeight,
        OpenglContextDescriptor Context,
        OpenglBorrowedTexture[] Textures,
        uint Target
    )
    {
        this.Extent = Extent;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Context = Context;
        this.Textures = Textures;
        this.Target = Target;
    }

    public RenderTargetExtent Extent { get; init; }
    public uint PhysicalWidth { get; init; }
    public uint PhysicalHeight { get; init; }
    public OpenglContextDescriptor Context { get; init; }
    public OpenglBorrowedTexture[] Textures
    {
        get => TexturesStorage.ToArray();
        init => TexturesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<OpenglBorrowedTexture> TexturesStorage { get; init; }
    public uint Target { get; init; }
    public static OpenglBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(
                null,
                "mln_opengl_borrowed_texture_descriptor_default"
            );
            return GeneratedValues.CopyOpenglBorrowedTextureDescriptor(
                NativeMethods.mln_opengl_borrowed_texture_descriptor_default()
            );
        }
    }
}
