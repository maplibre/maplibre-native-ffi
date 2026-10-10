// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_style_image_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_option_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum StyleImageOptionField : uint
{
    PixelRatio = 1,
    Sdf = 2,
    StretchX = 4,
    StretchY = 8,
    Content = 16,
    TextFitWidth = 32,
    TextFitHeight = 64,
}
