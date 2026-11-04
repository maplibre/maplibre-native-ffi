// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Computes the camera that shows exactly one tile on a square map.
    /// </summary>
    /// <remarks>
    /// <para>Jump a map in <c>MapMode.Tile</c> to the camera to render
    /// <c>tile</c>, where <c>logicalSize</c> is the map's size. The camera
    /// centers the tile, zooms until the tile fills the map, and looks straight
    /// down with north up and no padding. The map keeps its camera within its
    /// zoom limits, so for a deep tile, raise the maximum zoom past the
    /// camera's with <c>MapHandle.SetBoundsAsync()</c>.</para>
    /// <para>See <c>mln_camera_for_tile</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when <c>logicalSize</c>
    /// isn't square, or is wider than 512 pixels. For denser tiles, raise the
    /// map's scale factor instead.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when <c>tile.x</c> or
    /// <c>tile.y</c> isn't less than 2 to the power <c>tile.z</c>, or when the
    /// tile can't fill the map at a zoom from 0 to 25.5, as for tile 0/0/0 on a
    /// 256-pixel map.</description></item>
    /// </list>
    /// </remarks>
    public static CameraOptions CameraForTile(CanonicalTileId tile, LogicalSize logicalSize)
    {
        using var call = Enter(null);
        var outCamera = new mln_camera_options { size = (uint)sizeof(mln_camera_options) };
        Check(
            NativeMethods.mln_camera_for_tile(
                NativeCanonicalTileId(tile),
                NativeLogicalSize(logicalSize),
                &outCamera,
                Diagnostic
            )
        );
        return CopyCameraOptions(outCamera);
    }
}
