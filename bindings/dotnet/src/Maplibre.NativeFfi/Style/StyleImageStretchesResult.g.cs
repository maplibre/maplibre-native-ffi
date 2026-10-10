// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Borrowed image-stretch arrays available during a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_stretches_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct StyleImageStretchesResult
{
    public StyleImageStretchesResult(ImageStretch[] StretchX, ImageStretch[] StretchY)
    {
        this.StretchX = StretchX;
        this.StretchY = StretchY;
    }

    public ImageStretch[] StretchX
    {
        get => StretchXStorage.ToArray();
        init => StretchXStorage = ValueArray.Copy(value);
    }
    internal ValueArray<ImageStretch> StretchXStorage { get; init; }
    public ImageStretch[] StretchY
    {
        get => StretchYStorage.ToArray();
        init => StretchYStorage = ValueArray.Copy(value);
    }
    internal ValueArray<ImageStretch> StretchYStorage { get; init; }
}
