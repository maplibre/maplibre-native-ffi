// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A standalone projection of one map view.
/// </summary>
/// <remarks>
/// See <c>mln_map_projection</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class MapProjectionHandle : IDisposable, INativeOwner<MlnMapProjection>
{
    private readonly NativeHandleState<MlnMapProjection> state;

    internal MapProjectionHandle(MlnMapProjection handle)
    {
        state = new(
            handle,
            static (live, diagnostic) => NativeMethods.mln_map_projection_release(live, diagnostic),
            nameof(MapProjectionHandle),
            Abandon
        );
    }

    internal static MapProjectionHandle Adopt(MlnMapProjection handle) =>
        NativeHandleState<MlnMapProjection>.Adopt(
            handle,
            () => new MapProjectionHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnMapProjection live, mln_diagnostic* diagnostic) =>
        NativeMethods.mln_map_projection_release(live, diagnostic);

    NativeHandleState<MlnMapProjection> INativeOwner<MlnMapProjection>.State => state;
    internal MlnMapProjection Handle => state.Handle;

    // Runtime events report their source by this identity.
    public ulong Id => state.Handle.Value;

    public void Dispose()
    {
        state.Retire();
    }

    /// <summary>
    /// Creates a standalone projection of a view, such as the view of a
    /// rendered frame or of a map snapshot.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public static MapProjectionHandle Create(MapView view)
    {
        using var call = Enter(null);
        var nativeView = NativeMapView(view);
        MlnMapProjection outProjection = default;
        Check(NativeMethods.mln_map_projection_create(&nativeView, &outProjection, Diagnostic));
        return MapProjectionHandle.Adopt(outProjection);
    }

    /// <summary>
    /// Copies the projection camera into out_camera.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_get_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public CameraOptions GetCamera()
    {
        using var call = Enter(this);
        var outCamera = new mln_camera_options { size = (uint)sizeof(mln_camera_options) };
        Check(NativeMethods.mln_map_projection_get_camera(Handle, &outCamera, Diagnostic));
        return CopyCameraOptions(outCamera);
    }

    /// <summary>
    /// Converts a screen point to a geographic coordinate.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_lat_lng_for_pixel</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        using var call = Enter(this);
        var outCoordinate = default(mln_lat_lng);
        Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel(
                Handle,
                NativeScreenPoint(point),
                &outCoordinate,
                Diagnostic
            )
        );
        return CopyLatLng(outCoordinate);
    }

    /// <summary>
    /// Converts a screen point to an unwrapped geographic coordinate.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_lat_lng_for_pixel_unwrapped</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public LatLng LatLngForPixelUnwrapped(ScreenPoint point)
    {
        using var call = Enter(this);
        var outCoordinate = default(mln_lat_lng);
        Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel_unwrapped(
                Handle,
                NativeScreenPoint(point),
                &outCoordinate,
                Diagnostic
            )
        );
        return CopyLatLng(outCoordinate);
    }

    /// <summary>
    /// Reads the ground distance covered by one logical map pixel at a latitude
    /// for the helper camera zoom.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_meters_per_pixel_at_latitude</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public double MetersPerPixelAtLatitude(double latitude)
    {
        using var call = Enter(this);
        double outMetersPerPixel = default;
        Check(
            NativeMethods.mln_map_projection_meters_per_pixel_at_latitude(
                Handle,
                latitude,
                &outMetersPerPixel,
                Diagnostic
            )
        );
        return outMetersPerPixel;
    }

    /// <summary>
    /// Converts a geographic coordinate to a screen point.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_pixel_for_lat_lng</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public ScreenPoint PixelForLatLng(LatLng coordinate)
    {
        using var call = Enter(this);
        var outPoint = default(mln_screen_point);
        Check(
            NativeMethods.mln_map_projection_pixel_for_lat_lng(
                Handle,
                NativeLatLng(coordinate),
                &outPoint,
                Diagnostic
            )
        );
        return CopyScreenPoint(outPoint);
    }

    /// <summary>
    /// Releases a standalone projection.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public void Close()
    {
        state.Close();
    }

    /// <summary>
    /// Applies a camera update to a standalone projection.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_set_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public void SetCamera(CameraOptions camera)
    {
        using var call = Enter(this);
        var nativeCamera = NativeCameraOptions(camera);
        Check(NativeMethods.mln_map_projection_set_camera(Handle, &nativeCamera, Diagnostic));
    }
}
