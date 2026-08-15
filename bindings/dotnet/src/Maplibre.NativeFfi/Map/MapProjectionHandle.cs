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
/// and is thread-safe. A projection copies the map's transform state at creation and never
/// observes map changes made after that; a live projection prevents its map from closing.
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

    internal static Task<MapProjectionHandle> CreateAsync(
        MapHandle map,
        CancellationToken cancellationToken
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        cancellationToken.ThrowIfCancellationRequested();
        var operation = StartCreate(map.Handle);
        return OperationAwaiter.WaitThen(
            map.Runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                MlnMapProjection projection = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_projection_create_take_result(operation, &projection)
                );
                return new MapProjectionHandle(projection);
            },
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    private static MlnOperation StartCreate(MlnMap map)
    {
        MlnOperation operation = default;
        NativeStatus.Check(NativeMethods.mln_map_projection_create_start(map, &operation));
        return operation;
    }

    internal MlnMapProjection Handle => state.Handle;

    public bool IsClosed => state.IsClosed;

    /// <summary>Copies the projection camera, observing every earlier projection setter.</summary>
    public CameraOptions GetCamera()
    {
        var camera = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(NativeMethods.mln_map_projection_get_camera(Handle, &camera));
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Applies a camera update; only fields present on <paramref name="camera" /> apply.</summary>
    public void SetCamera(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        NativeStatus.Check(NativeMethods.mln_map_projection_set_camera(Handle, &nativeCamera));
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
        fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_projection_set_visible_coordinates(
                    Handle,
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
        mln_screen_point point = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_pixel_for_lat_lng(Handle, nativeCoordinate, &point)
        );
        return MapStructs.FromNative(point);
    }

    /// <summary>Converts a logical-pixel screen point to a geographic coordinate.</summary>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        var nativePoint = MapStructs.ToNative(point);
        mln_lat_lng coordinate = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel(Handle, nativePoint, &coordinate)
        );
        return CoreStructs.FromNative(coordinate);
    }

    /// <summary>
    /// Closes the projection, waiting for projection calls already running on other threads, and
    /// releases its map reservation before returning.
    /// </summary>
    public void Close() => state.Close();

    public void Dispose() => Close();
}
