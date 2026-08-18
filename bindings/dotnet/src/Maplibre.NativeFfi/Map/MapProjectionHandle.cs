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

    internal static Task<MapProjectionHandle> CreateAsync(
        MapHandle map,
        CancellationToken cancellationToken
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        cancellationToken.ThrowIfCancellationRequested();
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_projection_create(map.Handle, completion),
                static result => new MapProjectionHandle(
                    NativeCompletion.Value<MlnMapProjection>(result)
                )
            )
            .WaitAsync(cancellationToken);
    }

    internal MlnMapProjection Handle => state.Handle;

    public bool IsClosed => state.IsClosed;

    /// <summary>Copies the projection camera, observing every earlier projection setter.</summary>
    public CameraOptions GetCamera()
    {
        var camera = state.WithLive(live =>
        {
            var native = NativeMethods.mln_camera_options_default();
            NativeStatus.Check(NativeMethods.mln_map_projection_get_camera(live, &native));
            return native;
        });
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Applies a camera update; only fields present on <paramref name="camera" /> apply.</summary>
    public void SetCamera(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        state.WithLive(live =>
        {
            var native = nativeCamera;
            NativeStatus.Check(NativeMethods.mln_map_projection_set_camera(live, &native));
        });
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
        state.WithLive(live =>
        {
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
        });
    }

    /// <summary>Applies a camera fit for GeoJSON Geometry bytes.</summary>
    public void SetVisibleGeometry(byte[] geometry, EdgeInsets padding)
    {
        ArgumentNullException.ThrowIfNull(geometry);
        using var nativeGeometry = NativeStringView.From(geometry, nameof(geometry));
        var nativePadding = MapStructs.ToNative(padding);
        state.WithLive(live =>
            NativeStatus.Check(
                NativeMethods.mln_map_projection_set_visible_geometry(
                    live,
                    nativeGeometry.Value,
                    nativePadding
                )
            )
        );
    }

    /// <summary>Converts a geographic coordinate to a logical-pixel screen point.</summary>
    public ScreenPoint PixelForLatLng(LatLng coordinate)
    {
        var nativeCoordinate = CoreStructs.ToNative(coordinate);
        var point = state.WithLive(live =>
        {
            mln_screen_point native = default;
            NativeStatus.Check(
                NativeMethods.mln_map_projection_pixel_for_lat_lng(live, nativeCoordinate, &native)
            );
            return native;
        });
        return MapStructs.FromNative(point);
    }

    /// <summary>Converts a logical-pixel screen point to a geographic coordinate.</summary>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        var nativePoint = MapStructs.ToNative(point);
        var coordinate = state.WithLive(live =>
        {
            mln_lat_lng native = default;
            NativeStatus.Check(
                NativeMethods.mln_map_projection_lat_lng_for_pixel(live, nativePoint, &native)
            );
            return native;
        });
        return CoreStructs.FromNative(coordinate);
    }

    /// <summary>
    /// Closes the projection after waiting for projection calls already running on other threads.
    /// </summary>
    public void Close() => state.Close();

    public void Dispose() => state.TryClose();
}
