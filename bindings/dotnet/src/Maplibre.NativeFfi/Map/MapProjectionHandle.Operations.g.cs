// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi.Map;

public sealed unsafe partial class MapProjectionHandle : IDisposable
{
    private readonly NativeHandleState<MlnMapProjection> state;
    private readonly ulong nativeId;
    public ulong Id => nativeId;

    internal MapProjectionHandle(MlnMapProjection handle)
    {
        nativeId = handle.Value;
        state = new NativeHandleState<MlnMapProjection>(
            handle,
            static (live, diagnostic) => NativeMethods.mln_map_projection_close(live, diagnostic),
            nameof(MapProjectionHandle),
            static (live, diagnostic) => NativeMethods.mln_map_projection_close(live, diagnostic)
        );
    }

    internal static MapProjectionHandle Adopt(MlnMapProjection handle)
    {
        MapProjectionHandle? owner = null;
        try
        {
            owner = new MapProjectionHandle(handle);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_map_projection_close(handle, null);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnMapProjection Handle => state.Handle;

    internal NativeHandleState<MlnMapProjection>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_close"
        );
        state.Retire();
    }

    public void Close()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_close"
        );
        state.Close();
    }

    public CameraOptions GetCamera()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_get_camera"
        );
        var outCamera = new mln_camera_options { size = (uint)sizeof(mln_camera_options) };
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_get_camera(
                read.Handle,
                &outCamera,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return CopyCameraOptions(outCamera);
    }

    public LatLng LatLngForPixel(ScreenPoint point)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_lat_lng_for_pixel"
        );
        var outCoordinate = default(mln_lat_lng);
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel(
                read.Handle,
                NativeScreenPoint(point),
                &outCoordinate,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return CopyLatLng(outCoordinate);
    }

    public LatLng LatLngForPixelUnwrapped(ScreenPoint point)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_lat_lng_for_pixel_unwrapped"
        );
        var outCoordinate = default(mln_lat_lng);
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel_unwrapped(
                read.Handle,
                NativeScreenPoint(point),
                &outCoordinate,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return CopyLatLng(outCoordinate);
    }

    public double MetersPerPixelAtLatitude(double latitude)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_meters_per_pixel_at_latitude"
        );
        double outMetersPerPixel = default;
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_meters_per_pixel_at_latitude(
                read.Handle,
                latitude,
                &outMetersPerPixel,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return outMetersPerPixel;
    }

    public ScreenPoint PixelForLatLng(LatLng coordinate)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_pixel_for_lat_lng"
        );
        var outPoint = default(mln_screen_point);
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_pixel_for_lat_lng(
                read.Handle,
                NativeLatLng(coordinate),
                &outPoint,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        return CopyScreenPoint(outPoint);
    }

    public void SetCamera(CameraOptions camera)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_set_camera"
        );
        var nativeCamera = NativeCameraOptions(camera);
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_set_camera(
                Handle,
                &nativeCamera,
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
    }

    public void SetVisibleCoordinates(LatLng[] coordinates, EdgeInsets padding)
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_set_visible_coordinates"
        );
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_set_visible_coordinates(
                Handle,
                scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                checked((nuint)coordinates.Length),
                NativeEdgeInsets(padding),
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        scope.Accept();
    }

    public void SetVisibleGeometry(byte[] geometry, EdgeInsets padding)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_set_visible_geometry"
        );
        using var nativeGeometry = NativeStringView.From(geometry, nameof(geometry));
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_set_visible_geometry(
                Handle,
                nativeGeometry.Value,
                NativeEdgeInsets(padding),
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
    }
}
