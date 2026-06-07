using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;

namespace Maplibre.Native.Map;

/// <summary>Owner-thread projection snapshot handle.</summary>
public sealed unsafe class MapProjectionHandle : IDisposable
{
    private readonly NativeHandleState<mln_map_projection> state;

    private MapProjectionHandle(mln_map_projection* handle)
    {
        state = new NativeHandleState<mln_map_projection>(
            handle,
            static handle => NativeMethods.mln_map_projection_destroy(handle),
            nameof(MapProjectionHandle)
        );
    }

    internal static MapProjectionHandle Create(MapHandle map)
    {
        ArgumentNullException.ThrowIfNull(map);
        mln_map_projection* projection = null;
        NativeStatus.Check(NativeMethods.mln_map_projection_create(map.Pointer, &projection));
        return new MapProjectionHandle(projection);
    }

    internal mln_map_projection* Pointer => state.Pointer;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Gets the projection camera descriptor.</summary>
    public CameraOptions GetCamera()
    {
        var camera = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(NativeMethods.mln_map_projection_get_camera(Pointer, &camera));
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Sets the projection camera descriptor, applying only non-null fields.</summary>
    public void SetCamera(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        NativeStatus.Check(NativeMethods.mln_map_projection_set_camera(Pointer, &nativeCamera));
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
                    Pointer,
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
                Pointer,
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
            NativeMethods.mln_map_projection_pixel_for_lat_lng(Pointer, nativeCoordinate, &point)
        );
        return MapStructs.FromNative(point);
    }

    /// <summary>Converts a screen pixel to a geographic coordinate using this projection snapshot.</summary>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        var nativePoint = MapStructs.ToNative(point);
        mln_lat_lng coordinate = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel(Pointer, nativePoint, &coordinate)
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
