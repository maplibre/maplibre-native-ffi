// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public sealed unsafe partial class MapProjectionHandle : IDisposable, INativeOwner<MlnMapProjection>
{
    private readonly NativeHandleState<MlnMapProjection> state;

    internal MapProjectionHandle(MlnMapProjection handle)
    {
        state = new(
            handle,
            static (live, diagnostic) => NativeMethods.mln_map_projection_close(live, diagnostic),
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
        NativeMethods.mln_map_projection_close(live, diagnostic);

    NativeHandleState<MlnMapProjection> INativeOwner<MlnMapProjection>.State => state;
    internal MlnMapProjection Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_map_projection_close");
        state.Retire();
    }

    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_map_projection_close");
        state.Close();
    }

    public CameraOptions GetCamera()
    {
        using var read = state.Read(this, "mln_map_projection_get_camera");
        var outCamera = new mln_camera_options { size = (uint)sizeof(mln_camera_options) };
        Check(NativeMethods.mln_map_projection_get_camera(read.Handle, &outCamera, Diagnostic));
        return CopyCameraOptions(outCamera);
    }

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

    public void SetCamera(CameraOptions camera)
    {
        using var call = Enter(this, "mln_map_projection_set_camera");
        var nativeCamera = NativeCameraOptions(camera);
        Check(NativeMethods.mln_map_projection_set_camera(Handle, &nativeCamera, Diagnostic));
    }

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
