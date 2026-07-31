using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Pointer;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Json;
using Maplibre.Native.Render;
using Maplibre.Native.Runtime;
using Maplibre.Native.Style;

namespace Maplibre.Native.Map;

internal unsafe delegate mln_status MapAddCustomGeometrySource(
    MlnMap map,
    mln_string_view sourceId,
    mln_custom_geometry_source_options* options
);

/// <summary>Owner-thread map handle bound to a runtime.</summary>
public sealed unsafe class MapHandle : IDisposable
{
    private static readonly MapAddCustomGeometrySource DefaultAddCustomGeometrySource = static (
        map,
        sourceId,
        options
    ) => NativeMethods.mln_map_add_custom_geometry_source(map, sourceId, options);

    [ThreadStatic]
    private static MapAddCustomGeometrySource? addCustomGeometrySourceForTest;

    private readonly RuntimeHandle runtime;
    private readonly ulong nativeId;
    private readonly NativeHandleState<MlnMap> state;
    private readonly Dictionary<string, CustomGeometrySourceState> customGeometrySources = [];

    private MapHandle(RuntimeHandle runtime, MlnMap handle)
    {
        this.runtime = runtime;
        nativeId = handle.Value;
        state = new NativeHandleState<MlnMap>(
            handle,
            static handle => NativeMethods.mln_map_destroy(handle),
            nameof(MapHandle)
        );
    }

    /// <summary>Creates a map from a runtime on the runtime owner thread.</summary>
    public static MapHandle Create(RuntimeHandle runtime, MapOptions options)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        ArgumentNullException.ThrowIfNull(options);
        var nativeOptions = options.ToNative();
        MlnMap map = default;

