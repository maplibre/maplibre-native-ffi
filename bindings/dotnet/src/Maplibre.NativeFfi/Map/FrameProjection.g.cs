// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Where a frame drew the map, and the depth that it wrote there.
/// </summary>
/// <remarks>
/// <para>World coordinates are Web Mercator's. x runs from 0 at 180° west to 1
/// at 180° east, and on past both into the copies of the world beside the
/// first. y runs from 0 at the north edge of the Mercator world, about 85.05°
/// north, to 1 at its south edge. z is the altitude in meters above sea level,
/// at the scale of the heights of the map's 3D layers, and the map draws its
/// ground at 0.</para>
/// <para><c>matrix</c> takes a world point to the clip coordinates where the
/// frame drew it, with the camera's padding, roll, field of view, and center
/// altitude, and the map's viewport and projection mode. Clip y points up, as
/// the map draws with OpenGL, Metal, and WebGPU. The map negates clip y with
/// Vulkan, so negate it there too. At high zooms, single precision can't tell
/// nearby world points apart: subtract an origin near your geometry from its
/// coordinates in double precision, and fold the origin into the matrix, before
/// you convert to float.</para>
/// <para>Clip z follows <c>clip_depth_range</c>, the convention of the device's
/// graphics API. With the API's default depth range of 0 to 1, a fragment that
/// you draw through <c>matrix</c> gets the depth that the map's 3D layers, such
/// as fill extrusions, wrote at the same place: 0 at <c>near_distance</c> from
/// the camera, up to <c>far_depth</c> at <c>far_distance</c>. Depth-tested
/// against the frame's depth, your fragments hide behind the map's 3D layers
/// where those stand in front of them.</para>
/// <para>The frame's depth starts at 1. The map's 3D layers write the depth of
/// each of their fragments. A 2D layer below every 3D layer of the style that
/// draws opaque fragments, such as an opaque fill, writes one depth over all of
/// them, wherever they are: the higher the layer, the lower its depth, and
/// every one between <c>far_depth</c> and 1. Backgrounds, translucent
/// fragments, and the 2D layers above a 3D layer, such as labels, write no
/// depth. So what you draw through <c>matrix</c>, even on the ground, lies in
/// front of every 2D layer.</para>
/// <para>While <c>globe</c> is true, the frame drew the map onto a globe, and
/// its 2D layers wrote no depth. <c>matrix</c> then is the map's Mercator one,
/// which doesn't place points on the globe.</para>
/// <para>See <c>mln_frame_projection</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="NearDistance">
/// The distance from the camera to the plane where the depth that you get
/// through <c>matrix</c> is 0, along the camera's view. Like
/// <c>far_distance</c>, it is in pixels of the map at the camera's zoom, where
/// the world is 512 × 2^zoom pixels wide.
/// </param>
/// <param name="FarDistance">
/// The distance to the plane where the depth is <c>far_depth</c>.
/// </param>
/// <param name="FarDepth">
/// The depth at <c>far_distance</c>, below 1.
/// </param>
/// <param name="Globe">
/// Whether the frame drew the map as a globe, alone or blending into Mercator.
/// </param>
public readonly partial record struct FrameProjection(
    Mat4 Matrix,
    ClipDepthRange ClipDepthRange,
    double NearDistance,
    double FarDistance,
    double FarDepth,
    bool Globe
);
