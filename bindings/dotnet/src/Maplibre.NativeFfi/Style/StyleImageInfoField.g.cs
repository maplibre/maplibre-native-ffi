// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Fields available in <c>mln_style_image_info</c>.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_info_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum StyleImageInfoField : uint
{
    /// <summary>
    /// The image sets a content box.
    /// </summary>
    Content = 1,

    /// <summary>
    /// The image sets how it fits text horizontally.
    /// </summary>
    TextFitWidth = 2,

    /// <summary>
    /// The image sets how it fits text vertically.
    /// </summary>
    TextFitHeight = 4,
}
