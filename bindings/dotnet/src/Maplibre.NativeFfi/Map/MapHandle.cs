using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Map;

internal unsafe delegate mln_status MapAddCustomGeometrySource(
    MlnMap map,
    mln_buffer_view sourceId,
    mln_custom_geometry_source_options* options,
    ulong* outCommandId
);

internal unsafe delegate mln_status MapTakeCameraResult(
    MlnOperation operation,
    mln_camera_options* outCamera
);

internal unsafe delegate mln_status MapTakeBoundsResult(
    MlnOperation operation,
    mln_lat_lng_bounds* outBounds
);
internal unsafe delegate mln_status MapOperationStart(MlnOperation* outOperation);
internal delegate TResult MapOperationTake<TResult>(MlnOperation operation);

/// <summary>Any-thread map handle bound to a runtime.</summary>
public sealed unsafe partial class MapHandle : IDisposable
{
    private static readonly MapAddCustomGeometrySource DefaultAddCustomGeometrySource = static (
        map,
        sourceId,
        options,
        commandId
    ) => NativeMethods.mln_map_add_custom_geometry_source(map, sourceId, options, commandId);

    [ThreadStatic]
    private static MapAddCustomGeometrySource? addCustomGeometrySourceForTest;

    private readonly RuntimeHandle runtime;
    private readonly ulong nativeId;
    private readonly NativeHandleState<MlnMap> state;

    private MapHandle(RuntimeHandle runtime, MlnMap handle)
    {
        this.runtime = runtime;
        nativeId = handle.Value;
        state = new NativeHandleState<MlnMap>(
            handle,
            static _ => mln_status.MLN_STATUS_OK,
            nameof(MapHandle)
        );
    }

    /// <summary>Creates a map asynchronously.</summary>
    public static Task<MapHandle> CreateAsync(
        RuntimeHandle runtime,
        MapOptions options,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(runtime);
        ArgumentNullException.ThrowIfNull(options);
        cancellationToken.ThrowIfCancellationRequested();
        var nativeOptions = options.ToNative();
        var operation = StartCreate(runtime.Handle, nativeOptions);
        return OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                MlnMap map = default;
                NativeStatus.Check(NativeMethods.mln_map_create_take_result(operation, &map));
                var handle = new MapHandle(runtime, map);
                runtime.RegisterMap(handle);
                return handle;
            },
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    internal MlnMap Handle => state.Handle;

    internal RuntimeHandle Runtime => runtime;

    /// <summary>The issued native handle id, readable after close.</summary>
    internal ulong NativeId => nativeId;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Requests a repaint and returns its runtime command id.</summary>
    public ulong RequestRepaint()
    {
        ulong commandId = 0;
        NativeStatus.Check(NativeMethods.mln_map_request_repaint(Handle, &commandId));
        return commandId;
    }

    /// <summary>Requests a noncoalescing still-image render.</summary>
    public Task RequestStillImageAsync(CancellationToken cancellationToken = default)
    {
        var operation = StartMapOperation(outOperation =>
            NativeMethods.mln_map_request_still_image_start(Handle, outOperation)
        );
        return OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () => RuntimeHandle.CheckOperationCompletion(operation),
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    /// <summary>Sets native debug drawing options and returns its command ID.</summary>
    /// <remarks>The committed mask is visible as <see cref="MapSnapshot.DebugOptions" />.</remarks>
    public ulong SetDebugOptions(DebugOptions options)
    {
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_debug_options(Handle, (uint)options, &commandId)
        );
        return commandId;
    }

    /// <summary>Shows or hides the built-in rendering statistics overlay and returns its command ID.</summary>
    /// <remarks>
    /// The committed value is visible as <see cref="MapSnapshot.RenderingStatsViewEnabled" />.
    /// </remarks>
    public ulong SetRenderingStatsViewEnabled(bool enabled)
    {
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_rendering_stats_view_enabled(
                Handle,
                enabled ? (byte)1 : (byte)0,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Asks the native map to write debug logs and returns its command ID.</summary>
    public ulong DumpDebugLogs()
    {
        ulong commandId = 0;
        NativeStatus.Check(NativeMethods.mln_map_dump_debug_logs(Handle, &commandId));
        return commandId;
    }

    /// <summary>Gets a synchronous copy of the committed map state.</summary>
    public MapSnapshot GetSnapshot()
    {
        var snapshot = new mln_map_snapshot { size = (uint)sizeof(mln_map_snapshot) };
        NativeStatus.Check(NativeMethods.mln_map_snapshot_get(Handle, &snapshot));
        return new MapSnapshot(
            snapshot.generation,
            (DebugOptions)snapshot.debug_options,
            MapStructs.CameraOptionsFromNative(snapshot.camera),
            new LogicalExtent(
                snapshot.logical_extent.width,
                snapshot.logical_extent.height,
                snapshot.logical_extent.scale_factor
            ),
            MapStructs.ProjectionModeOptionsFromNative(snapshot.projection_mode),
            MapStructs.ViewportOptionsFromNative(snapshot.viewport),
            snapshot.fully_loaded != 0,
            snapshot.rendering_stats_view_enabled != 0,
            snapshot.repaint_demand != 0,
            (RuntimeEventMask)snapshot.event_mask,
            snapshot.latest_render_update_generation,
            MapStructs.TileOptionsFromNative(snapshot.tile),
            MapStructs.BoundOptionsFromNative(snapshot.bounds),
            MapStructs.FreeCameraOptionsFromNative(snapshot.free_camera)
        );
    }

    /// <summary>Submits a logical extent change and returns its runtime command id.</summary>
    public ulong Resize(LogicalExtent extent)
    {
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_resize(
                Handle,
                new mln_logical_extent
                {
                    width = extent.Width,
                    height = extent.Height,
                    scale_factor = extent.ScaleFactor,
                },
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Sets viewport options and returns its command ID.</summary>
    /// <remarks>The committed options are visible as <see cref="MapSnapshot.Viewport" />.</remarks>
    public ulong SetViewportOptions(ViewportOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_viewport_options(Handle, &nativeOptions, &commandId)
        );
        return commandId;
    }

    /// <summary>Sets tile tuning options and returns its command ID.</summary>
    /// <remarks>The committed options are visible as <see cref="MapSnapshot.Tile" />.</remarks>
    public ulong SetTileOptions(TileOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_tile_options(Handle, &nativeOptions, &commandId)
        );
        return commandId;
    }

    /// <summary>Gets a synchronous camera snapshot.</summary>
    public CameraSnapshot GetCameraSnapshot()
    {
        var camera = NativeMethods.mln_camera_options_default();
        ulong generation = 0;
        NativeStatus.Check(NativeMethods.mln_map_camera_snapshot_get(Handle, &camera, &generation));
        return new CameraSnapshot(MapStructs.CameraOptionsFromNative(camera), generation);
    }

    /// <summary>Submits a copied camera update and returns its runtime command id.</summary>
    public ulong UpdateCamera(CameraUpdate update)
    {
        var native = MapStructs.ToNative(update);
        ulong commandId = 0;
        NativeStatus.Check(NativeMethods.mln_map_update_camera(Handle, &native, &commandId));
        return commandId;
    }

    /// <summary>Submits one relative camera operation.</summary>
    public ulong ApplyCameraDelta(CameraDelta delta)
    {
        var native = MapStructs.ToNative(delta);
        ulong commandId = 0;
        NativeStatus.Check(NativeMethods.mln_map_apply_camera_delta(Handle, &native, &commandId));
        return commandId;
    }

    /// <summary>Reads the camera in runtime order.</summary>
    public Task<CameraSnapshot> QueryCameraAsync(CancellationToken cancellationToken = default)
    {
        var operation = StartMapOperation(outOperation =>
            NativeMethods.mln_map_camera_query_start(Handle, outOperation)
        );
        return OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                var result = new mln_camera_query_result
                {
                    size = (uint)sizeof(mln_camera_query_result),
                };
                NativeStatus.Check(
                    NativeMethods.mln_map_camera_query_take_result(operation, &result)
                );
                return new CameraSnapshot(
                    MapStructs.CameraOptionsFromNative(result.camera),
                    result.generation
                );
            },
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    public Task<CameraOptions> CameraForLatLngBoundsAsync(
        LatLngBounds bounds,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        var nativeBounds = MapStructs.ToNative(bounds);
        var nativeFitOptions = fitOptions is null ? default : MapStructs.ToNative(fitOptions);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_map_camera_for_lat_lng_bounds_start(
                Handle,
                nativeBounds,
                fitOptions is null ? null : &nativeFitOptions,
                &operation
            )
        );
        return TakeCameraAsync(
            operation,
            NativeMethods.mln_map_camera_for_lat_lng_bounds_take_result,
            cancellationToken
        );
    }

    public Task<CameraOptions> CameraForLatLngsAsync(
        IReadOnlyList<LatLng> coordinates,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(coordinates);
        var nativeCoordinates = new mln_lat_lng[coordinates.Count];
        for (var index = 0; index < coordinates.Count; index++)
        {
            nativeCoordinates[index] = CoreStructs.ToNative(coordinates[index]);
        }
        var nativeFitOptions = fitOptions is null ? default : MapStructs.ToNative(fitOptions);
        MlnOperation operation = default;
        fixed (mln_lat_lng* pointer = nativeCoordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_camera_for_lat_lngs_start(
                    Handle,
                    nativeCoordinates.Length == 0 ? null : pointer,
                    (nuint)nativeCoordinates.Length,
                    fitOptions is null ? null : &nativeFitOptions,
                    &operation
                )
            );
        }
        return TakeCameraAsync(
            operation,
            NativeMethods.mln_map_camera_for_lat_lngs_take_result,
            cancellationToken
        );
    }

