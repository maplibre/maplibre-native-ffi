// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Content-box insets in image pixels, measured from the image's top-left.
/// </summary>
/// <remarks>
/// See <c>mln_image_content</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ImageContent(
    float Left,
    float Top,
    float Right,
    float Bottom
);
