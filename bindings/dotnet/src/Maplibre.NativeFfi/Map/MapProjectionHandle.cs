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
public sealed unsafe class MapProjectionHandle : IDisposable
{
    private unsafe delegate mln_status ProjectionOperationStart(MlnOperation* outOperation);

    private static MlnOperation StartOperation(ProjectionOperationStart start)
    {
        MlnOperation operation = default;
        NativeStatus.Check(start(&operation));
        return operation;
    }

    private readonly RuntimeHandle runtime;
    private readonly NativeHandleState<MlnMapProjection> state;

    private MapProjectionHandle(RuntimeHandle runtime, MlnMapProjection handle)
    {
        this.runtime = runtime;
        state = new NativeHandleState<MlnMapProjection>(
            handle,
            static _ => mln_status.MLN_STATUS_OK,
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
        var operation = StartOperation(outOperation =>
            NativeMethods.mln_map_projection_create_start(map.Handle, outOperation)
        );
        return OperationAwaiter.WaitThen(
            map.Runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                MlnMapProjection projection = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_projection_create_take_result(operation, &projection)
                );
                return new MapProjectionHandle(map.Runtime, projection);
            },
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    internal MlnMapProjection Handle => state.Handle;

    public bool IsClosed => state.IsClosed;

    public Task<CameraOptions> GetCameraAsync(CancellationToken cancellationToken = default)
    {
        var operation = StartOperation(outOperation =>
            NativeMethods.mln_map_projection_get_camera_start(Handle, outOperation)
        );
        return OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                var camera = NativeMethods.mln_camera_options_default();
                NativeStatus.Check(
                    NativeMethods.mln_map_projection_get_camera_take_result(operation, &camera)
                );
                return MapStructs.CameraOptionsFromNative(camera);
            },
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    public ulong SetCamera(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_set_camera(Handle, &nativeCamera, &commandId)
        );
        return commandId;
    }

    public ulong SetVisibleCoordinates(IReadOnlyList<LatLng> coordinates, EdgeInsets padding)
    {
        ArgumentNullException.ThrowIfNull(coordinates);
        var nativeCoordinates = new mln_lat_lng[coordinates.Count];
        for (var index = 0; index < coordinates.Count; index++)
        {
            nativeCoordinates[index] = CoreStructs.ToNative(coordinates[index]);
        }
        var nativePadding = MapStructs.ToNative(padding);
        ulong commandId = 0;
        fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_projection_set_visible_coordinates(
                    Handle,
                    nativeCoordinates.Length == 0 ? null : coordinatesPointer,
                    (nuint)nativeCoordinates.Length,
                    nativePadding,
                    &commandId
                )
            );
        }
        return commandId;
    }

    public ulong SetVisibleGeometry(byte[] geometry, EdgeInsets padding)
    {
        ArgumentNullException.ThrowIfNull(geometry);
        using var nativeGeometry = NativeStringView.From(geometry, nameof(geometry));
        var nativePadding = MapStructs.ToNative(padding);
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_set_visible_geometry(
                Handle,
                nativeGeometry.Value,
                nativePadding,
                &commandId
            )
        );
        return commandId;
    }

    public Task<ScreenPoint> PixelForLatLngAsync(
        LatLng coordinate,
        CancellationToken cancellationToken = default
    )
    {
        var nativeCoordinate = CoreStructs.ToNative(coordinate);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_pixel_for_lat_lng_start(
                Handle,
                nativeCoordinate,
                &operation
            )
        );
        return TakePointAsync(operation, cancellationToken);
    }

    public Task<LatLng> LatLngForPixelAsync(
        ScreenPoint point,
        CancellationToken cancellationToken = default
    )
    {
        var nativePoint = MapStructs.ToNative(point);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_map_projection_lat_lng_for_pixel_start(
                Handle,
                nativePoint,
                &operation
            )
        );
        return TakeCoordinateAsync(operation, cancellationToken);
    }

    public Task CloseAsync(CancellationToken cancellationToken = default)
    {
        if (IsClosed)
        {
            return Task.CompletedTask;
        }
        cancellationToken.ThrowIfCancellationRequested();
        var operation = StartOperation(outOperation =>
            NativeMethods.mln_map_projection_close_start(Handle, outOperation)
        );
        return OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation),
            () =>
            {
                RuntimeHandle.CheckOperationCompletion(operation);
                state.Close();
            },
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    public void Dispose() => CloseAsync().ConfigureAwait(false).GetAwaiter().GetResult();

    private Task<ScreenPoint> TakePointAsync(
        MlnOperation operation,
        CancellationToken cancellationToken
    ) =>
        OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                mln_screen_point point = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_projection_pixel_for_lat_lng_take_result(
                        operation,
                        &point
                    )
                );
                return MapStructs.FromNative(point);
            },
            () => NativeMethods.mln_operation_release(operation)
        );

    private Task<LatLng> TakeCoordinateAsync(
        MlnOperation operation,
        CancellationToken cancellationToken
    ) =>
        OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                mln_lat_lng coordinate = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_projection_lat_lng_for_pixel_take_result(
                        operation,
                        &coordinate
                    )
                );
                return CoreStructs.FromNative(coordinate);
            },
            () => NativeMethods.mln_operation_release(operation)
        );
}
