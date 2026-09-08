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
    private volatile Task teardown = Task.CompletedTask;

    private MapHandle(RuntimeHandle runtime, MlnMap handle)
    {
        this.runtime = runtime;
        nativeId = handle.Value;
        state = new NativeHandleState<MlnMap>(handle, StartRelease, nameof(MapHandle));
    }

    /// <summary>Creates a map asynchronously.</summary>
    /// <remarks>
    /// The task carries the only reference to a map the runtime already created, so this create
    /// takes no cancellation token: abandoning the task would leak the native map.
    /// </remarks>
    public static Task<MapHandle> CreateAsync(RuntimeHandle runtime, MapOptions options)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        ArgumentNullException.ThrowIfNull(options);
        return NativeCompletion.Submit(
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
        );
    }

    internal MlnMap Handle => state.Handle;

    internal RuntimeHandle Runtime => runtime;

    /// <summary>The issued native handle id, readable after close.</summary>
    internal ulong NativeId => nativeId;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Requests a repaint.</summary>
    /// <remarks>
    /// A map outside <see cref="MapMode.Continuous" /> throws
    /// <see cref="Error.InvalidStateException" /> before the request is accepted.
    /// </remarks>
    public Task<CommandCompletion> RequestRepaintAsync(
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion => NativeMethods.mln_map_request_repaint(Handle, completion))
            .WaitAsync(cancellationToken);
    }

    /// <summary>Requests a noncoalescing still-image render.</summary>
    /// <remarks>
    /// The map must be in <see cref="MapMode.Static" /> or <see cref="MapMode.Tile" />. Keep
    /// servicing the attached render session while the returned task is pending: the image is
    /// produced by the frames that session renders. The task reports
    /// <see cref="Error.MaplibreStatus.InvalidState" /> when another still image was already
    /// pending, and <see cref="Error.MaplibreStatus.Cancelled" /> when the map closes first.
    /// </remarks>
    public Task RequestStillImageAsync(CancellationToken cancellationToken = default)
    {
        return NativeCompletion
            .SubmitUnit(completion => NativeMethods.mln_map_request_still_image(Handle, completion))
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets native debug drawing options.</summary>
    /// <remarks>The committed mask is visible as <see cref="MapSnapshot.DebugOptions" />.</remarks>
    public Task<CommandCompletion> SetDebugOptionsAsync(
        DebugOptions options,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_debug_options(Handle, (uint)options, completion)
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Shows or hides the built-in rendering statistics overlay.</summary>
    /// <remarks>
    /// The committed value is visible as <see cref="MapSnapshot.RenderingStatsViewEnabled" />.
    /// </remarks>
    public Task<CommandCompletion> SetRenderingStatsViewEnabledAsync(
        bool enabled,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_rendering_stats_view_enabled(
                    Handle,
                    enabled ? (byte)1 : (byte)0,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Asks the native map to write debug logs.</summary>
    public Task<CommandCompletion> DumpDebugLogsAsync(CancellationToken cancellationToken = default)
    {
        return NativeCompletion
            .SubmitCommand(completion => NativeMethods.mln_map_dump_debug_logs(Handle, completion))
            .WaitAsync(cancellationToken);
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
            snapshot.gesture_in_progress != 0,
            (RuntimeEventMask)snapshot.event_mask,
            snapshot.latest_render_update_generation,
            MapStructs.TileOptionsFromNative(snapshot.tile),
            MapStructs.BoundOptionsFromNative(snapshot.bounds),
            MapStructs.FreeCameraOptionsFromNative(snapshot.free_camera)
        );
    }

    /// <summary>Submits a logical extent change.</summary>
    /// <remarks>
    /// Only the width and height may change: the scale factor is fixed when the map is created, and
    /// a different one is rejected with <see cref="Error.MaplibreStatus.InvalidArgument" />. Resize
    /// an attached render session through
    /// <see cref="Render.RenderSessionHandle.ResizeAsync" /> instead, which submits this command
    /// itself; a direct map resize leaves the session waiting for an update the map never
    /// publishes.
    /// </remarks>
    public Task<CommandCompletion> ResizeAsync(
        LogicalExtent extent,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
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
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets viewport options.</summary>
    /// <remarks>The committed options are visible as <see cref="MapSnapshot.Viewport" />.</remarks>
    public Task<CommandCompletion> SetViewportOptionsAsync(
        ViewportOptions options,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeOptions = MapStructs.ToNative(options);
                return NativeMethods.mln_map_set_viewport_options(
                    Handle,
                    &nativeOptions,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets tile tuning options.</summary>
    /// <remarks>The committed options are visible as <see cref="MapSnapshot.Tile" />.</remarks>
    public Task<CommandCompletion> SetTileOptionsAsync(
        TileOptions options,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeOptions = MapStructs.ToNative(options);
                return NativeMethods.mln_map_set_tile_options(Handle, &nativeOptions, completion);
            })
            .WaitAsync(cancellationToken);
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
    public Task<CommandCompletion> UpdateCameraAsync(
        CameraUpdate update,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var native = MapStructs.ToNative(update);
                return NativeMethods.mln_map_update_camera(Handle, &native, completion);
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Submits one relative camera operation.</summary>
    public Task<CommandCompletion> ApplyCameraDeltaAsync(
        CameraDelta delta,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var native = MapStructs.ToNative(delta);
                return NativeMethods.mln_map_apply_camera_delta(Handle, &native, completion);
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Cancels the camera transitions running when this command commits.</summary>
    /// <remarks>
    /// A cancelled transition that carried a transition ID reports its end through a
    /// <see cref="RuntimeEventPayload.CameraTransitionFinished" /> event, the same way a completed
    /// one does. Cancelling with no transition running commits and changes nothing.
    /// </remarks>
    public Task<CommandCompletion> CancelTransitionsAsync(
        CancellationToken cancellationToken = default
    ) =>
        NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_cancel_transitions(Handle, completion)
            )
            .WaitAsync(cancellationToken);

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

    /// <summary>Calculates a camera that fits geographic bounds and fit options.</summary>
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

    /// <summary>Calculates a camera that fits geographic coordinates and fit options.</summary>
    public Task<CameraOptions> CameraForLatLngsAsync(
        IReadOnlyList<LatLng> coordinates,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        var nativeCoordinates = ToNativeCoordinates(coordinates, nameof(coordinates));
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

    /// <summary>Calculates a camera that fits geographic geometry and fit options.</summary>
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

    /// <summary>Computes geographic bounds for a camera from two viewport corners.</summary>
    /// <remarks>
    /// The box is the hull of the top-left and bottom-right screen corners for that camera in the
    /// current viewport. When bearing and pitch are zero, the box equals the visible area. Those
    /// corners are the northwest and southeast of the viewport. Longitudes stay in -180 to 180.
    /// </remarks>
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

    /// <summary>Computes geographic bounds for a camera from the four viewport corners.</summary>
    /// <remarks>
    /// The axis-aligned hull of all four screen corners and the center encompasses the projected
    /// viewport. Longitudes unwrap onto the shortest path through the center. A viewport that
    /// crosses the antimeridian reports values outside -180 to 180.
    /// </remarks>
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
    public Task<CommandCompletion> SetBoundsAsync(
        BoundOptions options,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeOptions = MapStructs.ToNative(options);
                return NativeMethods.mln_map_set_bounds(Handle, &nativeOptions, completion);
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets free-camera options.</summary>
    /// <remarks>The committed options are visible as <see cref="MapSnapshot.FreeCamera" />.</remarks>
    public Task<CommandCompletion> SetFreeCameraOptionsAsync(
        FreeCameraOptions options,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeOptions = MapStructs.ToNative(options);
                return NativeMethods.mln_map_set_free_camera_options(
                    Handle,
                    &nativeOptions,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Converts a geographic coordinate to a screen pixel using the current map projection.</summary>
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

    /// <summary>Converts geographic coordinates to screen pixels using the current map projection.</summary>
    public Task<ScreenPoint[]> PixelsForLatLngsAsync(
        IReadOnlyList<LatLng> coordinates,
        CancellationToken cancellationToken = default
    )
    {
        var native = ToNativeCoordinates(coordinates, nameof(coordinates));
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
    /// <remarks>
    /// The task carries the only reference to a projection the map already created, so this create
    /// takes no cancellation token: abandoning the task would leak the native projection.
    /// </remarks>
    public Task<MapProjectionHandle> CreateProjectionAsync() =>
        MapProjectionHandle.CreateAsync(this);

    /// <summary>Sets projection mode options.</summary>
    public Task<CommandCompletion> SetProjectionModeAsync(
        ProjectionModeOptions mode,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeMode = MapStructs.ToNative(mode);
                return NativeMethods.mln_map_set_projection_mode(Handle, &nativeMode, completion);
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Loads a style URL.</summary>
    public Task<CommandCompletion> SetStyleUrlAsync(
        string url,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(url);
        using var nativeUrl = NativeUtf8String.FromNullableString(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_url(Handle, nativeUrl.Pointer, completion)
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Loads inline style JSON.</summary>
    public Task<CommandCompletion> SetStyleJsonAsync(
        byte[] json,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(json);
        using var nativeJson = NativeStringView.From(json, nameof(json));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_json(Handle, nativeJson.Value, completion)
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets per-feature state on this map.</summary>
    public Task<CommandCompletion> SetFeatureStateAsync(
        FeatureStateSelector selector,
        byte[] state,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(selector);
        ArgumentNullException.ThrowIfNull(state);
        using var nativeState = NativeStringView.From(state, nameof(state));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeSelector = NativeFeatureStateSelector.From(selector);
                var selectorValue = nativeSelector.Value;
                return NativeMethods.mln_map_set_feature_state(
                    Handle,
                    &selectorValue,
                    nativeState.Value,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
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
    public Task<CommandCompletion> RemoveFeatureStateAsync(
        FeatureStateSelector selector,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(selector);
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeSelector = NativeFeatureStateSelector.From(selector);
                var selectorValue = nativeSelector.Value;
                return NativeMethods.mln_map_remove_feature_state(
                    Handle,
                    &selectorValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
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
    public Task<CommandCompletion> SetEventMaskAsync(
        RuntimeEventMask mask,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_event_mask(Handle, (ulong)mask, completion)
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a style source from UTF-8 JSON bytes.</summary>
    public Task<CommandCompletion> AddStyleSourceJsonAsync(
        string sourceId,
        byte[] sourceJson,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeJson = NativeStringView.From(sourceJson, nameof(sourceJson));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_add_style_source_json(
                    Handle,
                    nativeSourceId.Value,
                    nativeJson.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Removes a style source.</summary>
    /// <remarks>
    /// The <see cref="CommandCompletion" /> reports <see cref="Error.MaplibreStatus.NotFound" /> when
    /// no source carries the ID.
    /// </remarks>
    public Task<CommandCompletion> RemoveStyleSourceAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_remove_style_source(Handle, nativeSourceId.Value, completion)
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets whether a style source stores fetched tiles in persistent storage.</summary>
    /// <remarks>The command fails with a not-found status when the source does not exist.</remarks>
    public Task<CommandCompletion> SetStyleSourceVolatileAsync(
        string sourceId,
        bool isVolatile,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_source_volatile(
                    Handle,
                    nativeSourceId.Value,
                    isVolatile ? (byte)1 : (byte)0,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Lists style source IDs in style order.</summary>
    public Task<string[]> StyleSourceIdsAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            completion => NativeMethods.mln_map_list_style_source_ids(Handle, completion),
            ReadStrings,
            cancellationToken
        );

    /// <summary>Copies one style source's URL, or null when the source or URL is missing.</summary>
    public Task<string?> GetStyleSourceUrlAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_copy_style_source_url(
                    Handle,
                    nativeSourceId.Value,
                    completion
                ),
            ReadOptionalString,
            cancellationToken
        );
    }

    /// <summary>
    /// Copies one style source's attribution, or null when the source or attribution is missing.
    /// </summary>
    public Task<string?> GetStyleSourceAttributionAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_copy_style_source_attribution(
                    Handle,
                    nativeSourceId.Value,
                    completion
                ),
            ReadOptionalString,
            cancellationToken
        );
    }

    /// <summary>
    /// Copies one style source's inline TileJSON tile URLs, or null when no source carries the ID.
    /// </summary>
    /// <remarks>A URL-backed source, or one without inline TileJSON, reports an empty array.</remarks>
    public Task<string[]?> GetStyleSourceTileUrlsAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_get_style_source_tile_urls(
                    Handle,
                    nativeSourceId.Value,
                    completion
                ),
            ReadTileUrls,
            cancellationToken
        );
    }

    /// <summary>Adds a GeoJSON source that loads data from a URL.</summary>
    /// <remarks>
    /// <paramref name="options" /> is fixed when the source is created;
    /// <see cref="SetGeoJsonSourceUrlAsync" /> and <see cref="SetGeoJsonSourceDataAsync" /> keep it.
    /// </remarks>
    public Task<CommandCompletion> AddGeoJsonSourceUrlAsync(
        string sourceId,
        string url,
        GeoJsonSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeOptions = options is null
                    ? null
                    : NativeGeoJsonSourceOptions.From(options);
                var optionsValue = nativeOptions?.Value ?? default;
                return NativeMethods.mln_map_add_geojson_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Updates a GeoJSON source to load data from a URL.</summary>
    public Task<CommandCompletion> SetGeoJsonSourceUrlAsync(
        string sourceId,
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_geojson_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a GeoJSON source backed by prepared data.</summary>
    /// <remarks>
    /// The call borrows <paramref name="data" />; the source adopts the options baked into it
    /// when the data was prepared and keeps its own reference, so the handle may be released
    /// afterward.
    /// </remarks>
    public Task<CommandCompletion> AddGeoJsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(data);
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_add_geojson_source_data(
                    Handle,
                    nativeSourceId.Value,
                    data.Handle,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Updates a GeoJSON source with prepared data.</summary>
    /// <remarks>
    /// The call borrows <paramref name="data" />; the command fails when the options baked into
    /// it differ from the source's, except for cluster properties.
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(data);
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_geojson_source_data(
                    Handle,
                    nativeSourceId.Value,
                    data.Handle,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Overrides synchronous tiling for a GeoJSON source.</summary>
    /// <remarks>
    /// The effective behavior is the source's baked-in option OR this override.
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceSynchronousTilingAsync(
        string sourceId,
        bool enabled,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_geojson_source_synchronous_tiling(
                    Handle,
                    nativeSourceId.Value,
                    enabled ? (byte)1 : (byte)0,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
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
        CustomGeometrySourceOptions options,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(options);
        return AddCustomGeometrySourceAsync(
            sourceId,
            new CustomGeometrySourceState(options),
            cancellationToken
        );
    }

    internal Task<CommandCompletion> AddCustomGeometrySourceAsync(
        string sourceId,
        CustomGeometrySourceState sourceState,
        CancellationToken cancellationToken = default
    )
    {
        try
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            return NativeCompletion
                .SubmitCommand(completion =>
                {
                    var descriptor = sourceState.Descriptor;
                    return AddCustomGeometrySourceNative(
                        Handle,
                        nativeSourceId.Value,
                        &descriptor,
                        completion
                    );
                })
                .WaitAsync(cancellationToken);
        }
        catch
        {
            // A rejected add never referenced the state, so no release callback is owed for it.
            sourceState.Dispose();
            throw;
        }
    }

    /// <summary>Sets custom geometry source tile data.</summary>
    public Task<CommandCompletion> SetCustomGeometrySourceTileDataAsync(
        string sourceId,
        CanonicalTileId tileId,
        byte[] data,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeData = NativeStringView.From(data, nameof(data));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeTileId = StyleStructs.ToNative(tileId);
                return NativeMethods.mln_map_set_custom_geometry_source_tile_data(
                    Handle,
                    nativeSourceId.Value,
                    nativeTileId,
                    nativeData.Value,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Invalidates one custom geometry source tile.</summary>
    public Task<CommandCompletion> InvalidateCustomGeometrySourceTileAsync(
        string sourceId,
        CanonicalTileId tileId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_invalidate_custom_geometry_source_tile(
                    Handle,
                    nativeSourceId.Value,
                    StyleStructs.ToNative(tileId),
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Invalidates custom geometry source tiles that intersect bounds.</summary>
    public Task<CommandCompletion> InvalidateCustomGeometrySourceRegionAsync(
        string sourceId,
        LatLngBounds bounds,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_invalidate_custom_geometry_source_region(
                    Handle,
                    nativeSourceId.Value,
                    MapStructs.ToNative(bounds),
                    completion
                )
            )
            .WaitAsync(cancellationToken);
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
        CustomMvtVectorSourceOptions options,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(options);
        return AddCustomMvtVectorSourceAsync(
            sourceId,
            new CustomMvtVectorSourceState(options),
            cancellationToken
        );
    }

    internal Task<CommandCompletion> AddCustomMvtVectorSourceAsync(
        string sourceId,
        CustomMvtVectorSourceState sourceState,
        CancellationToken cancellationToken = default
    )
    {
        try
        {
            using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
            return NativeCompletion
                .SubmitCommand(completion =>
                {
                    var descriptor = sourceState.Descriptor;
                    return AddCustomMvtVectorSourceNative(
                        Handle,
                        nativeSourceId.Value,
                        &descriptor,
                        completion
                    );
                })
                .WaitAsync(cancellationToken);
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
        byte[] data,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeData = NativeStringView.From(data, nameof(data));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeTileId = StyleStructs.ToNative(tileId);
                return NativeMethods.mln_map_set_custom_mvt_vector_source_tile_data(
                    Handle,
                    nativeSourceId.Value,
                    nativeTileId,
                    nativeData.Value,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Reports a custom MVT vector source error for one tile.</summary>
    public Task<CommandCompletion> SetCustomMvtVectorSourceTileErrorAsync(
        string sourceId,
        CanonicalTileId tileId,
        string message,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeMessage = NativeStringView.From(message, nameof(message));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeTileId = StyleStructs.ToNative(tileId);
                return NativeMethods.mln_map_set_custom_mvt_vector_source_tile_error(
                    Handle,
                    nativeSourceId.Value,
                    nativeTileId,
                    nativeMessage.Value,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Invalidates one custom MVT vector source tile.</summary>
    public Task<CommandCompletion> InvalidateCustomMvtVectorSourceTileAsync(
        string sourceId,
        CanonicalTileId tileId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_invalidate_custom_mvt_vector_source_tile(
                    Handle,
                    nativeSourceId.Value,
                    StyleStructs.ToNative(tileId),
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a vector source that loads TileJSON from a URL.</summary>
    public Task<CommandCompletion> AddVectorSourceUrlAsync(
        string sourceId,
        string url,
        TileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeOptions = options is null
                    ? null
                    : NativeTileSourceOptions.From(options);
                var optionsValue = nativeOptions?.Value ?? default;
                return NativeMethods.mln_map_add_vector_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a vector source from inline tile URL templates.</summary>
    public Task<CommandCompletion> AddVectorSourceTilesAsync(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
                using var nativeOptions = options is null
                    ? null
                    : NativeTileSourceOptions.From(options);
                var optionsValue = nativeOptions?.Value ?? default;
                return NativeMethods.mln_map_add_vector_source_tiles(
                    Handle,
                    nativeSourceId.Value,
                    nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                    nativeTiles.Count,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a raster source that loads TileJSON from a URL.</summary>
    public Task<CommandCompletion> AddRasterSourceUrlAsync(
        string sourceId,
        string url,
        TileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeOptions = options is null
                    ? null
                    : NativeTileSourceOptions.From(options);
                var optionsValue = nativeOptions?.Value ?? default;
                return NativeMethods.mln_map_add_raster_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a raster source from inline tile URL templates.</summary>
    public Task<CommandCompletion> AddRasterSourceTilesAsync(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
                using var nativeOptions = options is null
                    ? null
                    : NativeTileSourceOptions.From(options);
                var optionsValue = nativeOptions?.Value ?? default;
                return NativeMethods.mln_map_add_raster_source_tiles(
                    Handle,
                    nativeSourceId.Value,
                    nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                    nativeTiles.Count,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a raster DEM source that loads TileJSON from a URL.</summary>
    public Task<CommandCompletion> AddRasterDemSourceUrlAsync(
        string sourceId,
        string url,
        TileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeOptions = options is null
                    ? null
                    : NativeTileSourceOptions.From(options);
                var optionsValue = nativeOptions?.Value ?? default;
                return NativeMethods.mln_map_add_raster_dem_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a raster DEM source from inline tile URL templates.</summary>
    public Task<CommandCompletion> AddRasterDemSourceTilesAsync(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
                using var nativeOptions = options is null
                    ? null
                    : NativeTileSourceOptions.From(options);
                var optionsValue = nativeOptions?.Value ?? default;
                return NativeMethods.mln_map_add_raster_dem_source_tiles(
                    Handle,
                    nativeSourceId.Value,
                    nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                    nativeTiles.Count,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets or replaces a style image.</summary>
    public Task<CommandCompletion> SetStyleImageAsync(
        string imageId,
        PremultipliedRgba8Image image,
        StyleImageOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeImage = NativeStyleImage.From(image);
                var imageValue = nativeImage.Value;
                using var nativeOptions = options is null
                    ? null
                    : NativeStyleImageOptions.From(options);
                var optionsValue = nativeOptions?.Value ?? default;
                return NativeMethods.mln_map_set_style_image(
                    Handle,
                    nativeImageId.Value,
                    &imageValue,
                    nativeOptions is null ? null : &optionsValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Submits a command that removes one runtime style image.</summary>
    /// <remarks>
    /// The <see cref="CommandCompletion" /> reports
    /// <see cref="Error.MaplibreStatus.NotFound" /> when no image has <paramref name="imageId" />.
    /// </remarks>
    public Task<CommandCompletion> RemoveStyleImageAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_remove_style_image(Handle, nativeImageId.Value, completion)
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>
    /// Copies one runtime style image as tightly packed premultiplied RGBA8 pixels, or null when
    /// no image carries <paramref name="imageId" />.
    /// </summary>
    public Task<byte[]?> GetStyleImagePremultipliedRgba8Async(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_copy_style_image_premultiplied_rgba8(
                    Handle,
                    nativeImageId.Value,
                    completion
                ),
            ReadOptionalBuffer,
            cancellationToken
        );
    }

    /// <summary>
    /// Copies one runtime style image's stretchable intervals, or null when no image carries
    /// <paramref name="imageId" />.
    /// </summary>
    public Task<(
        IReadOnlyList<ImageStretch> StretchX,
        IReadOnlyList<ImageStretch> StretchY
    )?> GetStyleImageStretchesAsync(string imageId, CancellationToken cancellationToken = default)
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_copy_style_image_stretches(
                    Handle,
                    nativeImageId.Value,
                    completion
                ),
            ReadStretches,
            cancellationToken
        );
    }

    /// <summary>Adds an image source that loads image data from a URL.</summary>
    public Task<CommandCompletion> AddImageSourceUrlAsync(
        string sourceId,
        IReadOnlyList<LatLng> coordinates,
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeCoordinates = ToNativeCoordinates(coordinates, nameof(coordinates));
                fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
                {
                    return NativeMethods.mln_map_add_image_source_url(
                        Handle,
                        nativeSourceId.Value,
                        coordinatesPointer,
                        (nuint)nativeCoordinates.Length,
                        nativeUrl.Value,
                        completion
                    );
                }
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds an image source with inline premultiplied RGBA8 image data.</summary>
    public Task<CommandCompletion> AddImageSourceImageAsync(
        string sourceId,
        IReadOnlyList<LatLng> coordinates,
        PremultipliedRgba8Image image,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeImage = NativeStyleImage.From(image);
                var imageValue = nativeImage.Value;
                var nativeCoordinates = ToNativeCoordinates(coordinates, nameof(coordinates));
                fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
                {
                    return NativeMethods.mln_map_add_image_source_image(
                        Handle,
                        nativeSourceId.Value,
                        coordinatesPointer,
                        (nuint)nativeCoordinates.Length,
                        &imageValue,
                        completion
                    );
                }
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Updates an image source to load image data from a URL.</summary>
    public Task<CommandCompletion> SetImageSourceUrlAsync(
        string sourceId,
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_image_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Updates an image source with inline premultiplied RGBA8 image data.</summary>
    public Task<CommandCompletion> SetImageSourceImageAsync(
        string sourceId,
        PremultipliedRgba8Image image,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeImage = NativeStyleImage.From(image);
                var imageValue = nativeImage.Value;
                return NativeMethods.mln_map_set_image_source_image(
                    Handle,
                    nativeSourceId.Value,
                    &imageValue,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Updates image source coordinates.</summary>
    public Task<CommandCompletion> SetImageSourceCoordinatesAsync(
        string sourceId,
        IReadOnlyList<LatLng> coordinates,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeCoordinates = ToNativeCoordinates(coordinates, nameof(coordinates));
                fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
                {
                    return NativeMethods.mln_map_set_image_source_coordinates(
                        Handle,
                        nativeSourceId.Value,
                        coordinatesPointer,
                        (nuint)nativeCoordinates.Length,
                        completion
                    );
                }
            })
            .WaitAsync(cancellationToken);
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
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeBeforeLayerId = NativeStringView.From(
                    beforeLayerId,
                    nameof(beforeLayerId)
                );
                return NativeMethods.mln_map_add_hillshade_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a color-relief layer for a raster DEM source.</summary>
    public Task<CommandCompletion> AddColorReliefLayerAsync(
        string layerId,
        string sourceId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeBeforeLayerId = NativeStringView.From(
                    beforeLayerId,
                    nameof(beforeLayerId)
                );
                return NativeMethods.mln_map_add_color_relief_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a source-free location indicator layer.</summary>
    public Task<CommandCompletion> AddLocationIndicatorLayerAsync(
        string layerId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                using var nativeBeforeLayerId = NativeStringView.From(
                    beforeLayerId,
                    nameof(beforeLayerId)
                );
                return NativeMethods.mln_map_add_location_indicator_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets a location indicator layer location.</summary>
    public Task<CommandCompletion> SetLocationIndicatorLocationAsync(
        string layerId,
        LatLng coordinate,
        double altitude,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_location_indicator_location(
                    Handle,
                    nativeLayerId.Value,
                    CoreStructs.ToNative(coordinate),
                    altitude,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets a location indicator layer bearing in degrees.</summary>
    public Task<CommandCompletion> SetLocationIndicatorBearingAsync(
        string layerId,
        double bearing,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_location_indicator_bearing(
                    Handle,
                    nativeLayerId.Value,
                    bearing,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets a location indicator layer accuracy radius in meters.</summary>
    public Task<CommandCompletion> SetLocationIndicatorAccuracyRadiusAsync(
        string layerId,
        double radius,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_location_indicator_accuracy_radius(
                    Handle,
                    nativeLayerId.Value,
                    radius,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets a location indicator layer image-name property.</summary>
    public Task<CommandCompletion> SetLocationIndicatorImageNameAsync(
        string layerId,
        LocationIndicatorImageKind imageKind,
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_location_indicator_image_name(
                    Handle,
                    nativeLayerId.Value,
                    (uint)imageKind,
                    nativeImageId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Adds a style layer.</summary>
    public Task<CommandCompletion> AddStyleLayerJsonAsync(
        byte[] layerJson,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeJson = NativeStringView.From(layerJson, nameof(layerJson));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_add_style_layer_json(
                    Handle,
                    nativeJson.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Submits a command that removes one style layer.</summary>
    /// <remarks>
    /// The <see cref="CommandCompletion" /> reports <see cref="Error.MaplibreStatus.NotFound" />
    /// when no layer has <paramref name="layerId" />.
    /// </remarks>
    public Task<CommandCompletion> RemoveStyleLayerAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_remove_style_layer(Handle, nativeLayerId.Value, completion)
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Lists style layer IDs in style order.</summary>
    public Task<string[]> StyleLayerIdsAsync(CancellationToken cancellationToken = default) =>
        RunMapOperationAsync(
            completion => NativeMethods.mln_map_list_style_layer_ids(Handle, completion),
            ReadStrings,
            cancellationToken
        );

    /// <summary>Moves a style layer.</summary>
    public Task<CommandCompletion> MoveStyleLayerAsync(
        string layerId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_move_style_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
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
    public Task<CommandCompletion> SetStyleLightJsonAsync(
        byte[] lightJson,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeJson = NativeStringView.From(lightJson, nameof(lightJson));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_light_json(Handle, nativeJson.Value, completion)
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets one style light property.</summary>
    public Task<CommandCompletion> SetStyleLightPropertyAsync(
        string propertyName,
        byte[] value,
        CancellationToken cancellationToken = default
    )
    {
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeStringView.From(value, nameof(value));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_light_property(
                    Handle,
                    nativePropertyName.Value,
                    nativeValue.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
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
    public Task<CommandCompletion> SetStyleTransitionOptionsAsync(
        StyleTransitionOptions options,
        CancellationToken cancellationToken = default
    )
    {
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var native = StyleStructs.ToNative(options);
                return NativeMethods.mln_map_set_style_transition_options(
                    Handle,
                    &native,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
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
        byte[] value,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeStringView.From(value, nameof(value));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_property(
                    Handle,
                    nativeLayerId.Value,
                    nativePropertyName.Value,
                    nativeValue.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
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
    public Task<CommandCompletion> SetLayerFilterAsync(
        string layerId,
        byte[]? filter,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeFilter = filter is null
            ? null
            : NativeStringView.From(filter, nameof(filter));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_filter(
                    Handle,
                    nativeLayerId.Value,
                    nativeFilter?.Pointer,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
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
    public Task<CommandCompletion> SetLayerSourceLayerAsync(
        string layerId,
        string sourceLayer,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceLayer = NativeStringView.From(sourceLayer, nameof(sourceLayer));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_source_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceLayer.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets a layer's source ID.</summary>
    public Task<CommandCompletion> SetLayerSourceIdAsync(
        string layerId,
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_source_id(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Copies one layer's source-layer ID, or null when the layer sets none.</summary>
    public Task<string?> GetLayerSourceLayerAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_copy_layer_source_layer(
                    Handle,
                    nativeLayerId.Value,
                    completion
                ),
            ReadOptionalString,
            cancellationToken
        );
    }

    /// <summary>Copies one layer's source ID, or null when the layer takes no source.</summary>
    public Task<string?> GetLayerSourceIdAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return RunMapOperationAsync(
            completion =>
                NativeMethods.mln_map_copy_layer_source_id(Handle, nativeLayerId.Value, completion),
            ReadOptionalString,
            cancellationToken
        );
    }

    /// <summary>Sets the lowest layer zoom.</summary>
    public Task<CommandCompletion> SetLayerMinZoomAsync(
        string layerId,
        double minZoom,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_min_zoom(
                    Handle,
                    nativeLayerId.Value,
                    minZoom,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets the highest layer zoom.</summary>
    public Task<CommandCompletion> SetLayerMaxZoomAsync(
        string layerId,
        double maxZoom,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_max_zoom(
                    Handle,
                    nativeLayerId.Value,
                    maxZoom,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Sets layer visibility.</summary>
    public Task<CommandCompletion> SetLayerVisibilityAsync(
        string layerId,
        StyleLayerVisibility visibility,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_visibility(
                    Handle,
                    nativeLayerId.Value,
                    (uint)visibility,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    /// <summary>Releases the map and waits for native teardown.</summary>
    /// <remarks>
    /// Call it from a host thread: a MapLibre callback that waits here blocks the teardown it waits
    /// for. Use <see cref="CloseAsync" /> to release the handle without blocking.
    /// </remarks>
    public void Close() => CloseAsync().GetAwaiter().GetResult();

    /// <summary>Releases the map without blocking and reports native teardown.</summary>
    /// <remarks>
    /// The returned task completes after the runtime retires the native map. The handle is consumed
    /// once this method returns, so a rejected release throws before there is a task to await.
    /// </remarks>
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

    /// <summary>Reads one borrowed view, treating no value and an empty view alike.</summary>
    private static string? ReadOptionalString(mln_completion_result* result)
    {
        if (result->value_count == 0)
        {
            return null;
        }
        var view = NativeCompletion.Value<mln_buffer_view>(result);
        return view.size == 0 ? null : RuntimeStructs.CopyUtf8((sbyte*)view.data, view.size);
    }

    private static (
        IReadOnlyList<ImageStretch> StretchX,
        IReadOnlyList<ImageStretch> StretchY
    )? ReadStretches(mln_completion_result* result)
    {
        if (result->value_count == 0)
        {
            return null;
        }
        var value = NativeCompletion.Value<mln_style_image_stretches_result>(result);
        return (
            CopyStretches(value.stretch_x, value.stretch_x_count) ?? [],
            CopyStretches(value.stretch_y, value.stretch_y_count) ?? []
        );
    }

    /// <summary>Copies borrowed tile URLs, reading no value as a missing source.</summary>
    private static string[]? ReadTileUrls(mln_completion_result* result)
    {
        if (result->value_count == 0)
        {
            return null;
        }
        var value = NativeCompletion.Value<mln_style_source_tile_urls_result>(result);
        var urls = new string[checked((int)value.tile_url_count)];
        for (nuint index = 0; index < value.tile_url_count; index++)
        {
            var view = value.tile_urls[index];
            urls[(int)index] = RuntimeStructs.CopyUtf8((sbyte*)view.data, view.size);
        }
        return urls;
    }

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
        var points = new ScreenPoint[values.Length];
        for (var index = 0; index < values.Length; index++)
        {
            points[index] = MapStructs.FromNative(values[index]);
        }
        return points;
    }

    private static LatLng[] ReadCoordinates(mln_completion_result* result)
    {
        var values = NativeCompletion.Values<mln_lat_lng>(result);
        var coordinates = new LatLng[values.Length];
        for (var index = 0; index < values.Length; index++)
        {
            coordinates[index] = CoreStructs.FromNative(values[index]);
        }
        return coordinates;
    }

    /// <inheritdoc />
    /// <remarks>
    /// This blocks on native teardown the way <see cref="Close" /> does, with the same rule against
    /// calling it from a MapLibre callback. Use <see cref="DisposeAsync" /> to await it instead.
    /// </remarks>
    public void Dispose()
    {
        Close();
        GC.KeepAlive(runtime);
    }

    /// <inheritdoc />
    public ValueTask DisposeAsync() => new(CloseAsync());
}