    public Task<CameraOptions> CameraForGeometryAsync(
        byte[] geometry,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeGeometry = NativeStringView.From(geometry, nameof(geometry));
        var nativeFitOptions = fitOptions is null ? default : MapStructs.ToNative(fitOptions);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_map_camera_for_geometry_start(
                Handle,
                nativeGeometry.Value,
                fitOptions is null ? null : &nativeFitOptions,
                &operation
            )
        );
        return TakeCameraAsync(
            operation,
            NativeMethods.mln_map_camera_for_geometry_take_result,
            cancellationToken
        );
    }

    public Task<LatLngBounds> LatLngBoundsForCameraAsync(
        CameraOptions camera,
        CancellationToken cancellationToken = default
    )
    {
        var nativeCamera = MapStructs.ToNative(camera);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_map_lat_lng_bounds_for_camera_start(Handle, &nativeCamera, &operation)
        );
        return TakeBoundsAsync(
            operation,
            NativeMethods.mln_map_lat_lng_bounds_for_camera_take_result,
            cancellationToken
        );
    }

    public Task<LatLngBounds> LatLngBoundsForCameraUnwrappedAsync(
        CameraOptions camera,
        CancellationToken cancellationToken = default
    )
    {
        var nativeCamera = MapStructs.ToNative(camera);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_map_lat_lng_bounds_for_camera_unwrapped_start(
                Handle,
                &nativeCamera,
                &operation
            )
        );
        return TakeBoundsAsync(
            operation,
            NativeMethods.mln_map_lat_lng_bounds_for_camera_unwrapped_take_result,
            cancellationToken
        );
    }

    /// <summary>Sets map bounds constraints and returns its command ID.</summary>
    /// <remarks>The committed constraints are visible as <see cref="MapSnapshot.Bounds" />.</remarks>
    public ulong SetBounds(BoundOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        ulong commandId = 0;
        NativeStatus.Check(NativeMethods.mln_map_set_bounds(Handle, &nativeOptions, &commandId));
        return commandId;
    }

    /// <summary>Sets free-camera options and returns its command ID.</summary>
    /// <remarks>The committed options are visible as <see cref="MapSnapshot.FreeCamera" />.</remarks>
    public ulong SetFreeCameraOptions(FreeCameraOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_free_camera_options(Handle, &nativeOptions, &commandId)
        );
        return commandId;
    }

    public Task<ScreenPoint> PixelForLatLngAsync(
        LatLng coordinate,
        CancellationToken cancellationToken = default
    )
    {
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_map_pixel_for_lat_lng_start(
                Handle,
                CoreStructs.ToNative(coordinate),
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
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_map_lat_lng_for_pixel_start(
                Handle,
                MapStructs.ToNative(point),
                &operation
            )
        );
        return TakeCoordinateAsync(operation, cancellationToken);
    }

    public Task<ScreenPoint[]> PixelsForLatLngsAsync(
        IReadOnlyList<LatLng> coordinates,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(coordinates);
        var native = coordinates.Select(value => CoreStructs.ToNative(value)).ToArray();
        MlnOperation operation = default;
        fixed (mln_lat_lng* pointer = native)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_pixels_for_lat_lngs_start(
                    Handle,
                    native.Length == 0 ? null : pointer,
                    (nuint)native.Length,
                    &operation
                )
            );
        }
        return TakePointsAsync(operation, native.Length, cancellationToken);
    }

    public Task<LatLng[]> LatLngsForPixelsAsync(
        IReadOnlyList<ScreenPoint> points,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(points);
        var native = points.Select(value => MapStructs.ToNative(value)).ToArray();
        MlnOperation operation = default;
        fixed (mln_screen_point* pointer = native)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_lat_lngs_for_pixels_start(
                    Handle,
                    native.Length == 0 ? null : pointer,
                    (nuint)native.Length,
                    &operation
                )
            );
        }
        return TakeCoordinatesAsync(operation, native.Length, cancellationToken);
    }

    /// <summary>Creates a standalone projection in runtime order.</summary>
    public Task<MapProjectionHandle> CreateProjectionAsync(
        CancellationToken cancellationToken = default
    ) => MapProjectionHandle.CreateAsync(this, cancellationToken);

    /// <summary>Sets projection mode options and returns its command ID.</summary>
    public ulong SetProjectionMode(ProjectionModeOptions mode)
    {
        var nativeMode = MapStructs.ToNative(mode);
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_projection_mode(Handle, &nativeMode, &commandId)
        );
        return commandId;
    }

    /// <summary>Loads a style URL and returns its command ID.</summary>
    public ulong SetStyleUrl(string url)
    {
        ArgumentNullException.ThrowIfNull(url);
        using var nativeUrl = NativeUtf8String.FromNullableString(url, nameof(url));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_style_url(Handle, nativeUrl.Pointer, &commandId)
        );
        return commandId;
    }

    /// <summary>Loads inline style JSON and returns its command ID.</summary>
    public ulong SetStyleJson(byte[] json)
    {
        ArgumentNullException.ThrowIfNull(json);
        using var nativeJson = NativeStringView.From(json, nameof(json));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_style_json(Handle, nativeJson.Value, &commandId)
        );
        return commandId;
    }

    /// <summary>Gets the style document that this map's style was last parsed from.</summary>
    public Task<byte[]> GetLoadedStyleJsonAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            operation => NativeMethods.mln_map_loaded_style_json_start(Handle, operation),
            operation =>
            {
                MlnBuffer buffer = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_loaded_style_json_take_result(operation, &buffer)
                );
                return ValueStructs.ReadBuffer(buffer);
            },
            cancellationToken
        );

    /// <summary>Gets the URL that this map's style was last requested from.</summary>
    public Task<string> GetStyleUrlAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            operation => NativeMethods.mln_map_style_url_start(Handle, operation),
            operation =>
            {
                MlnBuffer buffer = default;
                NativeStatus.Check(NativeMethods.mln_map_style_url_take_result(operation, &buffer));
                return System.Text.Encoding.UTF8.GetString(ValueStructs.ReadBuffer(buffer));
            },
            cancellationToken
        );

    /// <summary>Selects which map-originated event types this map queues.</summary>
    public ulong SetEventMask(RuntimeEventMask mask)
    {
        ulong commandId = 0;
        NativeStatus.Check(NativeMethods.mln_map_set_event_mask(Handle, (ulong)mask, &commandId));
        return commandId;
    }

    /// <summary>Reports the event mask in the latest committed map snapshot.</summary>
    public RuntimeEventMask GetEventMask() => GetSnapshot().EventMask;

    /// <summary>Adds a style source and returns its command ID.</summary>
    public ulong AddStyleSourceJson(string sourceId, byte[] sourceJson)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeJson = NativeStringView.From(sourceJson, nameof(sourceJson));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_add_style_source_json(
                Handle,
                nativeSourceId.Value,
                nativeJson.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Submits a command that removes one style source and returns its command ID.</summary>
    /// <remarks>
    /// The command's <c>COMMAND_FINISHED</c> event reports
    /// <see cref="Error.MaplibreStatus.NotFound" /> when no source has <paramref name="sourceId" />
    /// and <see cref="Error.MaplibreStatus.InvalidState" /> when a layer still uses the source.
    /// </remarks>
    public ulong RemoveStyleSource(string sourceId)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_remove_style_source(Handle, nativeSourceId.Value, &commandId)
        );
        return commandId;
    }

    internal Task<(mln_style_source_info Info, bool Found)> QueryStyleSourceInfoAsync(
        string sourceId,
        CancellationToken cancellationToken
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_style_source_info_start(
                    Handle,
                    nativeSourceId.Value,
                    operation
                ),
            operation =>
            {
                var info = new mln_style_source_info { size = (uint)sizeof(mln_style_source_info) };
                bool found = false;
                NativeStatus.Check(
                    NativeMethods.mln_map_get_style_source_info_take_result(
                        operation,
                        &info,
                        &found
                    )
                );
                return (info, found);
            },
            cancellationToken
        );
    }

    internal Task<string?> CopyStyleSourceStringAsync(
        string sourceId,
        bool attribution,
        CancellationToken cancellationToken
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return CopyStyleSourceStringAsync(nativeSourceId.Value, attribution, cancellationToken);
    }

    internal Task<string[]?> CopyStyleSourceTileUrlsAsync(
        string sourceId,
        CancellationToken cancellationToken
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_style_source_tile_urls_start(
                    Handle,
                    nativeSourceId.Value,
                    operation
                ),
            operation =>
            {
                MlnStyleStringList list = default;
                bool found = false;
                NativeStatus.Check(
                    NativeMethods.mln_map_get_style_source_tile_urls_take_result(
                        operation,
                        &list,
                        &found
                    )
                );
                return found ? CopyStyleStringList(list) : null;
            },
            cancellationToken
        );
    }

    /// <summary>Lists style source IDs in style order.</summary>
    public Task<string[]> StyleSourceIdsAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            operation => NativeMethods.mln_map_list_style_source_ids_start(Handle, operation),
            operation =>
            {
                MlnStyleIdList list = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_list_style_source_ids_take_result(operation, &list)
                );
                return CopyStyleIdList(list);
            },
            cancellationToken
        );

    /// <summary>Adds a GeoJSON source that loads data from a URL.</summary>
    /// <remarks>
    /// <paramref name="options" /> is fixed when the source is created;
    /// <see cref="SetGeoJsonSourceUrl" /> and <see cref="SetGeoJsonSourceData" /> keep it.
    /// </remarks>
    public ulong AddGeoJsonSourceUrl(string sourceId, string url, GeoJsonSourceOptions? options)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeGeoJsonSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_geojson_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                nativeOptions is null ? null : &optionsValue,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Updates a GeoJSON source to load data from a URL.</summary>
    public ulong SetGeoJsonSourceUrl(string sourceId, string url)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        NativeStatus.Check(
            NativeMethods.mln_map_set_geojson_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a GeoJSON source backed by prepared data.</summary>
    /// <remarks>
    /// The call borrows <paramref name="data" />; the source adopts the options baked into it
    /// when the data was prepared and keeps its own reference, so the handle may be released
    /// afterward.
    /// </remarks>
    public ulong AddGeoJsonSourceData(string sourceId, GeoJsonSourceDataHandle data)
    {
        ArgumentNullException.ThrowIfNull(data);
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        data.WithLive(prepared =>
        {
            ulong id = 0;
            NativeStatus.Check(
                NativeMethods.mln_map_add_geojson_source_data(
                    Handle,
                    nativeSourceId.Value,
                    prepared,
                    &id
                )
            );
            commandId = id;
        });
        return commandId;
    }

    /// <summary>Updates a GeoJSON source with prepared data.</summary>
    /// <remarks>
    /// The call borrows <paramref name="data" />; the command fails when the options baked into
    /// it differ from the source's, except for cluster properties.
    /// </remarks>
    public ulong SetGeoJsonSourceData(string sourceId, GeoJsonSourceDataHandle data)
    {
        ArgumentNullException.ThrowIfNull(data);
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        data.WithLive(prepared =>
        {
            ulong id = 0;
            NativeStatus.Check(
                NativeMethods.mln_map_set_geojson_source_data(
                    Handle,
                    nativeSourceId.Value,
                    prepared,
                    &id
                )
            );
            commandId = id;
        });
        return commandId;
    }

    /// <summary>Overrides synchronous tiling for a GeoJSON source.</summary>
    /// <remarks>
    /// The effective behavior is the source's baked-in option OR this override.
    /// </remarks>
    public ulong SetGeoJsonSourceSynchronousTiling(string sourceId, bool enabled)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_geojson_source_synchronous_tiling(
                Handle,
                nativeSourceId.Value,
                enabled ? (byte)1 : (byte)0,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a custom geometry source with tile callbacks.</summary>
    /// <remarks>
    /// The upcall stubs this installs live until MapLibre stops referencing them: until the source
    /// is removed, until a style load leaves a style without it, or until this map closes. The
    /// binding releases them from the callback the C API invokes then, so nothing here depends on
    /// the events <see cref="SetEventMask" /> selects.
    /// </remarks>
    public ulong AddCustomGeometrySource(string sourceId, CustomGeometrySourceOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        return AddCustomGeometrySource(sourceId, new CustomGeometrySourceState(options));
    }

    internal ulong AddCustomGeometrySource(string sourceId, CustomGeometrySourceState sourceState)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        try
        {
            var descriptor = sourceState.Descriptor;
            ulong commandId = 0;
            NativeStatus.Check(
                AddCustomGeometrySourceNative(Handle, nativeSourceId.Value, &descriptor, &commandId)
            );
            return commandId;
        }
        catch
        {
            sourceState.Dispose();
            throw;
        }
    }

    /// <summary>Sets custom geometry source tile data.</summary>
    public ulong SetCustomGeometrySourceTileData(
        string sourceId,
        CanonicalTileId tileId,
        byte[] data
    )
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeData = NativeStringView.From(data, nameof(data));
        var nativeTileId = StyleStructs.ToNative(tileId);
        NativeStatus.Check(
            NativeMethods.mln_map_set_custom_geometry_source_tile_data(
                Handle,
                nativeSourceId.Value,
                nativeTileId,
                nativeData.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Invalidates one custom geometry source tile.</summary>
    public ulong InvalidateCustomGeometrySourceTile(string sourceId, CanonicalTileId tileId)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        NativeStatus.Check(
            NativeMethods.mln_map_invalidate_custom_geometry_source_tile(
                Handle,
                nativeSourceId.Value,
                StyleStructs.ToNative(tileId),
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Invalidates custom geometry source tiles that intersect bounds.</summary>
    public ulong InvalidateCustomGeometrySourceRegion(string sourceId, LatLngBounds bounds)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        NativeStatus.Check(
            NativeMethods.mln_map_invalidate_custom_geometry_source_region(
                Handle,
                nativeSourceId.Value,
                MapStructs.ToNative(bounds),
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a vector source that loads TileJSON from a URL.</summary>
    public ulong AddVectorSourceUrl(string sourceId, string url, TileSourceOptions? options)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_vector_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                nativeOptions is null ? null : &optionsValue,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a vector source from inline tile URL templates.</summary>
    public ulong AddVectorSourceTiles(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options
    )
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_vector_source_tiles(
                Handle,
                nativeSourceId.Value,
                nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                nativeTiles.Count,
                nativeOptions is null ? null : &optionsValue,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a raster source that loads TileJSON from a URL.</summary>
    public ulong AddRasterSourceUrl(string sourceId, string url, TileSourceOptions? options)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                nativeOptions is null ? null : &optionsValue,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a raster source from inline tile URL templates.</summary>
    public ulong AddRasterSourceTiles(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options
    )
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_source_tiles(
                Handle,
                nativeSourceId.Value,
                nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                nativeTiles.Count,
                nativeOptions is null ? null : &optionsValue,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a raster DEM source that loads TileJSON from a URL.</summary>
    public ulong AddRasterDemSourceUrl(string sourceId, string url, TileSourceOptions? options)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_dem_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                nativeOptions is null ? null : &optionsValue,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a raster DEM source from inline tile URL templates.</summary>
    public ulong AddRasterDemSourceTiles(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options
    )
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_dem_source_tiles(
                Handle,
                nativeSourceId.Value,
                nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                nativeTiles.Count,
                nativeOptions is null ? null : &optionsValue,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Sets or replaces a style image.</summary>
    public ulong SetStyleImage(
        string imageId,
        PremultipliedRgba8Image image,
        StyleImageOptions? options
    )
    {
        ulong commandId = 0;
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        using var nativeImage = NativeStyleImage.From(image);
        var imageValue = nativeImage.Value;
        using var nativeOptions = options is null ? null : NativeStyleImageOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_set_style_image(
                Handle,
                nativeImageId.Value,
                &imageValue,
                nativeOptions is null ? null : &optionsValue,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Submits a command that removes one runtime style image and returns its command ID.</summary>
    /// <remarks>
    /// The command's <c>COMMAND_FINISHED</c> event reports
    /// <see cref="Error.MaplibreStatus.NotFound" /> when no image has <paramref name="imageId" />.
    /// </remarks>
    public ulong RemoveStyleImage(string imageId)
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_remove_style_image(Handle, nativeImageId.Value, &commandId)
        );
        return commandId;
    }

    /// <summary>Gets style image metadata when the image exists.</summary>
    public Task<StyleImageInfo?> StyleImageInfoAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_style_image_info_start(
                    Handle,
                    nativeImageId.Value,
                    operation
                ),
            operation =>
            {
                var info = NativeMethods.mln_style_image_info_default();
                bool found = false;
                NativeStatus.Check(
                    NativeMethods.mln_map_get_style_image_info_take_result(operation, &info, &found)
                );
                return found ? StyleStructs.FromNative(info) : null;
            },
            cancellationToken
        );
    }

    private static IReadOnlyList<ImageStretch> ToStretches(mln_image_stretch[] raw) =>
        Array.ConvertAll(raw, stretch => new ImageStretch(stretch.from, stretch.to));

    private static IReadOnlyList<ImageStretch>? NullIfEmpty(IReadOnlyList<ImageStretch>? values) =>
        values is null || values.Count == 0 ? null : values;

    internal Task<(
        IReadOnlyList<ImageStretch> StretchX,
        IReadOnlyList<ImageStretch> StretchY
    )?> TakeStyleImageStretchesAsync(
        string imageId,
        StyleImageInfo info,
        CancellationToken cancellationToken
    )
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        var operation = StartMapOperation(outOperation =>
            NativeMethods.mln_map_copy_style_image_stretches_start(
                Handle,
                nativeImageId.Value,
                outOperation
            )
        );
        return OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                RuntimeHandle.CheckOperationCompletion(operation);
                var rawX = new mln_image_stretch[checked((int)info.StretchXCount)];
                var rawY = new mln_image_stretch[checked((int)info.StretchYCount)];
                nuint xCount = 0;
                nuint yCount = 0;
                bool found = false;
                mln_status status;
                fixed (mln_image_stretch* pointerX = rawX)
                fixed (mln_image_stretch* pointerY = rawY)
                {
                    status = NativeMethods.mln_map_copy_style_image_stretches_take_result(
                        operation,
                        rawX.Length == 0 ? null : pointerX,
                        (nuint)rawX.Length,
                        &xCount,
                        rawY.Length == 0 ? null : pointerY,
                        (nuint)rawY.Length,
                        &yCount,
                        &found
                    );
                }
                if (status == mln_status.MLN_STATUS_INVALID_ARGUMENT && found)
                {
                    rawX = new mln_image_stretch[checked((int)xCount)];
                    rawY = new mln_image_stretch[checked((int)yCount)];
                    fixed (mln_image_stretch* pointerX = rawX)
                    fixed (mln_image_stretch* pointerY = rawY)
                    {
                        status = NativeMethods.mln_map_copy_style_image_stretches_take_result(
                            operation,
                            rawX.Length == 0 ? null : pointerX,
                            (nuint)rawX.Length,
                            &xCount,
                            rawY.Length == 0 ? null : pointerY,
                            (nuint)rawY.Length,
                            &yCount,
                            &found
                        );
                    }
                }
                NativeStatus.Check(status);
                return found
                    ? (ToStretches(rawX), ToStretches(rawY))
                    : ((IReadOnlyList<ImageStretch>, IReadOnlyList<ImageStretch>)?)null;
            },
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    internal Task<byte[]?> CopyStyleImagePixelsAsync(
        string imageId,
        CancellationToken cancellationToken
    )
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_copy_style_image_premultiplied_rgba8_start(
                    Handle,
                    nativeImageId.Value,
                    operation
                ),
            operation =>
            {
                MlnBuffer buffer = default;
                bool found = false;
                NativeStatus.Check(
                    NativeMethods.mln_map_copy_style_image_premultiplied_rgba8_take_result(
                        operation,
                        &buffer,
                        &found
                    )
                );
                return found ? ValueStructs.ReadBuffer(buffer) : null;
            },
            cancellationToken
        );
    }

    /// <summary>Adds an image source that loads image data from a URL.</summary>
    public ulong AddImageSourceUrl(string sourceId, IReadOnlyList<LatLng> coordinates, string url)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        var nativeCoordinates = ToNativeCoordinates(coordinates, nameof(coordinates));
        fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_add_image_source_url(
                    Handle,
                    nativeSourceId.Value,
                    coordinatesPointer,
                    (nuint)nativeCoordinates.Length,
                    nativeUrl.Value,
                    &commandId
                )
            );
        }
        return commandId;
    }

    /// <summary>Adds an image source with inline premultiplied RGBA8 image data.</summary>
    public ulong AddImageSourceImage(
        string sourceId,
        IReadOnlyList<LatLng> coordinates,
        PremultipliedRgba8Image image
    )
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeImage = NativeStyleImage.From(image);
        var imageValue = nativeImage.Value;
        var nativeCoordinates = ToNativeCoordinates(coordinates, nameof(coordinates));
        fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_add_image_source_image(
                    Handle,
                    nativeSourceId.Value,
                    coordinatesPointer,
                    (nuint)nativeCoordinates.Length,
                    &imageValue,
                    &commandId
                )
            );
        }
        return commandId;
    }

    /// <summary>Updates an image source to load image data from a URL.</summary>
    public ulong SetImageSourceUrl(string sourceId, string url)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        NativeStatus.Check(
            NativeMethods.mln_map_set_image_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Updates an image source with inline premultiplied RGBA8 image data.</summary>
    public ulong SetImageSourceImage(string sourceId, PremultipliedRgba8Image image)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeImage = NativeStyleImage.From(image);
        var imageValue = nativeImage.Value;
        NativeStatus.Check(
            NativeMethods.mln_map_set_image_source_image(
                Handle,
                nativeSourceId.Value,
                &imageValue,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Updates image source coordinates.</summary>
    public ulong SetImageSourceCoordinates(string sourceId, IReadOnlyList<LatLng> coordinates)
    {
        ulong commandId = 0;
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var nativeCoordinates = ToNativeCoordinates(coordinates, nameof(coordinates));
        fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_image_source_coordinates(
                    Handle,
                    nativeSourceId.Value,
                    coordinatesPointer,
                    (nuint)nativeCoordinates.Length,
                    &commandId
                )
            );
        }
        return commandId;
    }

    /// <summary>Gets image source coordinates when the source exists.</summary>
    public Task<LatLng[]?> GetImageSourceCoordinatesAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_image_source_coordinates_start(
                    Handle,
                    nativeSourceId.Value,
                    operation
                ),
            operation =>
            {
                var coordinates = new mln_lat_lng[4];
                nuint count = 0;
                bool found = false;
                fixed (mln_lat_lng* pointer = coordinates)
                {
                    NativeStatus.Check(
                        NativeMethods.mln_map_get_image_source_coordinates_take_result(
                            operation,
                            pointer,
                            (nuint)coordinates.Length,
                            &count,
                            &found
                        )
                    );
                }
                if (!found)
                {
                    return null;
                }
                var result = new LatLng[checked((int)count)];
                for (var index = 0; index < result.Length; index++)
                {
                    result[index] = CoreStructs.FromNative(coordinates[index]);
                }
                return result;
            },
            cancellationToken
        );
    }

    /// <summary>Adds a hillshade layer for a raster DEM source.</summary>
    public ulong AddHillshadeLayer(string layerId, string sourceId, string beforeLayerId)
    {
        ulong commandId = 0;
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_hillshade_layer(
                Handle,
                nativeLayerId.Value,
                nativeSourceId.Value,
                nativeBeforeLayerId.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a color-relief layer for a raster DEM source.</summary>
    public ulong AddColorReliefLayer(string layerId, string sourceId, string beforeLayerId)
    {
        ulong commandId = 0;
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_color_relief_layer(
                Handle,
                nativeLayerId.Value,
                nativeSourceId.Value,
                nativeBeforeLayerId.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a source-free location indicator layer.</summary>
    public ulong AddLocationIndicatorLayer(string layerId, string beforeLayerId)
    {
        ulong commandId = 0;
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_location_indicator_layer(
                Handle,
                nativeLayerId.Value,
                nativeBeforeLayerId.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Sets a location indicator layer location.</summary>
    public ulong SetLocationIndicatorLocation(string layerId, LatLng coordinate, double altitude)
    {
        ulong commandId = 0;
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_location_indicator_location(
                Handle,
                nativeLayerId.Value,
                CoreStructs.ToNative(coordinate),
                altitude,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Sets a location indicator layer bearing in degrees.</summary>
    public ulong SetLocationIndicatorBearing(string layerId, double bearing)
    {
        ulong commandId = 0;
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_location_indicator_bearing(
                Handle,
                nativeLayerId.Value,
                bearing,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Sets a location indicator layer accuracy radius in logical pixels.</summary>
    public ulong SetLocationIndicatorAccuracyRadius(string layerId, double radius)
    {
        ulong commandId = 0;
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_location_indicator_accuracy_radius(
                Handle,
                nativeLayerId.Value,
                radius,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Sets a location indicator layer image-name property.</summary>
    public ulong SetLocationIndicatorImageName(
        string layerId,
        LocationIndicatorImageKind imageKind,
        string imageId
    )
    {
        ulong commandId = 0;
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_location_indicator_image_name(
                Handle,
                nativeLayerId.Value,
                (uint)imageKind,
                nativeImageId.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Adds a style layer and returns its command ID.</summary>
    public ulong AddStyleLayerJson(byte[] layerJson, string beforeLayerId)
    {
        using var nativeJson = NativeStringView.From(layerJson, nameof(layerJson));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_add_style_layer_json(
                Handle,
                nativeJson.Value,
                nativeBeforeLayerId.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Submits a command that removes one style layer and returns its command ID.</summary>
    /// <remarks>
    /// The command's <c>COMMAND_FINISHED</c> event reports
    /// <see cref="Error.MaplibreStatus.NotFound" /> when no layer has <paramref name="layerId" />.
    /// </remarks>
    public ulong RemoveStyleLayer(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_remove_style_layer(Handle, nativeLayerId.Value, &commandId)
        );
        return commandId;
    }

    internal Task<(
        LayerInfo Info,
        bool HasSourceId,
        bool HasSourceLayer
    )?> QueryStyleLayerInfoAsync(string layerId, CancellationToken cancellationToken)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_style_layer_info_start(
                    Handle,
                    nativeLayerId.Value,
                    operation
                ),
            operation =>
            {
                var info = new mln_style_layer_info { size = (uint)sizeof(mln_style_layer_info) };
                bool found = false;
                NativeStatus.Check(
                    NativeMethods.mln_map_get_style_layer_info_take_result(operation, &info, &found)
                );
                if (!found)
                {
                    return ((LayerInfo, bool, bool)?)null;
                }
                var fields = (mln_style_layer_info_field)info.fields;
                return (
                    new LayerInfo(
                        layerId,
                        RuntimeStructs.CopyUtf8((sbyte*)info.type.data, info.type.size),
                        info.min_zoom,
                        info.max_zoom,
                        (StyleLayerVisibility)info.visibility,
                        info.visibility,
                        null,
                        null
                    ),
                    fields.HasFlag(mln_style_layer_info_field.MLN_STYLE_LAYER_INFO_SOURCE_ID),
                    fields.HasFlag(mln_style_layer_info_field.MLN_STYLE_LAYER_INFO_SOURCE_LAYER)
                );
            },
            cancellationToken
        );
    }

    /// <summary>Lists style layer IDs in style order.</summary>
    public Task<string[]> StyleLayerIdsAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            operation => NativeMethods.mln_map_list_style_layer_ids_start(Handle, operation),
            operation =>
            {
                MlnStyleIdList list = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_list_style_layer_ids_take_result(operation, &list)
                );
                return CopyStyleIdList(list);
            },
            cancellationToken
        );

    /// <summary>Moves a style layer and returns its command ID.</summary>
    public ulong MoveStyleLayer(string layerId, string beforeLayerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_move_style_layer(
                Handle,
                nativeLayerId.Value,
                nativeBeforeLayerId.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Gets a full style-spec layer JSON snapshot when the layer exists.</summary>
    public Task<byte[]?> GetStyleLayerJsonAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_style_layer_json_start(
                    Handle,
                    nativeLayerId.Value,
                    operation
                ),
            operation =>
            {
                MlnBuffer buffer = default;
                bool found = false;
                NativeStatus.Check(
                    NativeMethods.mln_map_get_style_layer_json_take_result(
                        operation,
                        &buffer,
                        &found
                    )
                );
                return found ? ValueStructs.ReadBuffer(buffer) : null;
            },
            cancellationToken
        );
    }

    /// <summary>Sets the style light document and returns its command ID.</summary>
    public ulong SetStyleLightJson(byte[] lightJson)
    {
        using var nativeJson = NativeStringView.From(lightJson, nameof(lightJson));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_style_light_json(Handle, nativeJson.Value, &commandId)
        );
        return commandId;
    }

    /// <summary>Sets one style light property and returns its command ID.</summary>
    public ulong SetStyleLightProperty(string propertyName, byte[] value)
    {
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeStringView.From(value, nameof(value));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_style_light_property(
                Handle,
                nativePropertyName.Value,
                nativeValue.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Gets one style light property snapshot, or null when undefined.</summary>
    public Task<byte[]?> GetStyleLightPropertyAsync(
        string propertyName,
        CancellationToken cancellationToken = default
    )
    {
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_style_light_property_start(
                    Handle,
                    nativePropertyName.Value,
                    operation
                ),
            operation =>
            {
                MlnBuffer buffer = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_get_style_light_property_take_result(operation, &buffer)
                );
                return ValueStructs.ReadOptionalBuffer(buffer);
            },
            cancellationToken
        );
    }

    /// <summary>Sets the style's transition options and returns its command ID.</summary>
    public ulong SetStyleTransitionOptions(StyleTransitionOptions options)
    {
        var native = StyleStructs.ToNative(options);
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_style_transition_options(Handle, &native, &commandId)
        );
        return commandId;
    }

    /// <summary>Gets the style's global transition options.</summary>
    public Task<StyleTransitionOptions> GetStyleTransitionOptionsAsync(
        CancellationToken cancellationToken = default
    ) =>
        RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_style_transition_options_start(Handle, operation),
            operation =>
            {
                var options = NativeMethods.mln_style_transition_options_default();
                NativeStatus.Check(
                    NativeMethods.mln_map_get_style_transition_options_take_result(
                        operation,
                        &options
                    )
                );
                return StyleStructs.FromNative(options);
            },
            cancellationToken
        );

    /// <summary>Sets one layer property and returns its command ID.</summary>
    public ulong SetLayerProperty(string layerId, string propertyName, byte[] value)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeStringView.From(value, nameof(value));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_property(
                Handle,
                nativeLayerId.Value,
                nativePropertyName.Value,
                nativeValue.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Gets one layer property snapshot, or null when undefined.</summary>
    public Task<byte[]?> GetLayerPropertyAsync(
        string layerId,
        string propertyName,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_layer_property_start(
                    Handle,
                    nativeLayerId.Value,
                    nativePropertyName.Value,
                    operation
                ),
            operation =>
            {
                MlnBuffer buffer = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_get_layer_property_take_result(operation, &buffer)
                );
                return ValueStructs.ReadOptionalBuffer(buffer);
            },
            cancellationToken
        );
    }

    /// <summary>Sets or clears a layer filter and returns its command ID.</summary>
    public ulong SetLayerFilter(string layerId, byte[]? filter)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeFilter = filter is null
            ? null
            : NativeStringView.From(filter, nameof(filter));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_filter(
                Handle,
                nativeLayerId.Value,
                nativeFilter?.Pointer,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Gets one layer filter snapshot, or null when no filter exists.</summary>
    public Task<byte[]?> GetLayerFilterAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return RunMapOperationAsync(
            operation =>
                NativeMethods.mln_map_get_layer_filter_start(
                    Handle,
                    nativeLayerId.Value,
                    operation
                ),
            operation =>
            {
                MlnBuffer buffer = default;
                NativeStatus.Check(
                    NativeMethods.mln_map_get_layer_filter_take_result(operation, &buffer)
                );
                return ValueStructs.ReadOptionalBuffer(buffer);
            },
            cancellationToken
        );
    }

    /// <summary>Sets a layer's source-layer ID and returns its command ID.</summary>
    public ulong SetLayerSourceLayer(string layerId, string sourceLayer)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceLayer = NativeStringView.From(sourceLayer, nameof(sourceLayer));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_source_layer(
                Handle,
                nativeLayerId.Value,
                nativeSourceLayer.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Gets one layer's source-layer ID.</summary>
    public Task<string> GetLayerSourceLayerAsync(
        string layerId,
        CancellationToken cancellationToken = default
    ) => CopyLayerTextAsync(layerId, sourceLayer: true, cancellationToken);

    /// <summary>Sets a layer's source ID and returns its command ID.</summary>
    public ulong SetLayerSourceId(string layerId, string sourceId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_source_id(
                Handle,
                nativeLayerId.Value,
                nativeSourceId.Value,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Gets one layer's source ID.</summary>
    public Task<string> GetLayerSourceIdAsync(
        string layerId,
        CancellationToken cancellationToken = default
    ) => CopyLayerTextAsync(layerId, sourceLayer: false, cancellationToken);

    /// <summary>Sets the lowest layer zoom and returns its command ID.</summary>
    public ulong SetLayerMinZoom(string layerId, double minZoom)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_min_zoom(
                Handle,
                nativeLayerId.Value,
                minZoom,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Sets the highest layer zoom and returns its command ID.</summary>
    public ulong SetLayerMaxZoom(string layerId, double maxZoom)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_max_zoom(
                Handle,
                nativeLayerId.Value,
                maxZoom,
                &commandId
            )
        );
        return commandId;
    }

    /// <summary>Sets layer visibility and returns its command ID.</summary>
    public ulong SetLayerVisibility(string layerId, StyleLayerVisibility visibility)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        ulong commandId = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_visibility(
                Handle,
                nativeLayerId.Value,
                (uint)visibility,
                &commandId
            )
        );
        return commandId;
    }

    private static MlnOperation StartCreate(MlnRuntime runtime, mln_map_options options)
    {
        MlnOperation operation = default;
        NativeStatus.Check(NativeMethods.mln_map_create_start(runtime, &options, &operation));
        return operation;
    }

    private static MlnOperation StartMapOperation(MapOperationStart start)
    {
        MlnOperation operation = default;
        NativeStatus.Check(start(&operation));
        return operation;
    }

    private Task<TResult> RunMapOperationAsync<TResult>(
        MapOperationStart start,
        MapOperationTake<TResult> take,
        CancellationToken cancellationToken
    )
    {
        cancellationToken.ThrowIfCancellationRequested();
        var operation = StartMapOperation(start);
        return OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                RuntimeHandle.CheckOperationCompletion(operation);
                return take(operation);
            },
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    private Task<string?> CopyStyleSourceStringAsync(
        mln_buffer_view sourceId,
        bool attribution,
        CancellationToken cancellationToken
    ) =>
        RunMapOperationAsync(
            operation =>
                attribution
                    ? NativeMethods.mln_map_copy_style_source_attribution_start(
                        Handle,
                        sourceId,
                        operation
                    )
                    : NativeMethods.mln_map_copy_style_source_url_start(
                        Handle,
                        sourceId,
                        operation
                    ),
            operation =>
            {
                MlnBuffer buffer = default;
                bool found = false;
                NativeStatus.Check(
                    attribution
                        ? NativeMethods.mln_map_copy_style_source_attribution_take_result(
                            operation,
                            &buffer,
                            &found
                        )
                        : NativeMethods.mln_map_copy_style_source_url_take_result(
                            operation,
                            &buffer,
                            &found
                        )
                );
                return found
                    ? System.Text.Encoding.UTF8.GetString(ValueStructs.ReadBuffer(buffer))
                    : null;
            },
            cancellationToken
        );

    private Task<string> CopyLayerTextAsync(
        string layerId,
        bool sourceLayer,
        CancellationToken cancellationToken
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return RunMapOperationAsync(
            operation =>
                sourceLayer
                    ? NativeMethods.mln_map_copy_layer_source_layer_start(
                        Handle,
                        nativeLayerId.Value,
                        operation
                    )
                    : NativeMethods.mln_map_copy_layer_source_id_start(
                        Handle,
                        nativeLayerId.Value,
                        operation
                    ),
            operation =>
            {
                MlnBuffer buffer = default;
                NativeStatus.Check(
                    sourceLayer
                        ? NativeMethods.mln_map_copy_layer_source_layer_take_result(
                            operation,
                            &buffer
                        )
                        : NativeMethods.mln_map_copy_layer_source_id_take_result(operation, &buffer)
                );
                return System.Text.Encoding.UTF8.GetString(ValueStructs.ReadBuffer(buffer));
            },
            cancellationToken
        );
    }

    /// <summary>Closes the map after its previously accepted work completes.</summary>
    public Task CloseAsync(CancellationToken cancellationToken = default)
    {
        if (IsClosed)
        {
            return Task.CompletedTask;
        }
        cancellationToken.ThrowIfCancellationRequested();
        var operation = StartMapOperation(outOperation =>
            NativeMethods.mln_map_close_start(Handle, outOperation)
        );
        return OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation),
            () =>
            {
                RuntimeHandle.CheckOperationCompletion(operation);
                state.Close();
                runtime.UnregisterMap(this);
            },
            () => NativeMethods.mln_operation_release(operation)
        );
    }

    internal static IDisposable UseCustomGeometrySourceInstallForTest(
        MapAddCustomGeometrySource addCustomGeometrySource
    )
    {
        var previous = addCustomGeometrySourceForTest;
        addCustomGeometrySourceForTest = addCustomGeometrySource;
        return new RestoreCustomGeometrySourceInstall(previous);
    }

    private static MapAddCustomGeometrySource AddCustomGeometrySourceNative =>
        addCustomGeometrySourceForTest ?? DefaultAddCustomGeometrySource;

    private sealed class RestoreCustomGeometrySourceInstall(MapAddCustomGeometrySource? previous)
        : IDisposable
    {
        public void Dispose()
        {
            addCustomGeometrySourceForTest = previous;
        }
    }

    private static mln_lat_lng[] ToNativeCoordinates(
        IReadOnlyList<LatLng> coordinates,
        string parameterName
    )
    {
        ArgumentNullException.ThrowIfNull(coordinates, parameterName);
        var nativeCoordinates = new mln_lat_lng[coordinates.Count];
        for (var index = 0; index < coordinates.Count; index++)
        {
            nativeCoordinates[index] = CoreStructs.ToNative(coordinates[index]);
        }
        return nativeCoordinates;
    }

    private static string[] CopyStyleIdList(MlnStyleIdList list)
    {
        if (list.IsNull)
        {
            return [];
        }

        try
        {
            nuint count = 0;
            NativeStatus.Check(NativeMethods.mln_style_id_list_count(list, &count));
            var ids = new string[checked((int)count)];
            for (var index = 0; index < ids.Length; index++)
            {
                mln_buffer_view id = default;
                NativeStatus.Check(NativeMethods.mln_style_id_list_get(list, (nuint)index, &id));
                ids[index] = RuntimeStructs.CopyUtf8((sbyte*)id.data, id.size);
            }

            return ids;
        }
        finally
        {
            NativeMethods.mln_style_id_list_destroy(list);
        }
    }

    private static string[] CopyStyleStringList(MlnStyleStringList list)
    {
        if (list.IsNull)
        {
            return [];
        }

        try
        {
            nuint count = 0;
            NativeStatus.Check(NativeMethods.mln_style_string_list_count(list, &count));
            var values = new string[checked((int)count)];
            for (var index = 0; index < values.Length; index++)
            {
                mln_buffer_view value = default;
                NativeStatus.Check(
                    NativeMethods.mln_style_string_list_get(list, (nuint)index, &value)
                );
                values[index] = RuntimeStructs.CopyUtf8((sbyte*)value.data, value.size);
            }

            return values;
        }
        finally
        {
            NativeMethods.mln_style_string_list_destroy(list);
        }
    }

    private Task<CameraOptions> TakeCameraAsync(
        MlnOperation operation,
        MapTakeCameraResult take,
        CancellationToken cancellationToken
    ) =>
        OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                var camera = NativeMethods.mln_camera_options_default();
                NativeStatus.Check(take(operation, &camera));
                return MapStructs.CameraOptionsFromNative(camera);
            },
            () => NativeMethods.mln_operation_release(operation)
        );

    private Task<LatLngBounds> TakeBoundsAsync(
        MlnOperation operation,
        MapTakeBoundsResult take,
        CancellationToken cancellationToken
    ) =>
        OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                mln_lat_lng_bounds bounds = default;
                NativeStatus.Check(take(operation, &bounds));
                return MapStructs.FromNative(bounds);
            },
            () => NativeMethods.mln_operation_release(operation)
        );

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
                    NativeMethods.mln_map_pixel_for_lat_lng_take_result(operation, &point)
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
                    NativeMethods.mln_map_lat_lng_for_pixel_take_result(operation, &coordinate)
                );
                return CoreStructs.FromNative(coordinate);
            },
            () => NativeMethods.mln_operation_release(operation)
        );

    private Task<ScreenPoint[]> TakePointsAsync(
        MlnOperation operation,
        int count,
        CancellationToken cancellationToken
    ) =>
        OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                var native = new mln_screen_point[count];
                nuint actual = 0;
                fixed (mln_screen_point* pointer = native)
                {
                    NativeStatus.Check(
                        NativeMethods.mln_map_pixels_for_lat_lngs_take_result(
                            operation,
                            native.Length == 0 ? null : pointer,
                            (nuint)native.Length,
                            &actual
                        )
                    );
                }
                return native
                    .Take(checked((int)actual))
                    .Select(value => MapStructs.FromNative(value))
                    .ToArray();
            },
            () => NativeMethods.mln_operation_release(operation)
        );

    private Task<LatLng[]> TakeCoordinatesAsync(
        MlnOperation operation,
        int count,
        CancellationToken cancellationToken
    ) =>
        OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                var native = new mln_lat_lng[count];
                nuint actual = 0;
                fixed (mln_lat_lng* pointer = native)
                {
                    NativeStatus.Check(
                        NativeMethods.mln_map_lat_lngs_for_pixels_take_result(
                            operation,
                            native.Length == 0 ? null : pointer,
                            (nuint)native.Length,
                            &actual
                        )
                    );
                }
                return native
                    .Take(checked((int)actual))
                    .Select(value => CoreStructs.FromNative(value))
                    .ToArray();
            },
            () => NativeMethods.mln_operation_release(operation)
        );

    /// <inheritdoc />
    public void Dispose()
    {
        CloseAsync().ConfigureAwait(false).GetAwaiter().GetResult();
        GC.KeepAlive(runtime);
    }
}
