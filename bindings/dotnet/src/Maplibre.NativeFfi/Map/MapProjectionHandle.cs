using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;

namespace Maplibre.NativeFfi.Map;

/// <summary>Owner-thread projection snapshot handle.</summary>
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

    internal MlnMapProjection Handle => state.Handle;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Gets the projection camera descriptor.</summary>
    public CameraOptions GetCamera()
    {
        var camera = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(NativeMethods.mln_map_projection_get_camera(Handle, &camera));
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Sets the projection camera descriptor, applying only non-null fields.</summary>
    public void SetCamera(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        NativeStatus.Check(NativeMethods.mln_map_projection_set_camera(Handle, &nativeCamera));
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

    /// <summary>Sets a camera that makes the supplied geometry visible with padding.</summary>
    public void SetVisibleGeometry(Geometry geometry, EdgeInsets padding)
    {
        ArgumentNullException.ThrowIfNull(geometry);
        using var nativeGeometry = NativeGeometry.From(geometry);
        var nativePadding = MapStructs.ToNative(padding);
        NativeStatus.Check(
            NativeMethods.mln_map_projection_set_visible_geometry(
                Handle,
                nativeGeometry.Pointer,
                nativePadding
            )
        );
    }

    /// <summary>Converts a geographic coordinate to a screen pixel using this projection snapshot.</summary>
    public ScreenPoint PixelForLatLng(LatLng coordinate)
    {
        var nativeCoordinate = CoreStructs.ToNative(coordinate);
        mln_screen_point point = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_pixel_for_lat_lng(Handle, nativeCoordinate, &point)
        );
        return MapStructs.FromNative(point);
    }

    /// <summary>Converts a screen pixel to a geographic coordinate using this projection snapshot.</summary>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        var nativePoint = MapStructs.ToNative(point);
        mln_lat_lng coordinate = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel(Handle, nativePoint, &coordinate)
        );
        return CoreStructs.FromNative(coordinate);
    }

    /// <summary>Destroys the projection on its owner thread.</summary>
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
