// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The frame that a custom layer draws into, as its callbacks get it.
/// </summary>
/// <remarks>
/// <para>The view and the frame projection are the ones that the frame draws
/// the map with, so they are never newer than the frame, unlike the map's
/// camera.</para>
/// <para>See <c>mln_custom_layer_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="View">
/// The camera, extent, and scale factor that the frame shows.
/// </param>
/// <param name="FrameProjection">
/// Where the frame draws the map, and the depth that it writes there.
/// </param>
/// <param name="Width">
/// The width of the frame's attachments, in physical pixels.
/// </param>
/// <param name="Height">
/// The height of the frame's attachments, in physical pixels.
/// </param>
public readonly partial record struct CustomLayerFrame(
    MapView View,
    FrameProjection FrameProjection,
    uint Width,
    uint Height
);
