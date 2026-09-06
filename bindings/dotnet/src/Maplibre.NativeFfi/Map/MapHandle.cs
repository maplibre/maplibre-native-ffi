using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Map;

internal unsafe delegate mln_status MapAddCustomGeometrySource(
    MlnMap map,
    mln_buffer_view sourceId,
    mln_custom_geometry_source_options* options,
    mln_completion* completion
);

internal unsafe delegate mln_status MapAddCustomMvtVectorSource(
    MlnMap map,
    mln_buffer_view sourceId,
    mln_custom_mvt_vector_source_options* options,
    mln_completion* completion
);

/// <summary>Any-thread map handle bound to a runtime.</summary>
public sealed unsafe partial class MapHandle : IDisposable, IAsyncDisposable
{
    private static readonly MapAddCustomGeometrySource DefaultAddCustomGeometrySource = static (
        map,
        sourceId,
        options,
        completion
    ) => NativeMethods.mln_map_add_custom_geometry_source(map, sourceId, options, completion);

    private static readonly MapAddCustomMvtVectorSource DefaultAddCustomMvtVectorSource = static (
        map,
        sourceId,
        options,
        completion
    ) => NativeMethods.mln_map_add_custom_mvt_vector_source(map, sourceId, options, completion);

    [ThreadStatic]
    private static MapAddCustomGeometrySource? addCustomGeometrySourceForTest;

    [ThreadStatic]
    private static MapAddCustomMvtVectorSource? addCustomMvtVectorSourceForTest;

    private readonly RuntimeHandle runtime;
    private readonly ulong nativeId;
    private readonly NativeHandleState<MlnMap> state;
    private Task teardown = Task.CompletedTask;

