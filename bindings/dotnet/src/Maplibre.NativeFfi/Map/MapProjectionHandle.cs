using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Map;

/// <summary>Any-thread standalone projection handle.</summary>
/// <remarks>
/// Every call after creation is synchronous, runs on the calling thread, is internally serialized,
/// and is thread-safe. A projection copies the map's transform state at creation, never
/// observes later map changes, and remains usable after its source map and runtime close.
/// </remarks>
public sealed unsafe class MapProjectionHandle : IDisposable
{
    private readonly NativeHandleState<MlnMapProjection> state;

    private MapProjectionHandle(MlnMapProjection handle)
    {
        state = new NativeHandleState<MlnMapProjection>(
            handle,
            static live => NativeMethods.mln_map_projection_close(live),
            nameof(MapProjectionHandle)
        );
    }

    internal static Task<MapProjectionHandle> CreateAsync(MapHandle map)
    {
        ArgumentNullException.ThrowIfNull(map);
        return NativeCompletion.Submit(
            completion => NativeMethods.mln_map_projection_create(map.Handle, completion),
            static result => new MapProjectionHandle(
                NativeCompletion.Value<MlnMapProjection>(result)
            )
        );
    }

    internal MlnMapProjection Handle => state.Handle;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Copies the projection camera, observing every earlier projection setter.</summary>
    public CameraOptions GetCamera()
    {
        var native = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(NativeMethods.mln_map_projection_get_camera(Handle, &native));
        return MapStructs.CameraOptionsFromNative(native);
    }

    /// <summary>Applies a camera update; only fields present on <paramref name="camera" /> apply.</summary>
    public void SetCamera(CameraOptions camera)
    {
        var native = MapStructs.ToNative(camera);
        NativeStatus.Check(NativeMethods.mln_map_projection_set_camera(Handle, &native));
    }

    /// <summary>Applies a camera fit for the coordinates.</summary>
    public void SetVisibleCoordinates(IReadOnlyList<LatLng> coordinates, EdgeInsets padding)
    {
        ArgumentNullException.ThrowIfNull(coordinates);
        var nativeCoordinates = new mln_lat_lng[coordinates.Count];
        for (var index = 0; index < coordinates.Count; index++)
        {
            nativeCoordinates[index] = CoreStructs.ToNative(coordinates[index]);
        }
        var nativePadding = MapStructs.ToNative(padding);
        var live = Handle;
        fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_projection_set_visible_coordinates(
                    live,
                    nativeCoordinates.Length == 0 ? null : coordinatesPointer,
                    (nuint)nativeCoordinates.Length,
                    nativePadding
                )
            );
        }
    }

    /// <summary>Applies a camera fit for GeoJSON Geometry bytes.</summary>
    public void SetVisibleGeometry(byte[] geometry, EdgeInsets padding)
    {
        ArgumentNullException.ThrowIfNull(geometry);
        using var nativeGeometry = NativeStringView.From(geometry, nameof(geometry));
        var nativePadding = MapStructs.ToNative(padding);
        NativeStatus.Check(
            NativeMethods.mln_map_projection_set_visible_geometry(
                Handle,
                nativeGeometry.Value,
                nativePadding
            )
        );
    }

    /// <summary>Converts a geographic coordinate to a logical-pixel screen point.</summary>
    public ScreenPoint PixelForLatLng(LatLng coordinate)
    {
        var nativeCoordinate = CoreStructs.ToNative(coordinate);
        mln_screen_point native = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_pixel_for_lat_lng(Handle, nativeCoordinate, &native)
        );
        return MapStructs.FromNative(native);
    }

    /// <summary>Converts a logical-pixel screen point to a geographic coordinate.</summary>
    /// <remarks>The longitude is wrapped to the range from -180 to 180 degrees.</remarks>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        var nativePoint = MapStructs.ToNative(point);
        mln_lat_lng native = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel(Handle, nativePoint, &native)
        );
        return CoreStructs.FromNative(native);
    }

    /// <summary>Converts a logical-pixel screen point to an unwrapped geographic coordinate.</summary>
    /// <remarks>The longitude preserves the visible world copy and may fall outside -180 to 180.</remarks>
    public LatLng LatLngForPixelUnwrapped(ScreenPoint point)
    {
        var nativePoint = MapStructs.ToNative(point);
        mln_lat_lng native = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel_unwrapped(
                Handle,
                nativePoint,
                &native
            )
        );
        return CoreStructs.FromNative(native);
    }

    /// <summary>Closes the projection, rejecting every later call on this handle.</summary>
    /// <remarks>
    /// A concurrent conversion running on another thread keeps the native object alive for its own
    /// call, because the C API leases the projection for each entry point.
    /// </remarks>
    public void Close() => state.Close();

    /// <inheritdoc />
    public void Dispose() => state.TryClose();
}
