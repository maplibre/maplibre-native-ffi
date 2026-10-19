// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A standalone projection of a map's transform state at its creation.
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
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_map_projection_release");
        state.Retire();
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
        using var read = state.Read(this, "mln_map_projection_get_camera");
        var outCamera = new mln_camera_options { size = (uint)sizeof(mln_camera_options) };
        Check(NativeMethods.mln_map_projection_get_camera(read.Handle, &outCamera, Diagnostic));
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
        using var read = state.Read(this, "mln_map_projection_lat_lng_for_pixel");
        var outCoordinate = default(mln_lat_lng);
        Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel(
                read.Handle,
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
        using var read = state.Read(this, "mln_map_projection_lat_lng_for_pixel_unwrapped");
        var outCoordinate = default(mln_lat_lng);
        Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel_unwrapped(
                read.Handle,
                NativeScreenPoint(point),
                &outCoordinate,
                Diagnostic
            )
        );
        return CopyLatLng(outCoordinate);
    }

    /// <summary>
    /// Converts screen points to geographic coordinates.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_lat_lngs_for_pixels</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public LatLng[] LatLngsForPixels(ScreenPoint[] points)
    {
        using var read = state.Read(this, "mln_map_projection_lat_lngs_for_pixels");
        using var scope = new NativeCallScope();
        var outCoordinates = scope.OutputArray<mln_lat_lng>(points.Length);
        Check(
            NativeMethods.mln_map_projection_lat_lngs_for_pixels(
                read.Handle,
                scope.Array<mln_screen_point, ScreenPoint>(points, item => NativeScreenPoint(item)),
                checked((nuint)points.Length),
                outCoordinates,
                Diagnostic
            )
        );
        scope.Accept();
        return NativeCallScope.CopyArray<mln_lat_lng, LatLng>(
            outCoordinates,
            (nuint)(points.Length),
            static item => CopyLatLng(item)
        );
    }

    /// <summary>
    /// Converts screen points to unwrapped geographic coordinates.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_lat_lngs_for_pixels_unwrapped</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public LatLng[] LatLngsForPixelsUnwrapped(ScreenPoint[] points)
    {
        using var read = state.Read(this, "mln_map_projection_lat_lngs_for_pixels_unwrapped");
        using var scope = new NativeCallScope();
        var outCoordinates = scope.OutputArray<mln_lat_lng>(points.Length);
        Check(
            NativeMethods.mln_map_projection_lat_lngs_for_pixels_unwrapped(
                read.Handle,
                scope.Array<mln_screen_point, ScreenPoint>(points, item => NativeScreenPoint(item)),
                checked((nuint)points.Length),
                outCoordinates,
                Diagnostic
            )
        );
        scope.Accept();
        return NativeCallScope.CopyArray<mln_lat_lng, LatLng>(
            outCoordinates,
            (nuint)(points.Length),
            static item => CopyLatLng(item)
        );
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
        using var read = state.Read(this, "mln_map_projection_meters_per_pixel_at_latitude");
        double outMetersPerPixel = default;
        Check(
            NativeMethods.mln_map_projection_meters_per_pixel_at_latitude(
                read.Handle,
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
        using var read = state.Read(this, "mln_map_projection_pixel_for_lat_lng");
        var outPoint = default(mln_screen_point);
        Check(
            NativeMethods.mln_map_projection_pixel_for_lat_lng(
                read.Handle,
                NativeLatLng(coordinate),
                &outPoint,
                Diagnostic
            )
        );
        return CopyScreenPoint(outPoint);
    }

    /// <summary>
    /// Converts geographic coordinates to screen points.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_pixels_for_lat_lngs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public ScreenPoint[] PixelsForLatLngs(LatLng[] coordinates)
    {
        using var read = state.Read(this, "mln_map_projection_pixels_for_lat_lngs");
        using var scope = new NativeCallScope();
        var outPoints = scope.OutputArray<mln_screen_point>(coordinates.Length);
        Check(
            NativeMethods.mln_map_projection_pixels_for_lat_lngs(
                read.Handle,
                scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                checked((nuint)coordinates.Length),
                outPoints,
                Diagnostic
            )
        );
        scope.Accept();
        return NativeCallScope.CopyArray<mln_screen_point, ScreenPoint>(
            outPoints,
            (nuint)(coordinates.Length),
            static item => CopyScreenPoint(item)
        );
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
        NativeCallbackGuard.EnsureAllowed(this, "mln_map_projection_release");
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
        using var call = Enter(this, "mln_map_projection_set_camera");
        var nativeCamera = NativeCameraOptions(camera);
        Check(NativeMethods.mln_map_projection_set_camera(Handle, &nativeCamera, Diagnostic));
    }

    /// <summary>
    /// Applies a camera fit for geographic coordinates.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_set_visible_coordinates</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public void SetVisibleCoordinates(LatLng[] coordinates, EdgeInsets padding)
    {
        using var scope = new NativeCallScope(this, "mln_map_projection_set_visible_coordinates");
        Check(
            NativeMethods.mln_map_projection_set_visible_coordinates(
                Handle,
                scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                checked((nuint)coordinates.Length),
                NativeEdgeInsets(padding),
                Diagnostic
            )
        );
        scope.Accept();
    }

    /// <summary>
    /// Applies a camera fit for GeoJSON Geometry bytes.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_set_visible_geometry</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public void SetVisibleGeometry(byte[] geometry, EdgeInsets padding)
    {
        using var scope = new NativeCallScope(this, "mln_map_projection_set_visible_geometry");
        Check(
            NativeMethods.mln_map_projection_set_visible_geometry(
                Handle,
                scope.Buffer(geometry),
                NativeEdgeInsets(padding),
                Diagnostic
            )
        );
        scope.Accept();
    }
}