    private MapHandle(RuntimeHandle runtime, MlnMap handle)
    {
        this.runtime = runtime;
        nativeId = handle.Value;
        state = new NativeHandleState<MlnMap>(handle, StartRelease, nameof(MapHandle));
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
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeOptions = options.ToNative();
                    return NativeMethods.mln_map_create(runtime.Handle, &nativeOptions, completion);
                },
                result =>
                {
                    var map = NativeCompletion.Value<MlnMap>(result);
                    var handle = new MapHandle(runtime, map);
                    runtime.RegisterMap(handle);
                    return handle;
                }
            )
            .WaitAsync(cancellationToken);
    }

    internal MlnMap Handle => state.Handle;

    internal RuntimeHandle Runtime => runtime;

    /// <summary>The issued native handle id, readable after close.</summary>
    internal ulong NativeId => nativeId;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Requests a repaint.</summary>
    public Task<CommandCompletion> RequestRepaintAsync()
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(NativeMethods.mln_map_request_repaint(Handle, completion));
        });
    }

    /// <summary>Requests a noncoalescing still-image render.</summary>
    public Task RequestStillImageAsync(CancellationToken cancellationToken = default)
    {
        return NativeCompletion
            .SubmitUnit(completion => NativeMethods.mln_map_request_still_image(Handle, completion))
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets native debug drawing options.</summary>
    /// <remarks>The committed mask is visible as <see cref="MapSnapshot.DebugOptions" />.</remarks>
    public Task<CommandCompletion> SetDebugOptionsAsync(DebugOptions options)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_debug_options(Handle, (uint)options, completion)
            );
        });
    }

    /// <summary>Shows or hides the built-in rendering statistics overlay.</summary>
    /// <remarks>
    /// The committed value is visible as <see cref="MapSnapshot.RenderingStatsViewEnabled" />.
    /// </remarks>
    public Task<CommandCompletion> SetRenderingStatsViewEnabledAsync(bool enabled)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_rendering_stats_view_enabled(
                    Handle,
                    enabled ? (byte)1 : (byte)0,
                    completion
                )
            );
        });
    }

    /// <summary>Asks the native map to write debug logs.</summary>
    public Task<CommandCompletion> DumpDebugLogsAsync()
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(NativeMethods.mln_map_dump_debug_logs(Handle, completion));
        });
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

    /// <summary>Submits a logical extent change.</summary>
    public Task<CommandCompletion> ResizeAsync(LogicalExtent extent)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_resize(
                    Handle,
                    new mln_logical_extent
                    {
                        width = extent.Width,
                        height = extent.Height,
                        scale_factor = extent.ScaleFactor,
                    },
                    completion
                )
            );
        });
    }

    /// <summary>Sets viewport options.</summary>
    /// <remarks>The committed options are visible as <see cref="MapSnapshot.Viewport" />.</remarks>
    public Task<CommandCompletion> SetViewportOptionsAsync(ViewportOptions options)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            var nativeOptions = MapStructs.ToNative(options);
            NativeStatus.Check(
                NativeMethods.mln_map_set_viewport_options(Handle, &nativeOptions, completion)
            );
        });
    }

    /// <summary>Sets tile tuning options.</summary>
    /// <remarks>The committed options are visible as <see cref="MapSnapshot.Tile" />.</remarks>
    public Task<CommandCompletion> SetTileOptionsAsync(TileOptions options)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            var nativeOptions = MapStructs.ToNative(options);
            NativeStatus.Check(
                NativeMethods.mln_map_set_tile_options(Handle, &nativeOptions, completion)
            );
        });
    }

    /// <summary>Gets a synchronous camera snapshot.</summary>
    public CameraSnapshot GetCameraSnapshot()
    {
        var camera = NativeMethods.mln_camera_options_default();
        ulong generation = 0;
        NativeStatus.Check(NativeMethods.mln_map_camera_snapshot_get(Handle, &camera, &generation));
        return new CameraSnapshot(MapStructs.CameraOptionsFromNative(camera), generation);
    }

    /// <summary>Submits a copied camera update.</summary>
    public Task<CommandCompletion> UpdateCameraAsync(CameraUpdate update)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            var native = MapStructs.ToNative(update);
            NativeStatus.Check(NativeMethods.mln_map_update_camera(Handle, &native, completion));
        });
    }

    /// <summary>Submits one relative camera operation.</summary>
    public Task<CommandCompletion> ApplyCameraDeltaAsync(CameraDelta delta)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            var native = MapStructs.ToNative(delta);
            NativeStatus.Check(
                NativeMethods.mln_map_apply_camera_delta(Handle, &native, completion)
            );
        });
    }

    /// <summary>Reads the camera in runtime order.</summary>
    public Task<CameraSnapshot> QueryCameraAsync(CancellationToken cancellationToken = default)
    {
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_camera_query(Handle, completion),
                result =>
                {
                    var value = NativeCompletion.Value<mln_camera_query_result>(result);
                    return new CameraSnapshot(
                        MapStructs.CameraOptionsFromNative(value.camera),
                        value.generation
                    );
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CameraOptions> CameraForLatLngBoundsAsync(
        LatLngBounds bounds,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        return RunMapOperationAsync(
            completion =>
            {
                var nativeBounds = MapStructs.ToNative(bounds);
                var nativeFitOptions = fitOptions is null
                    ? default
                    : MapStructs.ToNative(fitOptions);
                return NativeMethods.mln_map_camera_for_lat_lng_bounds(
                    Handle,
                    nativeBounds,
                    fitOptions is null ? null : &nativeFitOptions,
                    completion
                );
            },
            ReadCamera,
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
        return RunMapOperationAsync(
            completion =>
            {
                var nativeFitOptions = fitOptions is null
                    ? default
                    : MapStructs.ToNative(fitOptions);
                fixed (mln_lat_lng* pointer = nativeCoordinates)
                {
                    return NativeMethods.mln_map_camera_for_lat_lngs(
                        Handle,
                        nativeCoordinates.Length == 0 ? null : pointer,
                        (nuint)nativeCoordinates.Length,
                        fitOptions is null ? null : &nativeFitOptions,
                        completion
                    );
                }
            },
            ReadCamera,
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
        return RunMapOperationAsync(
            completion =>
            {
                var nativeFitOptions = fitOptions is null
                    ? default
                    : MapStructs.ToNative(fitOptions);
                return NativeMethods.mln_map_camera_for_geometry(
                    Handle,
                    nativeGeometry.Value,
                    fitOptions is null ? null : &nativeFitOptions,
                    completion
                );
            },
            ReadCamera,
            cancellationToken
        );
    }

    public Task<LatLngBounds> LatLngBoundsForCameraAsync(
        CameraOptions camera,
        CancellationToken cancellationToken = default
    )
    {
        return RunMapOperationAsync(
            completion =>
            {
                var nativeCamera = MapStructs.ToNative(camera);
                return NativeMethods.mln_map_lat_lng_bounds_for_camera(
                    Handle,
                    &nativeCamera,
                    completion
                );
            },
            ReadBounds,
            cancellationToken
        );
    }

    public Task<LatLngBounds> LatLngBoundsForCameraUnwrappedAsync(
        CameraOptions camera,
        CancellationToken cancellationToken = default
    )
    {
        return RunMapOperationAsync(
            completion =>
            {
                var nativeCamera = MapStructs.ToNative(camera);
                return NativeMethods.mln_map_lat_lng_bounds_for_camera_unwrapped(
                    Handle,
                    &nativeCamera,
                    completion
                );
            },
            ReadBounds,
            cancellationToken
        );
    }

    /// <summary>Sets map bounds constraints.</summary>
    /// <remarks>The committed constraints are visible as <see cref="MapSnapshot.Bounds" />.</remarks>
    public Task<CommandCompletion> SetBoundsAsync(BoundOptions options)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            var nativeOptions = MapStructs.ToNative(options);
            NativeStatus.Check(
                NativeMethods.mln_map_set_bounds(Handle, &nativeOptions, completion)
            );
        });
    }

    /// <summary>Sets free-camera options.</summary>
    /// <remarks>The committed options are visible as <see cref="MapSnapshot.FreeCamera" />.</remarks>
    public Task<CommandCompletion> SetFreeCameraOptionsAsync(FreeCameraOptions options)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            var nativeOptions = MapStructs.ToNative(options);
            NativeStatus.Check(
                NativeMethods.mln_map_set_free_camera_options(Handle, &nativeOptions, completion)
            );
        });
    }

    public Task<ScreenPoint> PixelForLatLngAsync(
        LatLng coordinate,
        CancellationToken cancellationToken = default
    )
    {
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_pixel_for_lat_lng(
                    Handle,
                    CoreStructs.ToNative(coordinate),
                    completion
                ),
            result => MapStructs.FromNative(NativeCompletion.Value<mln_screen_point>(result)),
            cancellationToken
        );
    }

    /// <summary>Converts a screen pixel to a geographic coordinate using the current map projection.</summary>
    /// <remarks>The longitude is wrapped to the range from -180 to 180 degrees.</remarks>
    public Task<LatLng> LatLngForPixelAsync(
        ScreenPoint point,
        CancellationToken cancellationToken = default
    )
    {
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_lat_lng_for_pixel(
                    Handle,
                    MapStructs.ToNative(point),
                    completion
                ),
            result => CoreStructs.FromNative(NativeCompletion.Value<mln_lat_lng>(result)),
            cancellationToken
        );
    }

    /// <summary>Converts a screen pixel to an unwrapped geographic coordinate.</summary>
    /// <remarks>The longitude preserves the visible world copy and may fall outside -180 to 180.</remarks>
    public Task<LatLng> LatLngForPixelUnwrappedAsync(
        ScreenPoint point,
        CancellationToken cancellationToken = default
    )
    {
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_lat_lng_for_pixel_unwrapped(
                    Handle,
                    MapStructs.ToNative(point),
                    completion
                ),
            result => CoreStructs.FromNative(NativeCompletion.Value<mln_lat_lng>(result)),
            cancellationToken
        );
    }

    public Task<ScreenPoint[]> PixelsForLatLngsAsync(
        IReadOnlyList<LatLng> coordinates,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(coordinates);
        var native = coordinates.Select(value => CoreStructs.ToNative(value)).ToArray();
        return RunMapOperationAsync(
            completion =>
            {
                fixed (mln_lat_lng* pointer = native)
                {
                    return NativeMethods.mln_map_pixels_for_lat_lngs(
                        Handle,
                        native.Length == 0 ? null : pointer,
                        (nuint)native.Length,
                        completion
                    );
                }
            },
            ReadPoints,
            cancellationToken
        );
    }

    /// <summary>Converts screen pixels to geographic coordinates using the current map projection.</summary>
    /// <remarks>Each longitude is wrapped to the range from -180 to 180 degrees.</remarks>
    public Task<LatLng[]> LatLngsForPixelsAsync(
        IReadOnlyList<ScreenPoint> points,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(points);
        var native = points.Select(value => MapStructs.ToNative(value)).ToArray();
        return RunMapOperationAsync(
            completion =>
            {
                fixed (mln_screen_point* pointer = native)
                {
                    return NativeMethods.mln_map_lat_lngs_for_pixels(
                        Handle,
                        native.Length == 0 ? null : pointer,
                        (nuint)native.Length,
                        completion
                    );
                }
            },
            ReadCoordinates,
            cancellationToken
        );
    }

    /// <summary>Converts screen pixels to unwrapped geographic coordinates.</summary>
    /// <remarks>Each longitude preserves its visible world copy and may fall outside -180 to 180.</remarks>
    public Task<LatLng[]> LatLngsForPixelsUnwrappedAsync(
        IReadOnlyList<ScreenPoint> points,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(points);
        var native = points.Select(value => MapStructs.ToNative(value)).ToArray();
        return RunMapOperationAsync(
            completion =>
            {
                fixed (mln_screen_point* pointer = native)
                {
                    return NativeMethods.mln_map_lat_lngs_for_pixels_unwrapped(
                        Handle,
                        native.Length == 0 ? null : pointer,
                        (nuint)native.Length,
                        completion
                    );
                }
            },
            ReadCoordinates,
            cancellationToken
        );
    }

    /// <summary>Creates a standalone projection in runtime order.</summary>
    public Task<MapProjectionHandle> CreateProjectionAsync(
        CancellationToken cancellationToken = default
    ) => MapProjectionHandle.CreateAsync(this, cancellationToken);

    /// <summary>Sets projection mode options.</summary>
    public Task<CommandCompletion> SetProjectionModeAsync(ProjectionModeOptions mode)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            var nativeMode = MapStructs.ToNative(mode);
            NativeStatus.Check(
                NativeMethods.mln_map_set_projection_mode(Handle, &nativeMode, completion)
            );
        });
    }

    /// <summary>Loads a style URL.</summary>
    public Task<CommandCompletion> SetStyleUrlAsync(string url)
    {
        ArgumentNullException.ThrowIfNull(url);
        using var nativeUrl = NativeUtf8String.FromNullableString(url, nameof(url));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_style_url(Handle, nativeUrl.Pointer, completion)
            );
        });
    }

    /// <summary>Loads inline style JSON.</summary>
    public Task<CommandCompletion> SetStyleJsonAsync(byte[] json)
    {
        ArgumentNullException.ThrowIfNull(json);
        using var nativeJson = NativeStringView.From(json, nameof(json));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_style_json(Handle, nativeJson.Value, completion)
            );
        });
    }

    /// <summary>Sets per-feature state on this map.</summary>
    public Task<CommandCompletion> SetFeatureStateAsync(FeatureStateSelector selector, byte[] state)
    {
        ArgumentNullException.ThrowIfNull(selector);
        ArgumentNullException.ThrowIfNull(state);
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSelector = NativeFeatureStateSelector.From(selector);
            using var nativeState = NativeStringView.From(state, nameof(state));
            var selectorValue = nativeSelector.Value;
            NativeStatus.Check(
                NativeMethods.mln_map_set_feature_state(
                    Handle,
                    &selectorValue,
                    nativeState.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Copies per-feature state from this map.</summary>
    public Task<byte[]> GetFeatureStateAsync(
        FeatureStateSelector selector,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(selector);
        return RunMapOperationAsync(
            completion =>
            {
                using var nativeSelector = NativeFeatureStateSelector.From(selector);
                var selectorValue = nativeSelector.Value;
                return NativeMethods.mln_map_get_feature_state(Handle, &selectorValue, completion);
            },
            ReadBuffer,
            cancellationToken
        );
    }

    /// <summary>Removes per-feature state from this map.</summary>
    public Task<CommandCompletion> RemoveFeatureStateAsync(FeatureStateSelector selector)
    {
        ArgumentNullException.ThrowIfNull(selector);
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSelector = NativeFeatureStateSelector.From(selector);
            var selectorValue = nativeSelector.Value;
            NativeStatus.Check(
                NativeMethods.mln_map_remove_feature_state(Handle, &selectorValue, completion)
            );
        });
    }

    /// <summary>Gets the style document that this map's style was last parsed from.</summary>
    public Task<byte[]> GetLoadedStyleJsonAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            completion => NativeMethods.mln_map_loaded_style_json(Handle, completion),
            ReadBuffer,
            cancellationToken
        );

    /// <summary>Gets the URL that this map's style was last requested from.</summary>
    public Task<string> GetStyleUrlAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            completion => NativeMethods.mln_map_style_url(Handle, completion),
            result => System.Text.Encoding.UTF8.GetString(ReadBuffer(result)),
            cancellationToken
        );

    /// <summary>Selects which map-originated event types this map queues.</summary>
    public Task<CommandCompletion> SetEventMaskAsync(RuntimeEventMask mask)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_event_mask(Handle, (ulong)mask, completion)
            );
        });
    }

    /// <summary>Reports the event mask in the latest committed map snapshot.</summary>
    public RuntimeEventMask GetEventMask() => GetSnapshot().EventMask;

    public Task<CommandCompletion> AddStyleSourceJsonAsync(string sourceId, byte[] sourceJson)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeJson = NativeStringView.From(sourceJson, nameof(sourceJson));
            NativeStatus.Check(
                NativeMethods.mln_map_add_style_source_json(
                    Handle,
                    nativeSourceId.Value,
                    nativeJson.Value,
                    completion
                )
            );
        });
    }

    public Task<CommandCompletion> RemoveStyleSourceAsync(string sourceId)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            NativeStatus.Check(
                NativeMethods.mln_map_remove_style_source(Handle, nativeSourceId.Value, completion)
            );
        });
    }

    /// <summary>Sets whether a style source stores fetched tiles in persistent storage.</summary>
    /// <remarks>The command fails with a not-found status when the source does not exist.</remarks>
    public Task<CommandCompletion> SetStyleSourceVolatileAsync(string sourceId, bool isVolatile)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            NativeStatus.Check(
                NativeMethods.mln_map_set_style_source_volatile(
                    Handle,
                    nativeSourceId.Value,
                    isVolatile ? (byte)1 : (byte)0,
                    completion
                )
            );
        });
    }

    /// <summary>Lists style source IDs in style order.</summary>
    public Task<string[]> StyleSourceIdsAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            completion => NativeMethods.mln_map_list_style_source_ids(Handle, completion),
            ReadStrings,
            cancellationToken
        );

    /// <summary>Adds a GeoJSON source that loads data from a URL.</summary>
    /// <remarks>
    /// <paramref name="options" /> is fixed when the source is created;
    /// <see cref="SetGeoJsonSourceUrlAsync" /> and <see cref="SetGeoJsonSourceDataAsync" /> keep it.
    /// </remarks>
    public Task<CommandCompletion> AddGeoJsonSourceUrlAsync(
        string sourceId,
        string url,
        GeoJsonSourceOptions? options
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeUrl = NativeStringView.From(url, nameof(url));
            using var nativeOptions = options is null
                ? null
                : NativeGeoJsonSourceOptions.From(options);
            var optionsValue = nativeOptions?.Value ?? default;
            NativeStatus.Check(
                NativeMethods.mln_map_add_geojson_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                )
            );
        });
    }

    /// <summary>Updates a GeoJSON source to load data from a URL.</summary>
    public Task<CommandCompletion> SetGeoJsonSourceUrlAsync(string sourceId, string url)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeUrl = NativeStringView.From(url, nameof(url));
            NativeStatus.Check(
                NativeMethods.mln_map_set_geojson_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a GeoJSON source backed by prepared data.</summary>
    /// <remarks>
    /// The call borrows <paramref name="data" />; the source adopts the options baked into it
    /// when the data was prepared and keeps its own reference, so the handle may be released
    /// afterward.
    /// </remarks>
    public Task<CommandCompletion> AddGeoJsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data
    )
    {
        ArgumentNullException.ThrowIfNull(data);
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            data.WithLive(prepared =>
            {
                NativeStatus.Check(
                    NativeMethods.mln_map_add_geojson_source_data(
                        Handle,
                        nativeSourceId.Value,
                        prepared,
                        completion
                    )
                );
            });
        });
    }

    /// <summary>Updates a GeoJSON source with prepared data.</summary>
    /// <remarks>
    /// The call borrows <paramref name="data" />; the command fails when the options baked into
    /// it differ from the source's, except for cluster properties.
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data
    )
    {
        ArgumentNullException.ThrowIfNull(data);
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            data.WithLive(prepared =>
            {
                NativeStatus.Check(
                    NativeMethods.mln_map_set_geojson_source_data(
                        Handle,
                        nativeSourceId.Value,
                        prepared,
                        completion
                    )
                );
            });
        });
    }

    /// <summary>Overrides synchronous tiling for a GeoJSON source.</summary>
    /// <remarks>
    /// The effective behavior is the source's baked-in option OR this override.
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceSynchronousTilingAsync(
        string sourceId,
        bool enabled
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            NativeStatus.Check(
                NativeMethods.mln_map_set_geojson_source_synchronous_tiling(
                    Handle,
                    nativeSourceId.Value,
                    enabled ? (byte)1 : (byte)0,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a custom geometry source with tile callbacks.</summary>
    /// <remarks>
    /// The upcall stubs this installs live until MapLibre stops referencing them: until the source
    /// is removed, until a style load leaves a style without it, or until this map closes. The
    /// binding releases them from the callback the C API invokes then, so nothing here depends on
    /// the events <see cref="SetEventMaskAsync" /> selects.
    /// </remarks>
    public Task<CommandCompletion> AddCustomGeometrySourceAsync(
        string sourceId,
        CustomGeometrySourceOptions options
    )
    {
        ArgumentNullException.ThrowIfNull(options);
        return AddCustomGeometrySourceAsync(sourceId, new CustomGeometrySourceState(options));
    }

    internal Task<CommandCompletion> AddCustomGeometrySourceAsync(
        string sourceId,
        CustomGeometrySourceState sourceState
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        try
        {
            return NativeCompletion.SubmitCommandChecked(completion =>
            {
                var descriptor = sourceState.Descriptor;
                NativeStatus.Check(
                    AddCustomGeometrySourceNative(
                        Handle,
                        nativeSourceId.Value,
                        &descriptor,
                        completion
                    )
                );
            });
        }
        catch
        {
            sourceState.Dispose();
            throw;
        }
    }

    /// <summary>Sets custom geometry source tile data.</summary>
    public Task<CommandCompletion> SetCustomGeometrySourceTileDataAsync(
        string sourceId,
        CanonicalTileId tileId,
        byte[] data
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeData = NativeStringView.From(data, nameof(data));
            var nativeTileId = StyleStructs.ToNative(tileId);
            NativeStatus.Check(
                NativeMethods.mln_map_set_custom_geometry_source_tile_data(
                    Handle,
                    nativeSourceId.Value,
                    nativeTileId,
                    nativeData.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Invalidates one custom geometry source tile.</summary>
    public Task<CommandCompletion> InvalidateCustomGeometrySourceTileAsync(
        string sourceId,
        CanonicalTileId tileId
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            NativeStatus.Check(
                NativeMethods.mln_map_invalidate_custom_geometry_source_tile(
                    Handle,
                    nativeSourceId.Value,
                    StyleStructs.ToNative(tileId),
                    completion
                )
            );
        });
    }

    /// <summary>Invalidates custom geometry source tiles that intersect bounds.</summary>
    public Task<CommandCompletion> InvalidateCustomGeometrySourceRegionAsync(
        string sourceId,
        LatLngBounds bounds
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            NativeStatus.Check(
                NativeMethods.mln_map_invalidate_custom_geometry_source_region(
                    Handle,
                    nativeSourceId.Value,
                    MapStructs.ToNative(bounds),
                    completion
                )
            );
        });
    }

    /// <summary>Adds a custom MVT vector source with tile callbacks.</summary>
    /// <remarks>
    /// The upcall stubs this installs live until MapLibre stops referencing them: until the source
    /// is removed, until a style load leaves a style without it, or until this map closes. The
    /// binding releases them from the callback the C API invokes then, so nothing here depends on
    /// the events <see cref="SetEventMaskAsync" /> selects.
    /// </remarks>
    public Task<CommandCompletion> AddCustomMvtVectorSourceAsync(
        string sourceId,
        CustomMvtVectorSourceOptions options
    )
    {
        ArgumentNullException.ThrowIfNull(options);
        return AddCustomMvtVectorSourceAsync(sourceId, new CustomMvtVectorSourceState(options));
    }

    internal Task<CommandCompletion> AddCustomMvtVectorSourceAsync(
        string sourceId,
        CustomMvtVectorSourceState sourceState
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        try
        {
            return NativeCompletion.SubmitCommandChecked(completion =>
            {
                var descriptor = sourceState.Descriptor;
                NativeStatus.Check(
                    AddCustomMvtVectorSourceNative(
                        Handle,
                        nativeSourceId.Value,
                        &descriptor,
                        completion
                    )
                );
            });
        }
        catch
        {
            // A rejected add never referenced the state, so no release callback is owed for it.
            sourceState.Dispose();
            throw;
        }
    }

    /// <summary>Sets custom MVT vector source tile data.</summary>
    public Task<CommandCompletion> SetCustomMvtVectorSourceTileDataAsync(
        string sourceId,
        CanonicalTileId tileId,
        byte[] data
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeData = NativeStringView.From(data, nameof(data));
            var nativeTileId = StyleStructs.ToNative(tileId);
            NativeStatus.Check(
                NativeMethods.mln_map_set_custom_mvt_vector_source_tile_data(
                    Handle,
                    nativeSourceId.Value,
                    nativeTileId,
                    nativeData.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Reports a custom MVT vector source error for one tile.</summary>
    public Task<CommandCompletion> SetCustomMvtVectorSourceTileErrorAsync(
        string sourceId,
        CanonicalTileId tileId,
        string message
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeMessage = NativeStringView.From(message, nameof(message));
            var nativeTileId = StyleStructs.ToNative(tileId);
            NativeStatus.Check(
                NativeMethods.mln_map_set_custom_mvt_vector_source_tile_error(
                    Handle,
                    nativeSourceId.Value,
                    nativeTileId,
                    nativeMessage.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Invalidates one custom MVT vector source tile.</summary>
    public Task<CommandCompletion> InvalidateCustomMvtVectorSourceTileAsync(
        string sourceId,
        CanonicalTileId tileId
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            NativeStatus.Check(
                NativeMethods.mln_map_invalidate_custom_mvt_vector_source_tile(
                    Handle,
                    nativeSourceId.Value,
                    StyleStructs.ToNative(tileId),
                    completion
                )
            );
        });
    }

    /// <summary>Adds a vector source that loads TileJSON from a URL.</summary>
    public Task<CommandCompletion> AddVectorSourceUrlAsync(
        string sourceId,
        string url,
        TileSourceOptions? options
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeUrl = NativeStringView.From(url, nameof(url));
            using var nativeOptions = options is null
                ? null
                : NativeTileSourceOptions.From(options);
            var optionsValue = nativeOptions?.Value ?? default;
            NativeStatus.Check(
                NativeMethods.mln_map_add_vector_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a vector source from inline tile URL templates.</summary>
    public Task<CommandCompletion> AddVectorSourceTilesAsync(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
            using var nativeOptions = options is null
                ? null
                : NativeTileSourceOptions.From(options);
            var optionsValue = nativeOptions?.Value ?? default;
            NativeStatus.Check(
                NativeMethods.mln_map_add_vector_source_tiles(
                    Handle,
                    nativeSourceId.Value,
                    nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                    nativeTiles.Count,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a raster source that loads TileJSON from a URL.</summary>
    public Task<CommandCompletion> AddRasterSourceUrlAsync(
        string sourceId,
        string url,
        TileSourceOptions? options
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeUrl = NativeStringView.From(url, nameof(url));
            using var nativeOptions = options is null
                ? null
                : NativeTileSourceOptions.From(options);
            var optionsValue = nativeOptions?.Value ?? default;
            NativeStatus.Check(
                NativeMethods.mln_map_add_raster_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a raster source from inline tile URL templates.</summary>
    public Task<CommandCompletion> AddRasterSourceTilesAsync(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
            using var nativeOptions = options is null
                ? null
                : NativeTileSourceOptions.From(options);
            var optionsValue = nativeOptions?.Value ?? default;
            NativeStatus.Check(
                NativeMethods.mln_map_add_raster_source_tiles(
                    Handle,
                    nativeSourceId.Value,
                    nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                    nativeTiles.Count,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a raster DEM source that loads TileJSON from a URL.</summary>
    public Task<CommandCompletion> AddRasterDemSourceUrlAsync(
        string sourceId,
        string url,
        TileSourceOptions? options
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeUrl = NativeStringView.From(url, nameof(url));
            using var nativeOptions = options is null
                ? null
                : NativeTileSourceOptions.From(options);
            var optionsValue = nativeOptions?.Value ?? default;
            NativeStatus.Check(
                NativeMethods.mln_map_add_raster_dem_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a raster DEM source from inline tile URL templates.</summary>
    public Task<CommandCompletion> AddRasterDemSourceTilesAsync(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
            using var nativeOptions = options is null
                ? null
                : NativeTileSourceOptions.From(options);
            var optionsValue = nativeOptions?.Value ?? default;
            NativeStatus.Check(
                NativeMethods.mln_map_add_raster_dem_source_tiles(
                    Handle,
                    nativeSourceId.Value,
                    nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                    nativeTiles.Count,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                )
            );
        });
    }

    /// <summary>Sets or replaces a style image.</summary>
    public Task<CommandCompletion> SetStyleImageAsync(
        string imageId,
        PremultipliedRgba8Image image,
        StyleImageOptions? options
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
            using var nativeImage = NativeStyleImage.From(image);
            var imageValue = nativeImage.Value;
            using var nativeOptions = options is null
                ? null
                : NativeStyleImageOptions.From(options);
            var optionsValue = nativeOptions?.Value ?? default;
            NativeStatus.Check(
                NativeMethods.mln_map_set_style_image(
                    Handle,
                    nativeImageId.Value,
                    &imageValue,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                )
            );
        });
    }

    /// <summary>Removes a runtime style image.</summary>
    public Task<CommandCompletion> RemoveStyleImageAsync(string imageId)
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion.SubmitCommandChecked(completion =>
            NativeStatus.Check(
                NativeMethods.mln_map_remove_style_image(Handle, nativeImageId.Value, completion)
            )
        );
    }

    /// <summary>Submits a command that removes one runtime style image.</summary>
    /// <remarks>
    /// The command's <c>command completion</c> event reports
    /// <see cref="Error.MaplibreStatus.NotFound" /> when no image has <paramref name="imageId" />.
    /// </remarks>
    private static IReadOnlyList<ImageStretch> ToStretches(mln_image_stretch[] raw) =>
        Array.ConvertAll(raw, stretch => new ImageStretch(stretch.from, stretch.to));

    /// <summary>Adds an image source that loads image data from a URL.</summary>
    public Task<CommandCompletion> AddImageSourceUrlAsync(
        string sourceId,
        IReadOnlyList<LatLng> coordinates,
        string url
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
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
                        completion
                    )
                );
            }
        });
    }

    /// <summary>Adds an image source with inline premultiplied RGBA8 image data.</summary>
    public Task<CommandCompletion> AddImageSourceImageAsync(
        string sourceId,
        IReadOnlyList<LatLng> coordinates,
        PremultipliedRgba8Image image
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
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
                        completion
                    )
                );
            }
        });
    }

    /// <summary>Updates an image source to load image data from a URL.</summary>
    public Task<CommandCompletion> SetImageSourceUrlAsync(string sourceId, string url)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeUrl = NativeStringView.From(url, nameof(url));
            NativeStatus.Check(
                NativeMethods.mln_map_set_image_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Updates an image source with inline premultiplied RGBA8 image data.</summary>
    public Task<CommandCompletion> SetImageSourceImageAsync(
        string sourceId,
        PremultipliedRgba8Image image
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeImage = NativeStyleImage.From(image);
            var imageValue = nativeImage.Value;
            NativeStatus.Check(
                NativeMethods.mln_map_set_image_source_image(
                    Handle,
                    nativeSourceId.Value,
                    &imageValue,
                    completion
                )
            );
        });
    }

    /// <summary>Updates image source coordinates.</summary>
    public Task<CommandCompletion> SetImageSourceCoordinatesAsync(
        string sourceId,
        IReadOnlyList<LatLng> coordinates
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
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
                        completion
                    )
                );
            }
        });
    }

    /// <summary>Gets image source coordinates when the source exists.</summary>
    public Task<LatLng[]?> GetImageSourceCoordinatesAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_get_image_source_coordinates(
                    Handle,
                    nativeSourceId.Value,
                    completion
                ),
            result => result->value_count == 0 ? null : ReadCoordinates(result),
            cancellationToken
        );
    }

    /// <summary>Adds a hillshade layer for a raster DEM source.</summary>
    public Task<CommandCompletion> AddHillshadeLayerAsync(
        string layerId,
        string sourceId,
        string beforeLayerId
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeBeforeLayerId = NativeStringView.From(
                beforeLayerId,
                nameof(beforeLayerId)
            );
            NativeStatus.Check(
                NativeMethods.mln_map_add_hillshade_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a color-relief layer for a raster DEM source.</summary>
    public Task<CommandCompletion> AddColorReliefLayerAsync(
        string layerId,
        string sourceId,
        string beforeLayerId
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            using var nativeBeforeLayerId = NativeStringView.From(
                beforeLayerId,
                nameof(beforeLayerId)
            );
            NativeStatus.Check(
                NativeMethods.mln_map_add_color_relief_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a source-free location indicator layer.</summary>
    public Task<CommandCompletion> AddLocationIndicatorLayerAsync(
        string layerId,
        string beforeLayerId
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
            using var nativeBeforeLayerId = NativeStringView.From(
                beforeLayerId,
                nameof(beforeLayerId)
            );
            NativeStatus.Check(
                NativeMethods.mln_map_add_location_indicator_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Sets a location indicator layer location.</summary>
    public Task<CommandCompletion> SetLocationIndicatorLocationAsync(
        string layerId,
        LatLng coordinate,
        double altitude
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
            NativeStatus.Check(
                NativeMethods.mln_map_set_location_indicator_location(
                    Handle,
                    nativeLayerId.Value,
                    CoreStructs.ToNative(coordinate),
                    altitude,
                    completion
                )
            );
        });
    }

    /// <summary>Sets a location indicator layer bearing in degrees.</summary>
    public Task<CommandCompletion> SetLocationIndicatorBearingAsync(string layerId, double bearing)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
            NativeStatus.Check(
                NativeMethods.mln_map_set_location_indicator_bearing(
                    Handle,
                    nativeLayerId.Value,
                    bearing,
                    completion
                )
            );
        });
    }

    /// <summary>Sets a location indicator layer accuracy radius in meters.</summary>
    public Task<CommandCompletion> SetLocationIndicatorAccuracyRadiusAsync(
        string layerId,
        double radius
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
            NativeStatus.Check(
                NativeMethods.mln_map_set_location_indicator_accuracy_radius(
                    Handle,
                    nativeLayerId.Value,
                    radius,
                    completion
                )
            );
        });
    }

    /// <summary>Sets a location indicator layer image-name property.</summary>
    public Task<CommandCompletion> SetLocationIndicatorImageNameAsync(
        string layerId,
        LocationIndicatorImageKind imageKind,
        string imageId
    )
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
            using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
            NativeStatus.Check(
                NativeMethods.mln_map_set_location_indicator_image_name(
                    Handle,
                    nativeLayerId.Value,
                    (uint)imageKind,
                    nativeImageId.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Adds a style layer.</summary>
    public Task<CommandCompletion> AddStyleLayerJsonAsync(byte[] layerJson, string beforeLayerId)
    {
        using var nativeJson = NativeStringView.From(layerJson, nameof(layerJson));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_add_style_layer_json(
                    Handle,
                    nativeJson.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Submits a command that removes one style layer.</summary>
    /// <remarks>
    /// The command's <c>command completion</c> event reports
    /// <see cref="Error.MaplibreStatus.NotFound" /> when no layer has <paramref name="layerId" />.
    /// </remarks>
    public Task<CommandCompletion> RemoveStyleLayerAsync(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_remove_style_layer(Handle, nativeLayerId.Value, completion)
            );
        });
    }

    /// <summary>Lists style layer IDs in style order.</summary>
    public Task<string[]> StyleLayerIdsAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            completion => NativeMethods.mln_map_list_style_layer_ids(Handle, completion),
            ReadStrings,
            cancellationToken
        );

    /// <summary>Moves a style layer.</summary>
    public Task<CommandCompletion> MoveStyleLayerAsync(string layerId, string beforeLayerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_move_style_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Gets a full style-spec layer JSON snapshot when the layer exists.</summary>
    public Task<byte[]?> GetStyleLayerJsonAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_get_style_layer_json(Handle, nativeLayerId.Value, completion),
            ReadOptionalBuffer,
            cancellationToken
        );
    }

    /// <summary>Sets the style light document.</summary>
    public Task<CommandCompletion> SetStyleLightJsonAsync(byte[] lightJson)
    {
        using var nativeJson = NativeStringView.From(lightJson, nameof(lightJson));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_style_light_json(Handle, nativeJson.Value, completion)
            );
        });
    }

    /// <summary>Sets one style light property.</summary>
    public Task<CommandCompletion> SetStyleLightPropertyAsync(string propertyName, byte[] value)
    {
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeStringView.From(value, nameof(value));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_style_light_property(
                    Handle,
                    nativePropertyName.Value,
                    nativeValue.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Gets one style light property snapshot, or null when undefined.</summary>
    public Task<byte[]?> GetStyleLightPropertyAsync(
        string propertyName,
        CancellationToken cancellationToken = default
    )
    {
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_get_style_light_property(
                    Handle,
                    nativePropertyName.Value,
                    completion
                ),
            ReadOptionalBuffer,
            cancellationToken
        );
    }

    /// <summary>Sets the style's transition options.</summary>
    public Task<CommandCompletion> SetStyleTransitionOptionsAsync(StyleTransitionOptions options)
    {
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            var native = StyleStructs.ToNative(options);
            NativeStatus.Check(
                NativeMethods.mln_map_set_style_transition_options(Handle, &native, completion)
            );
        });
    }

    /// <summary>Gets the style's global transition options.</summary>
    public Task<StyleTransitionOptions> GetStyleTransitionOptionsAsync(
        CancellationToken cancellationToken = default
    ) =>
        RunMapOperationAsync(
            completion => NativeMethods.mln_map_get_style_transition_options(Handle, completion),
            result =>
                StyleStructs.FromNative(
                    NativeCompletion.Value<mln_style_transition_options>(result)
                ),
            cancellationToken
        );

    /// <summary>Sets one layer property.</summary>
    public Task<CommandCompletion> SetLayerPropertyAsync(
        string layerId,
        string propertyName,
        byte[] value
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeStringView.From(value, nameof(value));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_layer_property(
                    Handle,
                    nativeLayerId.Value,
                    nativePropertyName.Value,
                    nativeValue.Value,
                    completion
                )
            );
        });
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
            completion =>
                NativeMethods.mln_map_get_layer_property(
                    Handle,
                    nativeLayerId.Value,
                    nativePropertyName.Value,
                    completion
                ),
            ReadOptionalBuffer,
            cancellationToken
        );
    }

    /// <summary>Sets or clears a layer filter.</summary>
    public Task<CommandCompletion> SetLayerFilterAsync(string layerId, byte[]? filter)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeFilter = filter is null
            ? null
            : NativeStringView.From(filter, nameof(filter));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_layer_filter(
                    Handle,
                    nativeLayerId.Value,
                    nativeFilter?.Pointer,
                    completion
                )
            );
        });
    }

    /// <summary>Gets one layer filter snapshot, or null when no filter exists.</summary>
    public Task<byte[]?> GetLayerFilterAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_get_layer_filter(Handle, nativeLayerId.Value, completion),
            ReadOptionalBuffer,
            cancellationToken
        );
    }

    /// <summary>Sets a layer's source-layer ID.</summary>
    public Task<CommandCompletion> SetLayerSourceLayerAsync(string layerId, string sourceLayer)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceLayer = NativeStringView.From(sourceLayer, nameof(sourceLayer));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_layer_source_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceLayer.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Sets a layer's source ID.</summary>
    public Task<CommandCompletion> SetLayerSourceIdAsync(string layerId, string sourceId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_layer_source_id(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceId.Value,
                    completion
                )
            );
        });
    }

    /// <summary>Sets the lowest layer zoom.</summary>
    public Task<CommandCompletion> SetLayerMinZoomAsync(string layerId, double minZoom)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_layer_min_zoom(
                    Handle,
                    nativeLayerId.Value,
                    minZoom,
                    completion
                )
            );
        });
    }

    /// <summary>Sets the highest layer zoom.</summary>
    public Task<CommandCompletion> SetLayerMaxZoomAsync(string layerId, double maxZoom)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_layer_max_zoom(
                    Handle,
                    nativeLayerId.Value,
                    maxZoom,
                    completion
                )
            );
        });
    }

    /// <summary>Sets layer visibility.</summary>
    public Task<CommandCompletion> SetLayerVisibilityAsync(
        string layerId,
        StyleLayerVisibility visibility
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion.SubmitCommandChecked(completion =>
        {
            NativeStatus.Check(
                NativeMethods.mln_map_set_layer_visibility(
                    Handle,
                    nativeLayerId.Value,
                    (uint)visibility,
                    completion
                )
            );
        });
    }

    /// <summary>Releases the map and waits for native teardown.</summary>
    public void Close() => CloseAsync().GetAwaiter().GetResult();

    /// <summary>Releases the map without blocking and reports native teardown.</summary>
    public Task CloseAsync()
    {
        if (!IsClosed)
        {
            state.Close();
            runtime.UnregisterMap(this);
        }

        return teardown;
    }

    private mln_status StartRelease(MlnMap handle)
    {
        teardown = NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_map_release(handle, completion)
        );
        return mln_status.MLN_STATUS_OK;
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

    internal static IDisposable UseCustomMvtVectorSourceInstallForTest(
        MapAddCustomMvtVectorSource addCustomMvtVectorSource
    )
    {
        var previous = addCustomMvtVectorSourceForTest;
        addCustomMvtVectorSourceForTest = addCustomMvtVectorSource;
        return new RestoreCustomMvtVectorSourceInstall(previous);
    }

    private static MapAddCustomMvtVectorSource AddCustomMvtVectorSourceNative =>
        addCustomMvtVectorSourceForTest ?? DefaultAddCustomMvtVectorSource;

    private sealed class RestoreCustomGeometrySourceInstall(MapAddCustomGeometrySource? previous)
        : IDisposable
    {
        public void Dispose()
        {
            addCustomGeometrySourceForTest = previous;
        }
    }

    private sealed class RestoreCustomMvtVectorSourceInstall(MapAddCustomMvtVectorSource? previous)
        : IDisposable
    {
        public void Dispose()
        {
            addCustomMvtVectorSourceForTest = previous;
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

    private static Task<T> RunMapOperationAsync<T>(
        CompletionSubmit start,
        CompletionConverter<T> convert,
        CancellationToken cancellationToken
    ) => NativeCompletion.Submit(start, convert).WaitAsync(cancellationToken);

    private static byte[] ReadBuffer(mln_completion_result* result) =>
        ValueStructs.CopyBufferView(NativeCompletion.Value<mln_buffer_view>(result));

    private static byte[]? ReadOptionalBuffer(mln_completion_result* result) =>
        result->value_count == 0 ? null : ReadBuffer(result);

    private static string[] ReadStrings(mln_completion_result* result)
    {
        var views = NativeCompletion.Values<mln_buffer_view>(result);
        var strings = new string[views.Length];
        for (var index = 0; index < views.Length; index++)
        {
            strings[index] = RuntimeStructs.CopyUtf8((sbyte*)views[index].data, views[index].size);
        }
        return strings;
    }

    private static CameraOptions ReadCamera(mln_completion_result* result) =>
        MapStructs.CameraOptionsFromNative(NativeCompletion.Value<mln_camera_options>(result));

    private static LatLngBounds ReadBounds(mln_completion_result* result) =>
        MapStructs.FromNative(NativeCompletion.Value<mln_lat_lng_bounds>(result));

    private static ScreenPoint[] ReadPoints(mln_completion_result* result)
    {
        var values = NativeCompletion.Values<mln_screen_point>(result);
        return values.ToArray().Select(MapStructs.FromNative).ToArray();
    }

    private static LatLng[] ReadCoordinates(mln_completion_result* result)
    {
        var values = NativeCompletion.Values<mln_lat_lng>(result);
        return values.ToArray().Select(CoreStructs.FromNative).ToArray();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        Close();
        GC.KeepAlive(runtime);
    }

    /// <inheritdoc />
    public ValueTask DisposeAsync() => new(CloseAsync());
}
