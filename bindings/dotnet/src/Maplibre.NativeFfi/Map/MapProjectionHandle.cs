using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;

namespace Maplibre.NativeFfi.Map;

/// <summary>Any-thread projection snapshot handle with serialized native calls.</summary>
public sealed unsafe class MapProjectionHandle : IDisposable
{
    private readonly NativeHandleState<MlnMapProjection> state;

    private MapProjectionHandle(MlnMapProjection handle)
    {
        state = new NativeHandleState<MlnMapProjection>(
            handle,
            static handle => NativeMethods.mln_map_projection_destroy(handle),
            nameof(MapProjectionHandle)
        );
    }

    internal static MapProjectionHandle Create(MapHandle map)
    {
        ArgumentNullException.ThrowIfNull(map);
        MlnMapProjection projection = default;
        NativeStatus.Check(NativeMethods.mln_map_projection_create(map.Handle, &projection));
        return new MapProjectionHandle(projection);
    }

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Gets the projection camera descriptor.</summary>
    public CameraOptions GetCamera()
    {
        return state.WithLive(handle =>
        {
            var camera = NativeMethods.mln_camera_options_default();
            NativeStatus.Check(NativeMethods.mln_map_projection_get_camera(handle, &camera));
            return MapStructs.CameraOptionsFromNative(camera);
        });
    }

    /// <summary>Sets the projection camera descriptor, applying only non-null fields.</summary>
    public void SetCamera(CameraOptions camera)
    {
        state.WithLive(handle =>
        {
            var nativeCamera = MapStructs.ToNative(camera);
            NativeStatus.Check(NativeMethods.mln_map_projection_set_camera(handle, &nativeCamera));
        });
    }

    /// <summary>Sets a camera that makes the supplied coordinates visible with padding.</summary>
    public void SetVisibleCoordinates(IReadOnlyList<LatLng> coordinates, EdgeInsets padding)
    {
        ArgumentNullException.ThrowIfNull(coordinates);
        var nativeCoordinates = new mln_lat_lng[coordinates.Count];
        for (var index = 0; index < coordinates.Count; index++)
        {
            nativeCoordinates[index] = CoreStructs.ToNative(coordinates[index]);
        }

        state.WithLive(handle =>
        {
            var nativePadding = MapStructs.ToNative(padding);
            fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
            {
                NativeStatus.Check(
                    NativeMethods.mln_map_projection_set_visible_coordinates(
                        handle,
                        nativeCoordinates.Length == 0 ? null : coordinatesPointer,
                        (nuint)nativeCoordinates.Length,
                        nativePadding
                    )
                );
            }
        });
    }

    /// <summary>Sets a camera that makes the supplied geometry visible with padding.</summary>
    public void SetVisibleGeometry(byte[] geometry, EdgeInsets padding)
    {
        ArgumentNullException.ThrowIfNull(geometry);
        state.WithLive(handle =>
        {
            using var nativeGeometry = NativeStringView.From(geometry, nameof(geometry));
            var nativePadding = MapStructs.ToNative(padding);
            NativeStatus.Check(
                NativeMethods.mln_map_projection_set_visible_geometry(
                    handle,
                    nativeGeometry.Value,
                    nativePadding
                )
            );
        });
    }

    /// <summary>Converts a geographic coordinate to a screen pixel using this projection snapshot.</summary>
    public ScreenPoint PixelForLatLng(LatLng coordinate)
    {
        return state.WithLive(handle =>
        {
            var nativeCoordinate = CoreStructs.ToNative(coordinate);
            mln_screen_point point = default;
            NativeStatus.Check(
                NativeMethods.mln_map_projection_pixel_for_lat_lng(handle, nativeCoordinate, &point)
            );
            return MapStructs.FromNative(point);
        });
    }

    /// <summary>Converts a screen pixel to a geographic coordinate using this projection snapshot.</summary>
    /// <remarks>The longitude is wrapped to the range from -180 to 180 degrees.</remarks>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        return state.WithLive(handle =>
        {
            var nativePoint = MapStructs.ToNative(point);
            mln_lat_lng coordinate = default;
            NativeStatus.Check(
                NativeMethods.mln_map_projection_lat_lng_for_pixel(handle, nativePoint, &coordinate)
            );
            return CoreStructs.FromNative(coordinate);
        });
    }

    /// <summary>Converts a screen pixel to an unwrapped geographic coordinate.</summary>
    /// <remarks>The longitude preserves the visible world copy and may fall outside -180 to 180.</remarks>
    public LatLng LatLngForPixelUnwrapped(ScreenPoint point)
    {
        return state.WithLive(handle =>
        {
            var nativePoint = MapStructs.ToNative(point);
            mln_lat_lng coordinate = default;
            NativeStatus.Check(
                NativeMethods.mln_map_projection_lat_lng_for_pixel_unwrapped(
                    handle,
                    nativePoint,
                    &coordinate
                )
            );
            return CoreStructs.FromNative(coordinate);
        });
    }

    /// <summary>Destroys the projection after active calls complete.</summary>
    public void Close()
    {
        state.Close();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        state.TryClose();
    }
}