        NativeStatus.Check(NativeMethods.mln_map_create(runtime.Handle, &nativeOptions, &map));
        var handle = new MapHandle(runtime, map);
        runtime.RegisterMap(handle);
        return handle;
    }

    internal MlnMap Handle => state.Handle;

    /// <summary>
    /// The issued native handle id, readable after close so the runtime can
    /// unregister this wrapper.
    /// </summary>
    internal ulong NativeId => nativeId;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Requests a repaint for a continuous map.</summary>
    public void RequestRepaint()
    {
        NativeStatus.Check(NativeMethods.mln_map_request_repaint(Handle));
    }

    /// <summary>Requests an asynchronous still-image render for a static map.</summary>
    public void RequestStillImage()
    {
        NativeStatus.Check(NativeMethods.mln_map_request_still_image(Handle));
    }

    /// <summary>Sets native debug drawing options.</summary>
    public void SetDebugOptions(DebugOptions options)
    {
        NativeStatus.Check(NativeMethods.mln_map_set_debug_options(Handle, (uint)options));
    }

    /// <summary>Gets native debug drawing options.</summary>
    public DebugOptions GetDebugOptions()
    {
        uint options = 0;
        NativeStatus.Check(NativeMethods.mln_map_get_debug_options(Handle, &options));
        return (DebugOptions)options;
    }

    /// <summary>Shows or hides the built-in rendering statistics overlay.</summary>
    public void SetRenderingStatsViewEnabled(bool enabled)
    {
        NativeStatus.Check(
            NativeMethods.mln_map_set_rendering_stats_view_enabled(
                Handle,
                enabled ? (byte)1 : (byte)0
            )
        );
    }

    /// <summary>Whether the built-in rendering statistics overlay is enabled.</summary>
    public bool GetRenderingStatsViewEnabled()
    {
        bool enabled = false;
        NativeStatus.Check(
            NativeMethods.mln_map_get_rendering_stats_view_enabled(Handle, &enabled)
        );
        return enabled;
    }

    /// <summary>Whether the native map reports all required resources loaded.</summary>
    public bool IsFullyLoaded()
    {
        bool loaded = false;
        NativeStatus.Check(NativeMethods.mln_map_is_fully_loaded(Handle, &loaded));
        return loaded;
    }

    /// <summary>Asks the native map to write debug logs through the native log system.</summary>
    public void DumpDebugLogs()
    {
        NativeStatus.Check(NativeMethods.mln_map_dump_debug_logs(Handle));
    }

    /// <summary>
    /// Gets the map's logical viewport size in UI pixels and its pixel ratio. The size starts at
    /// the creation width and height, and follows the attach and resize rules documented on
    /// <see cref="MapOptions"/>. The scale factor is fixed for the lifetime of the map and is
    /// independent of any render target's scale factor.
    /// </summary>
    public (uint Width, uint Height, double ScaleFactor) GetSize()
    {
        uint width;
        uint height;
        double scaleFactor;
        NativeStatus.Check(NativeMethods.mln_map_get_size(Handle, &width, &height, &scaleFactor));
        return (width, height, scaleFactor);
    }

    /// <summary>Gets the map's viewport options.</summary>
    public ViewportOptions GetViewportOptions()
    {
        var options = NativeMethods.mln_map_viewport_options_default();
        NativeStatus.Check(NativeMethods.mln_map_get_viewport_options(Handle, &options));
        return MapStructs.ViewportOptionsFromNative(options);
    }

    /// <summary>Sets viewport options, applying only non-null descriptor fields.</summary>
    public void SetViewportOptions(ViewportOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        NativeStatus.Check(NativeMethods.mln_map_set_viewport_options(Handle, &nativeOptions));
    }

    /// <summary>Gets tile tuning options.</summary>
    public TileOptions GetTileOptions()
    {
        var options = NativeMethods.mln_map_tile_options_default();
        NativeStatus.Check(NativeMethods.mln_map_get_tile_options(Handle, &options));
        return MapStructs.TileOptionsFromNative(options);
    }

    /// <summary>Sets tile tuning options, applying only non-null descriptor fields.</summary>
    public void SetTileOptions(TileOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        NativeStatus.Check(NativeMethods.mln_map_set_tile_options(Handle, &nativeOptions));
    }

    /// <summary>Gets the current camera descriptor.</summary>
    public CameraOptions GetCamera()
    {
        var camera = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(NativeMethods.mln_map_get_camera(Handle, &camera));
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Moves immediately to the camera descriptor, applying only non-null fields.</summary>
    public void JumpTo(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        NativeStatus.Check(NativeMethods.mln_map_jump_to(Handle, &nativeCamera));
    }

    /// <summary>Eases to the camera descriptor with animation options.</summary>
    /// <remarks>
    /// A <see langword="null" /> <paramref name="animation" />, or one with no
    /// <c>Duration</c>, uses the native default duration of zero, so the camera
    /// reaches the target immediately and nothing is animated. Set <c>Duration</c>
    /// explicitly to animate.
    /// </remarks>
    public void EaseTo(CameraOptions camera, AnimationOptions? animation)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_ease_to(
                Handle,
                &nativeCamera,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Flies to the camera descriptor with animation options.</summary>
    /// <remarks>
    /// <see cref="FlyTo" /> is the one camera command that derives a duration when
    /// none is given: a <see langword="null" /> <paramref name="animation" />, or
    /// one with no <c>Duration</c>, flies at a default velocity of 1.2 rho-screenfuls
    /// per second, so the duration scales with the distance travelled. Set
    /// <c>Duration</c> explicitly to pin it.
    /// </remarks>
    public void FlyTo(CameraOptions camera, AnimationOptions? animation)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_fly_to(
                Handle,
                &nativeCamera,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Moves the map by a screen delta.</summary>
    public void MoveBy(double deltaX, double deltaY)
    {
        NativeStatus.Check(NativeMethods.mln_map_move_by(Handle, deltaX, deltaY));
    }

    /// <summary>Moves the map by a screen delta with animation options.</summary>
    /// <remarks>
    /// A <see langword="null" /> <paramref name="animation" />, or one with no
    /// <c>Duration</c>, eases with the native default duration of zero, so the
    /// change applies instantly; see <see cref="EaseTo" />.
    /// </remarks>
    public void MoveByAnimated(double deltaX, double deltaY, AnimationOptions? animation)
    {
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_move_by_animated(
                Handle,
                deltaX,
                deltaY,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Scales the map around a screen anchor.</summary>
    public void ScaleBy(double scale, ScreenPoint? anchor)
    {
        var nativeAnchor = anchor is { } value ? MapStructs.ToNative(value) : default;
        NativeStatus.Check(
            NativeMethods.mln_map_scale_by(Handle, scale, anchor.HasValue ? &nativeAnchor : null)
        );
    }

    /// <summary>Scales the map around a screen anchor with animation options.</summary>
    /// <remarks>
    /// A <see langword="null" /> <paramref name="animation" />, or one with no
    /// <c>Duration</c>, eases with the native default duration of zero, so the
    /// change applies instantly; see <see cref="EaseTo" />.
    /// </remarks>
    public void ScaleByAnimated(double scale, ScreenPoint? anchor, AnimationOptions? animation)
    {
        var nativeAnchor = anchor is { } anchorValue ? MapStructs.ToNative(anchorValue) : default;
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_scale_by_animated(
                Handle,
                scale,
                anchor.HasValue ? &nativeAnchor : null,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Rotates around two screen points.</summary>
    public void RotateBy(ScreenPoint first, ScreenPoint second)
    {
        var nativeFirst = MapStructs.ToNative(first);
        var nativeSecond = MapStructs.ToNative(second);
        NativeStatus.Check(NativeMethods.mln_map_rotate_by(Handle, nativeFirst, nativeSecond));
    }

    /// <summary>Rotates around two screen points with animation options.</summary>
    /// <remarks>
    /// A <see langword="null" /> <paramref name="animation" />, or one with no
    /// <c>Duration</c>, eases with the native default duration of zero, so the
    /// change applies instantly; see <see cref="EaseTo" />.
    /// </remarks>
    public void RotateByAnimated(ScreenPoint first, ScreenPoint second, AnimationOptions? animation)
    {
        var nativeFirst = MapStructs.ToNative(first);
        var nativeSecond = MapStructs.ToNative(second);
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_rotate_by_animated(
                Handle,
                nativeFirst,
                nativeSecond,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Pitches the map by a delta in degrees.</summary>
    public void PitchBy(double pitch)
    {
        NativeStatus.Check(NativeMethods.mln_map_pitch_by(Handle, pitch));
    }

    /// <summary>Pitches the map by a delta in degrees with animation options.</summary>
    /// <remarks>
    /// A <see langword="null" /> <paramref name="animation" />, or one with no
    /// <c>Duration</c>, eases with the native default duration of zero, so the
    /// change applies instantly; see <see cref="EaseTo" />.
    /// </remarks>
    public void PitchByAnimated(double pitch, AnimationOptions? animation)
    {
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_pitch_by_animated(
                Handle,
                pitch,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Cancels in-flight camera transitions.</summary>
    public void CancelTransitions()
    {
        NativeStatus.Check(NativeMethods.mln_map_cancel_transitions(Handle));
    }

    /// <summary>Marks whether a host-driven gesture is in progress.</summary>
    /// <remarks>
    /// A host that decodes its own pointer gestures sets this to <see langword="true"/> when a
    /// gesture starts and back to <see langword="false"/> when it ends, so the camera commands
    /// issued in between belong to one live gesture. The flag stays set until the host clears it,
    /// so pair every <see langword="true"/> with a <see langword="false"/>.
    /// </remarks>
    public void SetGestureInProgress(bool inProgress)
    {
        NativeStatus.Check(
            NativeMethods.mln_map_set_gesture_in_progress(Handle, inProgress ? (byte)1 : (byte)0)
        );
    }

    /// <summary>Whether a host-driven gesture is currently in progress.</summary>
    public bool IsGestureInProgress()
    {
        bool inProgress = false;
        NativeStatus.Check(NativeMethods.mln_map_is_gesture_in_progress(Handle, &inProgress));
        return inProgress;
    }

    /// <summary>Calculates a camera that fits geographic bounds and fit options.</summary>
    public CameraOptions CameraForLatLngBounds(LatLngBounds bounds, CameraFitOptions? fitOptions)
    {
        var nativeBounds = MapStructs.ToNative(bounds);
        var nativeFitOptions = fitOptions is null ? default : MapStructs.ToNative(fitOptions);
        var camera = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(
            NativeMethods.mln_map_camera_for_lat_lng_bounds(
                Handle,
                nativeBounds,
                fitOptions is null ? null : &nativeFitOptions,
                &camera
            )
        );
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Calculates a camera that fits geographic coordinates and fit options.</summary>
    public CameraOptions CameraForLatLngs(
        IReadOnlyList<LatLng> coordinates,
        CameraFitOptions? fitOptions
    )
    {
        ArgumentNullException.ThrowIfNull(coordinates);
        var nativeCoordinates = new mln_lat_lng[coordinates.Count];
        for (var index = 0; index < coordinates.Count; index++)
        {
            nativeCoordinates[index] = CoreStructs.ToNative(coordinates[index]);
        }

        var nativeFitOptions = fitOptions is null ? default : MapStructs.ToNative(fitOptions);
        var camera = NativeMethods.mln_camera_options_default();
        fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_camera_for_lat_lngs(
                    Handle,
                    nativeCoordinates.Length == 0 ? null : coordinatesPointer,
                    (nuint)nativeCoordinates.Length,
                    fitOptions is null ? null : &nativeFitOptions,
                    &camera
                )
            );
        }
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Calculates a camera that fits geographic geometry and fit options.</summary>
    public CameraOptions CameraForGeometry(Geometry geometry, CameraFitOptions? fitOptions)
    {
        using var nativeGeometry = NativeGeometry.From(geometry);
        var nativeFitOptions = fitOptions is null ? default : MapStructs.ToNative(fitOptions);
        var camera = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(
            NativeMethods.mln_map_camera_for_geometry(
                Handle,
                nativeGeometry.Pointer,
                fitOptions is null ? null : &nativeFitOptions,
                &camera
            )
        );
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Calculates geographic bounds for a camera.</summary>
    public LatLngBounds LatLngBoundsForCamera(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        mln_lat_lng_bounds bounds = default;
        NativeStatus.Check(
            NativeMethods.mln_map_lat_lng_bounds_for_camera(Handle, &nativeCamera, &bounds)
        );
        return MapStructs.FromNative(bounds);
    }

    /// <summary>Calculates unwrapped geographic bounds for a camera.</summary>
    public LatLngBounds LatLngBoundsForCameraUnwrapped(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        mln_lat_lng_bounds bounds = default;
        NativeStatus.Check(
            NativeMethods.mln_map_lat_lng_bounds_for_camera_unwrapped(
                Handle,
                &nativeCamera,
                &bounds
            )
        );
        return MapStructs.FromNative(bounds);
    }

    /// <summary>Gets map bounds constraints.</summary>
    public BoundOptions GetBounds()
    {
        var options = NativeMethods.mln_bound_options_default();
        NativeStatus.Check(NativeMethods.mln_map_get_bounds(Handle, &options));
        return MapStructs.BoundOptionsFromNative(options);
    }

    /// <summary>Sets map bounds constraints, applying only non-null descriptor fields.</summary>
    public void SetBounds(BoundOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        NativeStatus.Check(NativeMethods.mln_map_set_bounds(Handle, &nativeOptions));
    }

    /// <summary>Gets free-camera options.</summary>
    public FreeCameraOptions GetFreeCameraOptions()
    {
        var options = NativeMethods.mln_free_camera_options_default();
        NativeStatus.Check(NativeMethods.mln_map_get_free_camera_options(Handle, &options));
        return MapStructs.FreeCameraOptionsFromNative(options);
    }

    /// <summary>Sets free-camera options, applying only non-null descriptor fields.</summary>
    public void SetFreeCameraOptions(FreeCameraOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        NativeStatus.Check(NativeMethods.mln_map_set_free_camera_options(Handle, &nativeOptions));
    }

    /// <summary>Converts a geographic coordinate to a screen pixel using the current map projection.</summary>
    public ScreenPoint PixelForLatLng(LatLng coordinate)
    {
        var nativeCoordinate = CoreStructs.ToNative(coordinate);
        mln_screen_point point = default;
        NativeStatus.Check(
            NativeMethods.mln_map_pixel_for_lat_lng(Handle, nativeCoordinate, &point)
        );
        return MapStructs.FromNative(point);
    }

    /// <summary>Converts a screen pixel to a geographic coordinate using the current map projection.</summary>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        var nativePoint = MapStructs.ToNative(point);
        mln_lat_lng coordinate = default;
        NativeStatus.Check(
            NativeMethods.mln_map_lat_lng_for_pixel(Handle, nativePoint, &coordinate)
        );
        return CoreStructs.FromNative(coordinate);
    }

    /// <summary>Converts geographic coordinates to screen pixels using the current map projection.</summary>
    public ScreenPoint[] PixelsForLatLngs(IReadOnlyList<LatLng> coordinates)
    {
        ArgumentNullException.ThrowIfNull(coordinates);
        if (coordinates.Count == 0)
        {
            return [];
        }

        var nativeCoordinates = new mln_lat_lng[coordinates.Count];
        var points = new mln_screen_point[coordinates.Count];
        for (var index = 0; index < coordinates.Count; index++)
        {
            nativeCoordinates[index] = CoreStructs.ToNative(coordinates[index]);
        }

        fixed (mln_lat_lng* coordinatesPointer = nativeCoordinates)
        fixed (mln_screen_point* pointsPointer = points)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_pixels_for_lat_lngs(
                    Handle,
                    coordinatesPointer,
                    (nuint)nativeCoordinates.Length,
                    pointsPointer
                )
            );
        }

        var result = new ScreenPoint[points.Length];
        for (var index = 0; index < result.Length; index++)
        {
            result[index] = MapStructs.FromNative(points[index]);
        }
        return result;
    }

    /// <summary>Converts screen pixels to geographic coordinates using the current map projection.</summary>
    public LatLng[] LatLngsForPixels(IReadOnlyList<ScreenPoint> points)
    {
        ArgumentNullException.ThrowIfNull(points);
        if (points.Count == 0)
        {
            return [];
        }

        var nativePoints = new mln_screen_point[points.Count];
        var coordinates = new mln_lat_lng[points.Count];
        for (var index = 0; index < points.Count; index++)
        {
            nativePoints[index] = MapStructs.ToNative(points[index]);
        }

        fixed (mln_screen_point* pointsPointer = nativePoints)
        fixed (mln_lat_lng* coordinatesPointer = coordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_lat_lngs_for_pixels(
                    Handle,
                    pointsPointer,
                    (nuint)nativePoints.Length,
                    coordinatesPointer
                )
            );
        }

        var result = new LatLng[coordinates.Length];
        for (var index = 0; index < result.Length; index++)
        {
            result[index] = CoreStructs.FromNative(coordinates[index]);
        }
        return result;
    }

    /// <summary>Creates a standalone projection snapshot from the map's current camera state.</summary>
    public MapProjectionHandle CreateProjection()
    {
        return MapProjectionHandle.Create(this);
    }

    /// <summary>Gets projection mode options.</summary>
    public ProjectionModeOptions GetProjectionMode()
    {
        var mode = NativeMethods.mln_projection_mode_default();
        NativeStatus.Check(NativeMethods.mln_map_get_projection_mode(Handle, &mode));
        return MapStructs.ProjectionModeOptionsFromNative(mode);
    }

    /// <summary>Sets projection mode options, applying only non-null descriptor fields.</summary>
    public void SetProjectionMode(ProjectionModeOptions mode)
    {
        var nativeMode = MapStructs.ToNative(mode);
        NativeStatus.Check(NativeMethods.mln_map_set_projection_mode(Handle, &nativeMode));
    }

    /// <summary>Loads a style URL through MapLibre Native style APIs.</summary>
    /// <remarks>
    /// Loading is asynchronous, so a style that is missing, unreachable, or
    /// malformed still returns normally here and reports through a
    /// <see cref="RuntimeEventType.MapLoadingFailed" /> runtime event. Watch the
    /// runtime event queue to observe style load failures. A well-formed style
    /// that MapLibre rejects semantically, such as an unknown <c>version</c> or a
    /// layer naming a missing source, produces neither an exception nor an event:
    /// MapLibre logs it and renders what it can.
    /// </remarks>
    public void SetStyleUrl(string url)
    {
        ArgumentNullException.ThrowIfNull(url);
        using var nativeUrl = NativeUtf8String.FromNullableString(url, nameof(url));
        NativeStatus.Check(NativeMethods.mln_map_set_style_url(Handle, nativeUrl.Pointer));
    }

    /// <summary>Loads inline style JSON through MapLibre Native style APIs.</summary>
    /// <remarks>
    /// Malformed JSON is reported twice: this call throws the parse error
    /// synchronously, and the same message also arrives as a
    /// <see cref="RuntimeEventType.MapLoadingFailed" /> runtime event. Handle both
    /// so a queued failure event is not a surprise. A well-formed style that
    /// MapLibre rejects semantically, such as an unknown <c>version</c> or a layer
    /// naming a missing source, produces neither an exception nor an event:
    /// MapLibre logs it and renders what it can.
    /// </remarks>
    public void SetStyleJson(string json)
    {
        ArgumentNullException.ThrowIfNull(json);
        using var nativeJson = NativeUtf8String.FromNullableString(json, nameof(json));
        NativeStatus.Check(NativeMethods.mln_map_set_style_json(Handle, nativeJson.Pointer));
        ClearCustomGeometrySources();
    }

    /// <summary>Adds a style source from a JSON-like value.</summary>
    public void AddStyleSourceJson(string sourceId, JsonValue sourceJson)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeJson = NativeJsonValue.From(sourceJson);
        NativeStatus.Check(
            NativeMethods.mln_map_add_style_source_json(
                Handle,
                nativeSourceId.Value,
                nativeJson.Pointer
            )
        );
    }

    /// <summary>Removes a style source and reports whether it existed.</summary>
    public bool RemoveStyleSource(string sourceId)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        bool removed = false;
        NativeStatus.Check(
            NativeMethods.mln_map_remove_style_source(Handle, nativeSourceId.Value, &removed)
        );
        if (removed && customGeometrySources.Remove(sourceId, out var state))
        {
            state.Dispose();
        }
        return removed;
    }

    /// <summary>Whether a style source exists.</summary>
    public bool StyleSourceExists(string sourceId)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        bool exists = false;
        NativeStatus.Check(
            NativeMethods.mln_map_style_source_exists(Handle, nativeSourceId.Value, &exists)
        );
        return exists;
    }

    /// <summary>Gets a style source type when the source exists.</summary>
    public SourceType? StyleSourceType(string sourceId)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        uint sourceType = 0;
        bool found = false;
        NativeStatus.Check(
            NativeMethods.mln_map_get_style_source_type(
                Handle,
                nativeSourceId.Value,
                &sourceType,
                &found
            )
        );
        return found ? (SourceType)sourceType : null;
    }

    /// <summary>Gets fixed style source metadata when the source exists.</summary>
    public SourceInfo? StyleSourceInfo(string sourceId)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var info = new mln_style_source_info { size = (uint)sizeof(mln_style_source_info) };
        bool found = false;
        NativeStatus.Check(
            NativeMethods.mln_map_get_style_source_info(Handle, nativeSourceId.Value, &info, &found)
        );
        if (!found)
        {
            return null;
        }

        string? attribution = null;
        if (info.has_attribution != 0)
        {
            attribution = string.Empty;
            if (info.attribution_size > 0)
            {
                var buffer = new byte[checked((int)info.attribution_size)];
                nuint attributionSize = 0;
                bool attributionFound = false;
                fixed (byte* bufferPointer = buffer)
                {
                    NativeStatus.Check(
                        NativeMethods.mln_map_copy_style_source_attribution(
                            Handle,
                            nativeSourceId.Value,
                            (sbyte*)bufferPointer,
                            (nuint)buffer.Length,
                            &attributionSize,
                            &attributionFound
                        )
                    );
                }

                if (!attributionFound)
                {
                    return null;
                }

                if (attributionSize > (nuint)buffer.Length)
                {
                    throw new InvalidOperationException(
                        $"Native style source attribution size {attributionSize} exceeds buffer length {buffer.Length}."
                    );
                }

                fixed (byte* bufferPointer = buffer)
                {
                    attribution = RuntimeStructs.CopyUtf8((sbyte*)bufferPointer, attributionSize);
                }
            }
        }

        return new SourceInfo(
            sourceId,
            (SourceType)info.type,
            info.type,
            info.is_volatile != 0,
            attribution
        );
    }

    /// <summary>Lists style source IDs in style order.</summary>
    public string[] StyleSourceIds()
    {
        MlnStyleIdList list = default;
        NativeStatus.Check(NativeMethods.mln_map_list_style_source_ids(Handle, &list));
        return CopyStyleIdList(list);
    }

    /// <summary>Adds a GeoJSON source that loads data from a URL.</summary>
    /// <remarks>
    /// <paramref name="options" /> is fixed when the source is created;
    /// <see cref="SetGeoJsonSourceUrl" /> and <see cref="SetGeoJsonSourceData" /> keep the options
    /// the source was added with.
    /// </remarks>
    public void AddGeoJsonSourceUrl(string sourceId, string url, GeoJsonSourceOptions? options)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeGeoJsonSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_geojson_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Updates a GeoJSON source to load data from a URL.</summary>
    public void SetGeoJsonSourceUrl(string sourceId, string url)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        NativeStatus.Check(
            NativeMethods.mln_map_set_geojson_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value
            )
        );
    }

    /// <summary>Adds a GeoJSON source with inline data.</summary>
    /// <remarks>
    /// <paramref name="options" /> is fixed when the source is created;
    /// <see cref="SetGeoJsonSourceUrl" /> and <see cref="SetGeoJsonSourceData" /> keep the options
    /// the source was added with.
    /// </remarks>
    public void AddGeoJsonSourceData(string sourceId, GeoJson data, GeoJsonSourceOptions? options)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeData = NativeGeoJson.From(data);
        using var nativeOptions = options is null ? null : NativeGeoJsonSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_geojson_source_data(
                Handle,
                nativeSourceId.Value,
                nativeData.Pointer,
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Updates a GeoJSON source with inline data.</summary>
    public void SetGeoJsonSourceData(string sourceId, GeoJson data)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeData = NativeGeoJson.From(data);
        NativeStatus.Check(
            NativeMethods.mln_map_set_geojson_source_data(
                Handle,
                nativeSourceId.Value,
                nativeData.Pointer
            )
        );
    }

    /// <summary>Adds a custom geometry source with tile callbacks.</summary>
    public void AddCustomGeometrySource(string sourceId, CustomGeometrySourceOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var sourceState = new CustomGeometrySourceState(options);
        try
        {
            var descriptor = sourceState.Descriptor;
            NativeStatus.Check(
                AddCustomGeometrySourceNative(Handle, nativeSourceId.Value, &descriptor)
            );
            if (customGeometrySources.Remove(sourceId, out var previous))
            {
                previous.Dispose();
            }
            customGeometrySources[sourceId] = sourceState;
        }
        catch
        {
            sourceState.Dispose();
            throw;
        }
    }

    /// <summary>Sets custom geometry source tile data.</summary>
    public void SetCustomGeometrySourceTileData(
        string sourceId,
        CanonicalTileId tileId,
        GeoJson data
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeData = NativeGeoJson.From(data);
        var nativeTileId = StyleStructs.ToNative(tileId);
        NativeStatus.Check(
            NativeMethods.mln_map_set_custom_geometry_source_tile_data(
                Handle,
                nativeSourceId.Value,
                nativeTileId,
                nativeData.Pointer
            )
        );
    }

    /// <summary>Invalidates one custom geometry source tile.</summary>
    public void InvalidateCustomGeometrySourceTile(string sourceId, CanonicalTileId tileId)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        NativeStatus.Check(
            NativeMethods.mln_map_invalidate_custom_geometry_source_tile(
                Handle,
                nativeSourceId.Value,
                StyleStructs.ToNative(tileId)
            )
        );
    }

    /// <summary>Invalidates custom geometry source tiles that intersect bounds.</summary>
    public void InvalidateCustomGeometrySourceRegion(string sourceId, LatLngBounds bounds)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        NativeStatus.Check(
            NativeMethods.mln_map_invalidate_custom_geometry_source_region(
                Handle,
                nativeSourceId.Value,
                MapStructs.ToNative(bounds)
            )
        );
    }

    /// <summary>Adds a vector source that loads TileJSON from a URL.</summary>
    public void AddVectorSourceUrl(string sourceId, string url, TileSourceOptions? options)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_vector_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Adds a vector source from inline tile URL templates.</summary>
    public void AddVectorSourceTiles(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options
    )
    {
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
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Adds a raster source that loads TileJSON from a URL.</summary>
    public void AddRasterSourceUrl(string sourceId, string url, TileSourceOptions? options)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Adds a raster source from inline tile URL templates.</summary>
    public void AddRasterSourceTiles(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options
    )
    {
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
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Adds a raster DEM source that loads TileJSON from a URL.</summary>
    public void AddRasterDemSourceUrl(string sourceId, string url, TileSourceOptions? options)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_dem_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Adds a raster DEM source from inline tile URL templates.</summary>
    public void AddRasterDemSourceTiles(
        string sourceId,
        IReadOnlyList<string> tiles,
        TileSourceOptions? options
    )
    {
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
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Sets or replaces a style image.</summary>
    public void SetStyleImage(
        string imageId,
        PremultipliedRgba8Image image,
        StyleImageOptions? options
    )
    {
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
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Removes a style image and reports whether it existed.</summary>
    public bool RemoveStyleImage(string imageId)
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        bool removed = false;
        NativeStatus.Check(
            NativeMethods.mln_map_remove_style_image(Handle, nativeImageId.Value, &removed)
        );
        return removed;
    }

    /// <summary>Whether a style image exists.</summary>
    public bool StyleImageExists(string imageId)
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        bool exists = false;
        NativeStatus.Check(
            NativeMethods.mln_map_style_image_exists(Handle, nativeImageId.Value, &exists)
        );
        return exists;
    }

    /// <summary>Gets style image metadata when the image exists.</summary>
    public StyleImageInfo? StyleImageInfo(string imageId)
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        var info = NativeMethods.mln_style_image_info_default();
        bool found = false;
        NativeStatus.Check(
            NativeMethods.mln_map_get_style_image_info(Handle, nativeImageId.Value, &info, &found)
        );
        return found ? StyleStructs.FromNative(info) : null;
    }

    /// <summary>Copies a style image's stretchable intervals when it exists.</summary>
    /// <remarks>Probes the required counts, then copies.</remarks>
    public (
        IReadOnlyList<ImageStretch> StretchX,
        IReadOnlyList<ImageStretch> StretchY
    )? StyleImageStretches(string imageId)
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        nuint xCount = 0;
        nuint yCount = 0;
        bool found = false;
        NativeStatus.Check(
            NativeMethods.mln_map_copy_style_image_stretches(
                Handle,
                nativeImageId.Value,
                null,
                0,
                &xCount,
                null,
                0,
                &yCount,
                &found
            )
        );
        if (!found)
        {
            return null;
        }

        var rawX = new mln_image_stretch[checked((int)xCount)];
        var rawY = new mln_image_stretch[checked((int)yCount)];
        fixed (mln_image_stretch* pointerX = rawX)
        fixed (mln_image_stretch* pointerY = rawY)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_copy_style_image_stretches(
                    Handle,
                    nativeImageId.Value,
                    rawX.Length == 0 ? null : pointerX,
                    (nuint)rawX.Length,
                    &xCount,
                    rawY.Length == 0 ? null : pointerY,
                    (nuint)rawY.Length,
                    &yCount,
                    &found
                )
            );
        }
        return (ToStretches(rawX), ToStretches(rawY));
    }

    private static IReadOnlyList<ImageStretch> ToStretches(mln_image_stretch[] raw) =>
        Array.ConvertAll(raw, stretch => new ImageStretch(stretch.from, stretch.to));

    private static IReadOnlyList<ImageStretch>? NullIfEmpty(IReadOnlyList<ImageStretch>? values) =>
        values is null || values.Count == 0 ? null : values;

    /// <summary>Copies a style image as premultiplied RGBA8 pixels when it exists.</summary>
    public StyleImage? CopyStyleImagePremultipliedRgba8(string imageId)
    {
        var info = StyleImageInfo(imageId);
        if (info is null)
        {
            return null;
        }

        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        var bytes = new byte[checked((int)info.ByteLength)];
        nuint byteLength = 0;
        bool found = false;
        fixed (byte* bytesPointer = bytes)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_copy_style_image_premultiplied_rgba8(
                    Handle,
                    nativeImageId.Value,
                    bytes.Length == 0 ? null : bytesPointer,
                    (nuint)bytes.Length,
                    &byteLength,
                    &found
                )
            );
        }

        if (!found)
        {
            return null;
        }

        if (byteLength > (nuint)bytes.Length)
        {
            throw new InvalidOperationException(
                $"Native style image byte length {byteLength} exceeds buffer length {bytes.Length}."
            );
        }

        if (byteLength != (nuint)bytes.Length)
        {
            Array.Resize(ref bytes, checked((int)byteLength));
        }

        // Native storage keeps no empty-versus-absent distinction for stretches, so an image
        // without intervals reports them as absent.
        var stretches = StyleImageStretches(imageId);
        return new StyleImage(
            new PremultipliedRgba8Image(
                bytes,
                new TextureImageInfo(info.Width, info.Height, info.Stride, (ulong)byteLength)
            ),
            new StyleImageOptions
            {
                PixelRatio = info.PixelRatio,
                Sdf = info.Sdf,
                StretchX = NullIfEmpty(stretches?.StretchX),
                StretchY = NullIfEmpty(stretches?.StretchY),
                Content = info.Content,
                TextFitWidth = info.TextFitWidth,
                TextFitHeight = info.TextFitHeight,
            }
        );
    }

    /// <summary>Adds an image source that loads image data from a URL.</summary>
    public void AddImageSourceUrl(string sourceId, IReadOnlyList<LatLng> coordinates, string url)
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
                    nativeUrl.Value
                )
            );
        }
    }

    /// <summary>Adds an image source with inline premultiplied RGBA8 image data.</summary>
    public void AddImageSourceImage(
        string sourceId,
        IReadOnlyList<LatLng> coordinates,
        PremultipliedRgba8Image image
    )
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
                    &imageValue
                )
            );
        }
    }

    /// <summary>Updates an image source to load image data from a URL.</summary>
    public void SetImageSourceUrl(string sourceId, string url)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        NativeStatus.Check(
            NativeMethods.mln_map_set_image_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value
            )
        );
    }

    /// <summary>Updates an image source with inline premultiplied RGBA8 image data.</summary>
    public void SetImageSourceImage(string sourceId, PremultipliedRgba8Image image)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeImage = NativeStyleImage.From(image);
        var imageValue = nativeImage.Value;
        NativeStatus.Check(
            NativeMethods.mln_map_set_image_source_image(Handle, nativeSourceId.Value, &imageValue)
        );
    }

    /// <summary>Updates image source coordinates.</summary>
    public void SetImageSourceCoordinates(string sourceId, IReadOnlyList<LatLng> coordinates)
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
                    (nuint)nativeCoordinates.Length
                )
            );
        }
    }

    /// <summary>Gets image source coordinates when the source exists.</summary>
    public LatLng[]? GetImageSourceCoordinates(string sourceId)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var coordinates = new mln_lat_lng[4];
        nuint coordinateCount = 0;
        bool found = false;
        fixed (mln_lat_lng* coordinatesPointer = coordinates)
        {
            NativeStatus.Check(
                NativeMethods.mln_map_get_image_source_coordinates(
                    Handle,
                    nativeSourceId.Value,
                    coordinatesPointer,
                    (nuint)coordinates.Length,
                    &coordinateCount,
                    &found
                )
            );
        }

        if (!found)
        {
            return null;
        }

        var result = new LatLng[checked((int)coordinateCount)];
        for (var index = 0; index < result.Length; index++)
        {
            result[index] = CoreStructs.FromNative(coordinates[index]);
        }
        return result;
    }

    /// <summary>Adds a hillshade layer for a raster DEM source.</summary>
    public void AddHillshadeLayer(string layerId, string sourceId, string beforeLayerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_hillshade_layer(
                Handle,
                nativeLayerId.Value,
                nativeSourceId.Value,
                nativeBeforeLayerId.Value
            )
        );
    }

    /// <summary>Adds a color-relief layer for a raster DEM source.</summary>
    public void AddColorReliefLayer(string layerId, string sourceId, string beforeLayerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_color_relief_layer(
                Handle,
                nativeLayerId.Value,
                nativeSourceId.Value,
                nativeBeforeLayerId.Value
            )
        );
    }

    /// <summary>Adds a source-free location indicator layer.</summary>
    public void AddLocationIndicatorLayer(string layerId, string beforeLayerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_location_indicator_layer(
                Handle,
                nativeLayerId.Value,
                nativeBeforeLayerId.Value
            )
        );
    }

    /// <summary>Sets a location indicator layer location.</summary>
    public void SetLocationIndicatorLocation(string layerId, LatLng coordinate, double altitude)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_location_indicator_location(
                Handle,
                nativeLayerId.Value,
                CoreStructs.ToNative(coordinate),
                altitude
            )
        );
    }

    /// <summary>Sets a location indicator layer bearing in degrees.</summary>
    public void SetLocationIndicatorBearing(string layerId, double bearing)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_location_indicator_bearing(
                Handle,
                nativeLayerId.Value,
                bearing
            )
        );
    }

    /// <summary>Sets a location indicator layer accuracy radius in logical pixels.</summary>
    public void SetLocationIndicatorAccuracyRadius(string layerId, double radius)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_location_indicator_accuracy_radius(
                Handle,
                nativeLayerId.Value,
                radius
            )
        );
    }

    /// <summary>Sets a location indicator layer image-name property.</summary>
    public void SetLocationIndicatorImageName(
        string layerId,
        LocationIndicatorImageKind imageKind,
        string imageId
    )
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_location_indicator_image_name(
                Handle,
                nativeLayerId.Value,
                (uint)imageKind,
                nativeImageId.Value
            )
        );
    }

    /// <summary>Adds a style layer from a JSON-like value.</summary>
    public void AddStyleLayerJson(JsonValue layerJson, string beforeLayerId)
    {
        using var nativeJson = NativeJsonValue.From(layerJson);
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_style_layer_json(
                Handle,
                nativeJson.Pointer,
                nativeBeforeLayerId.Value
            )
        );
    }

    /// <summary>Removes a style layer and reports whether it existed.</summary>
    public bool RemoveStyleLayer(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        bool removed = false;
        NativeStatus.Check(
            NativeMethods.mln_map_remove_style_layer(Handle, nativeLayerId.Value, &removed)
        );
        return removed;
    }

    /// <summary>Whether a style layer exists.</summary>
    public bool StyleLayerExists(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        bool exists = false;
        NativeStatus.Check(
            NativeMethods.mln_map_style_layer_exists(Handle, nativeLayerId.Value, &exists)
        );
        return exists;
    }

    /// <summary>Gets a style layer type when the layer exists.</summary>
    public string? StyleLayerType(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        mln_string_view layerType = default;
        bool found = false;
        NativeStatus.Check(
            NativeMethods.mln_map_get_style_layer_type(
                Handle,
                nativeLayerId.Value,
                &layerType,
                &found
            )
        );
        return found ? RuntimeStructs.CopyUtf8(layerType.data, layerType.size) : null;
    }

    /// <summary>Lists style layer IDs in style order.</summary>
    public string[] StyleLayerIds()
    {
        MlnStyleIdList list = default;
        NativeStatus.Check(NativeMethods.mln_map_list_style_layer_ids(Handle, &list));
        return CopyStyleIdList(list);
    }

    /// <summary>Moves a style layer before another layer, or to the top when beforeLayerId is empty.</summary>
    public void MoveStyleLayer(string layerId, string beforeLayerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_move_style_layer(
                Handle,
                nativeLayerId.Value,
                nativeBeforeLayerId.Value
            )
        );
    }

    /// <summary>Gets a full style-spec layer JSON snapshot when the layer exists.</summary>
    public JsonValue? GetStyleLayerJson(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        MlnJsonSnapshot snapshot = default;
        bool found = false;
        NativeStatus.Check(
            NativeMethods.mln_map_get_style_layer_json(
                Handle,
                nativeLayerId.Value,
                &snapshot,
                &found
            )
        );
        return found ? ValueStructs.ReadJsonSnapshot(snapshot) : null;
    }

    /// <summary>Sets the style light document from a JSON-like value.</summary>
    public void SetStyleLightJson(JsonValue lightJson)
    {
        using var nativeJson = NativeJsonValue.From(lightJson);
        NativeStatus.Check(NativeMethods.mln_map_set_style_light_json(Handle, nativeJson.Pointer));
    }

    /// <summary>Sets one style light property from a JSON-like value.</summary>
    public void SetStyleLightProperty(string propertyName, JsonValue value)
    {
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeJsonValue.From(value);
        NativeStatus.Check(
            NativeMethods.mln_map_set_style_light_property(
                Handle,
                nativePropertyName.Value,
                nativeValue.Pointer
            )
        );
    }

    /// <summary>Gets one style light property snapshot, or null when undefined.</summary>
    public JsonValue? GetStyleLightProperty(string propertyName)
    {
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        MlnJsonSnapshot snapshot = default;
        NativeStatus.Check(
            NativeMethods.mln_map_get_style_light_property(
                Handle,
                nativePropertyName.Value,
                &snapshot
            )
        );
        return ValueStructs.ReadJsonSnapshot(snapshot);
    }

    /// <summary>Sets one layer property from a JSON-like value.</summary>
    public void SetLayerProperty(string layerId, string propertyName, JsonValue value)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeJsonValue.From(value);
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_property(
                Handle,
                nativeLayerId.Value,
                nativePropertyName.Value,
                nativeValue.Pointer
            )
        );
    }

    /// <summary>Gets one layer property snapshot, or null when undefined.</summary>
    public JsonValue? GetLayerProperty(string layerId, string propertyName)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        MlnJsonSnapshot snapshot = default;
        NativeStatus.Check(
            NativeMethods.mln_map_get_layer_property(
                Handle,
                nativeLayerId.Value,
                nativePropertyName.Value,
                &snapshot
            )
        );
        return ValueStructs.ReadJsonSnapshot(snapshot);
    }

    /// <summary>Sets or clears one layer filter from a JSON-like value.</summary>
    public void SetLayerFilter(string layerId, JsonValue? filter)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeFilter = filter is null ? null : NativeJsonValue.From(filter);
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_filter(
                Handle,
                nativeLayerId.Value,
                nativeFilter?.Pointer
            )
        );
    }

    /// <summary>Gets one layer filter snapshot, or null when no filter exists.</summary>
    public JsonValue? GetLayerFilter(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        MlnJsonSnapshot snapshot = default;
        NativeStatus.Check(
            NativeMethods.mln_map_get_layer_filter(Handle, nativeLayerId.Value, &snapshot)
        );
        var value = ValueStructs.ReadJsonSnapshot(snapshot);
        return value is JsonValue.Null ? null : value;
    }

    /// <summary>Sets one layer's source-layer ID.</summary>
    /// <remarks>
    /// Layer types that take no source, such as background, are rejected.
    /// </remarks>
    public void SetLayerSourceLayer(string layerId, string sourceLayer)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceLayer = NativeStringView.From(sourceLayer, nameof(sourceLayer));
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_source_layer(
                Handle,
                nativeLayerId.Value,
                nativeSourceLayer.Value
            )
        );
    }

    /// <summary>Gets one layer's source-layer ID, empty when it carries none.</summary>
    public string GetLayerSourceLayer(string layerId) => CopyLayerText(layerId, sourceLayer: true);

    /// <summary>Sets one layer's source ID.</summary>
    /// <remarks>
    /// Layer types that take no source, such as background, are rejected. The named source need
    /// not exist yet.
    /// </remarks>
    public void SetLayerSourceId(string layerId, string sourceId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_source_id(
                Handle,
                nativeLayerId.Value,
                nativeSourceId.Value
            )
        );
    }

    /// <summary>Gets one layer's source ID, empty when it carries none.</summary>
    public string GetLayerSourceId(string layerId) => CopyLayerText(layerId, sourceLayer: false);

    /// <summary>
    /// Probes the required length, then copies. A null buffer with zero capacity is a size probe
    /// the C API answers with OK.
    /// </summary>
    private string CopyLayerText(string layerId, bool sourceLayer)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        nuint required = 0;
        NativeStatus.Check(
            sourceLayer
                ? NativeMethods.mln_map_copy_layer_source_layer(
                    Handle,
                    nativeLayerId.Value,
                    null,
                    0,
                    &required
                )
                : NativeMethods.mln_map_copy_layer_source_id(
                    Handle,
                    nativeLayerId.Value,
                    null,
                    0,
                    &required
                )
        );
        if (required == 0)
        {
            return string.Empty;
        }

        var buffer = new byte[checked((int)required)];
        nuint copied = 0;
        fixed (byte* bufferPointer = buffer)
        {
            NativeStatus.Check(
                sourceLayer
                    ? NativeMethods.mln_map_copy_layer_source_layer(
                        Handle,
                        nativeLayerId.Value,
                        (sbyte*)bufferPointer,
                        required,
                        &copied
                    )
                    : NativeMethods.mln_map_copy_layer_source_id(
                        Handle,
                        nativeLayerId.Value,
                        (sbyte*)bufferPointer,
                        required,
                        &copied
                    )
            );
            return RuntimeStructs.CopyUtf8((sbyte*)bufferPointer, copied) ?? string.Empty;
        }
    }

    /// <summary>Sets the lowest zoom at which one layer draws.</summary>
    /// <remarks>Pass <c>double.NegativeInfinity</c> for no lower bound.</remarks>
    public void SetLayerMinZoom(string layerId, double minZoom)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_min_zoom(Handle, nativeLayerId.Value, minZoom)
        );
    }

    /// <summary>Gets the lowest zoom at which one layer draws.</summary>
    /// <remarks>A layer with no lower bound reports <c>double.NegativeInfinity</c>.</remarks>
    public double GetLayerMinZoom(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        double minZoom = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_get_layer_min_zoom(Handle, nativeLayerId.Value, &minZoom)
        );
        return minZoom;
    }

    /// <summary>Sets the highest zoom at which one layer draws.</summary>
    /// <remarks>Pass <c>double.PositiveInfinity</c> for no upper bound.</remarks>
    public void SetLayerMaxZoom(string layerId, double maxZoom)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_max_zoom(Handle, nativeLayerId.Value, maxZoom)
        );
    }

    /// <summary>Gets the highest zoom at which one layer draws.</summary>
    /// <remarks>A layer with no upper bound reports <c>double.PositiveInfinity</c>.</remarks>
    public double GetLayerMaxZoom(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        double maxZoom = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_get_layer_max_zoom(Handle, nativeLayerId.Value, &maxZoom)
        );
        return maxZoom;
    }

    /// <summary>Sets whether one layer draws.</summary>
    public void SetLayerVisibility(string layerId, StyleLayerVisibility visibility)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        NativeStatus.Check(
            NativeMethods.mln_map_set_layer_visibility(
                Handle,
                nativeLayerId.Value,
                (uint)visibility
            )
        );
    }

    /// <summary>Gets whether one layer draws.</summary>
    public StyleLayerVisibility GetLayerVisibility(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        uint visibility = 0;
        NativeStatus.Check(
            NativeMethods.mln_map_get_layer_visibility(Handle, nativeLayerId.Value, &visibility)
        );
        return (StyleLayerVisibility)visibility;
    }

    /// <summary>Destroys the map on its owner thread.</summary>
    /// <remarks>
    /// Closing discards this map's queued runtime events and its recorded loading
    /// failure without a flush and without a terminal event. Snapshot any mirrored
    /// state you still need before closing, and drive teardown from the close
    /// result rather than awaiting an event.
    /// </remarks>
    public void Close()
    {
        state.Close();
        runtime.UnregisterMap(this);
        ClearCustomGeometrySources();
    }

    internal int CustomGeometrySourceCountForTest => customGeometrySources.Count;

    internal CustomGeometrySourceState? CustomGeometrySourceForTest(string sourceId) =>
        customGeometrySources.GetValueOrDefault(sourceId);

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

    internal void ReleaseDetachedCustomGeometrySources()
    {
        foreach (var (sourceId, sourceState) in customGeometrySources.ToArray())
        {
            var sourceType = StyleSourceType(sourceId);
            if (sourceType == SourceType.CustomVector)
            {
                continue;
            }

            if (customGeometrySources.Remove(sourceId))
            {
                sourceState.Dispose();
            }
        }
    }

    private void ClearCustomGeometrySources()
    {
        foreach (var source in customGeometrySources.Values)
        {
            source.Dispose();
        }
        customGeometrySources.Clear();
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
                mln_string_view id = default;
                NativeStatus.Check(NativeMethods.mln_style_id_list_get(list, (nuint)index, &id));
                ids[index] = RuntimeStructs.CopyUtf8(id.data, id.size);
            }

            return ids;
        }
        finally
        {
            NativeMethods.mln_style_id_list_destroy(list);
        }
    }

    /// <inheritdoc />
    public void Dispose()
    {
        if (state.TryClose())
        {
            runtime.UnregisterMap(this);
            ClearCustomGeometrySources();
        }
        GC.KeepAlive(runtime);
    }
}
