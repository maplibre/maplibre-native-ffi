// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A camera view for converting between screen points and coordinates.
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
        state = new(handle, Abandon, nameof(MapProjectionHandle), Abandon);
    }

    internal static MapProjectionHandle Adopt(MlnMapProjection handle) =>
        NativeHandleState<MlnMapProjection>.Adopt(
            handle,
            () => new MapProjectionHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnMapProjection live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_map_projection_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnMapProjection> INativeOwner<MlnMapProjection>.State => state;
    internal MlnMapProjection Handle => state.Handle;

    /// <summary>
    /// The native handle value. It stays readable after release.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    void IDisposable.Dispose()
    {
        state.Close();
    }

    /// <summary>
    /// Creates a projection that converts coordinates for one view of a map.
    /// </summary>
    /// <remarks>
    /// <para>Create it from the <c>view</c> of an <c>MapSnapshot</c> or an
    /// <c>RenderFrameResult</c>. The projection doesn't follow the map, so
    /// create a new one after the map's camera, size, or style projection
    /// changes.</para>
    /// <para>See <c>mln_map_projection_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the view's style
    /// projection draws a globe and the library can't project
    /// one.</description></item>
    /// </list>
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
    /// Reads the projection's camera.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_get_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public MapProjectionCameraResult GetCamera()
    {
        using var call = Enter(this);
        var outResult = new mln_map_projection_camera_result
        {
            size = (uint)sizeof(mln_map_projection_camera_result),
        };
        Check(NativeMethods.mln_map_projection_get_camera(Handle, &outResult, Diagnostic));
        return CopyMapProjectionCameraResult(outResult);
    }

    /// <summary>
    /// Reports whether the globe hides a coordinate from the camera.
    /// </summary>
    /// <remarks>
    /// <para>Mercator hides nothing, so this reports false unless the view's
    /// style projection draws a globe.</para>
    /// <para>See <c>mln_map_projection_is_location_occluded</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.</para>
    /// </remarks>
    public bool IsLocationOccluded(LatLng coordinate)
    {
        using var call = Enter(this);
        bool outOccluded = default;
        Check(
            NativeMethods.mln_map_projection_is_location_occluded(
                Handle,
                NativeLatLng(coordinate),
                &outOccluded,
                Diagnostic
            )
        );
        return outOccluded;
    }

    /// <summary>
    /// Finds the coordinate at a screen point.
    /// </summary>
    /// <remarks>
    /// <para>The longitude stays between -180 and 180.</para>
    /// <para>See <c>mln_map_projection_lat_lng_for_pixel</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.</para>
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
    /// Finds the coordinate at a screen point, without wrapping its longitude.
    /// </summary>
    /// <remarks>
    /// <para>The longitude can fall outside -180 to 180, to show which copy of
    /// the world the point is in.</para>
    /// <para>See <c>mln_map_projection_lat_lng_for_pixel_unwrapped</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.</para>
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
    /// Finds the coordinates at several screen points, as
    /// <c>MapProjectionHandle.LatLngForPixel()</c> does.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_lat_lngs_for_pixels</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public LatLng[] LatLngsForPixels(ScreenPoint[] points)
    {
        using var scope = new NativeCallScope(this);
        var outCoordinates = scope.OutputArray<mln_lat_lng>(points.Length);
        Check(
            NativeMethods.mln_map_projection_lat_lngs_for_pixels(
                Handle,
                scope.Array<mln_screen_point, ScreenPoint>(
                    points,
                    static (item, scope) => NativeScreenPoint(item)
                ),
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
    /// Finds the coordinates at several screen points, as
    /// <c>MapProjectionHandle.LatLngForPixelUnwrapped()</c> does.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_lat_lngs_for_pixels_unwrapped</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public LatLng[] LatLngsForPixelsUnwrapped(ScreenPoint[] points)
    {
        using var scope = new NativeCallScope(this);
        var outCoordinates = scope.OutputArray<mln_lat_lng>(points.Length);
        Check(
            NativeMethods.mln_map_projection_lat_lngs_for_pixels_unwrapped(
                Handle,
                scope.Array<mln_screen_point, ScreenPoint>(
                    points,
                    static (item, scope) => NativeScreenPoint(item)
                ),
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
    /// Computes how many meters one screen pixel covers at <c>latitude</c>, at
    /// the projection's zoom.
    /// </summary>
    /// <remarks>
    /// <para>The result ignores the camera's pitch.</para>
    /// <para>See <c>mln_map_projection_meters_per_pixel_at_latitude</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.</para>
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
    /// Finds the screen point of a coordinate.
    /// </summary>
    /// <remarks>
    /// <para>The point is in the copy of the world nearest the camera's center.
    /// On a globe, a coordinate on the far side also gets a point;
    /// <c>MapProjectionHandle.IsLocationOccluded()</c> says whether the globe
    /// hides it.</para>
    /// <para>See <c>mln_map_projection_pixel_for_lat_lng</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.</para>
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
    /// Finds the screen points of several coordinates, as
    /// <c>MapProjectionHandle.PixelForLatLng()</c> does.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_pixels_for_lat_lngs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public ScreenPoint[] PixelsForLatLngs(LatLng[] coordinates)
    {
        using var scope = new NativeCallScope(this);
        var outPoints = scope.OutputArray<mln_screen_point>(coordinates.Length);
        Check(
            NativeMethods.mln_map_projection_pixels_for_lat_lngs(
                Handle,
                scope.Array<mln_lat_lng, LatLng>(
                    coordinates,
                    static (item, scope) => NativeLatLng(item)
                ),
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
    /// Releases a projection.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public void Release()
    {
        state.Close();
    }

    /// <summary>
    /// Moves the projection's camera, without changing the map.
    /// </summary>
    /// <remarks>
    /// <para>Camera fields that <c>camera</c> doesn't set keep their values.
    /// The style projection stays as the view had it, even at another
    /// zoom.</para>
    /// <para>See <c>mln_map_projection_set_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void SetCamera(CameraOptions camera)
    {
        using var call = Enter(this);
        var nativeCamera = NativeCameraOptions(camera);
        Check(NativeMethods.mln_map_projection_set_camera(Handle, &nativeCamera, Diagnostic));
    }
}
