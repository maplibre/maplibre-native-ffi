using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Json;
using Maplibre.Native.Render;
using Maplibre.Native.Runtime;
using Maplibre.Native.Style;

namespace Maplibre.Native.Map;

/// <summary>Owner-thread map handle bound to a runtime.</summary>
public sealed unsafe class MapHandle : IDisposable
{
    private readonly RuntimeHandle runtime;
    private readonly nint nativeAddress;
    private readonly NativeHandleState<mln_map> state;
    private readonly Dictionary<string, CustomGeometrySourceState> customGeometrySources = [];

    private MapHandle(RuntimeHandle runtime, mln_map* handle)
    {
        this.runtime = runtime;
        nativeAddress = (nint)handle;
        state = new NativeHandleState<mln_map>(
            handle,
            static handle => NativeMethods.mln_map_destroy(handle),
            nameof(MapHandle)
        );
    }

    /// <summary>Creates a map from a runtime on the runtime owner thread.</summary>
    public static MapHandle Create(RuntimeHandle runtime, MapOptions? options = null)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        options ??= new MapOptions();
        var nativeOptions = options.ToNative();
        mln_map* map = null;

        NativeStatus.Check(NativeMethods.mln_map_create(runtime.Pointer, &nativeOptions, &map));
        var handle = new MapHandle(runtime, map);
        runtime.RegisterMap(handle);
        return handle;
    }

    internal mln_map* Pointer => state.Pointer;

    internal nint NativeAddress => nativeAddress;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Requests a repaint for a continuous map.</summary>
    public void RequestRepaint()
    {
        NativeStatus.Check(NativeMethods.mln_map_request_repaint(Pointer));
    }

    /// <summary>Requests an asynchronous still-image render for a static map.</summary>
    public void RequestStillImage()
    {
        NativeStatus.Check(NativeMethods.mln_map_request_still_image(Pointer));
    }

    /// <summary>Sets native debug drawing options.</summary>
    public void SetDebugOptions(DebugOptions options)
    {
        NativeStatus.Check(NativeMethods.mln_map_set_debug_options(Pointer, (uint)options));
    }

    /// <summary>Gets native debug drawing options.</summary>
    public DebugOptions GetDebugOptions()
    {
        uint options = 0;
        NativeStatus.Check(NativeMethods.mln_map_get_debug_options(Pointer, &options));
        return (DebugOptions)options;
    }

    /// <summary>Shows or hides the built-in rendering statistics overlay.</summary>
    public void SetRenderingStatsViewEnabled(bool enabled)
    {
        NativeStatus.Check(
            NativeMethods.mln_map_set_rendering_stats_view_enabled(
                Pointer,
                enabled ? (byte)1 : (byte)0
            )
        );
    }

    /// <summary>Whether the built-in rendering statistics overlay is enabled.</summary>
    public bool GetRenderingStatsViewEnabled()
    {
        bool enabled = false;
        NativeStatus.Check(
            NativeMethods.mln_map_get_rendering_stats_view_enabled(Pointer, &enabled)
        );
        return enabled;
    }

    /// <summary>Whether the native map reports all required resources loaded.</summary>
    public bool IsFullyLoaded()
    {
        bool loaded = false;
        NativeStatus.Check(NativeMethods.mln_map_is_fully_loaded(Pointer, &loaded));
        return loaded;
    }

    /// <summary>Asks the native map to write debug logs through the native log system.</summary>
    public void DumpDebugLogs()
    {
        NativeStatus.Check(NativeMethods.mln_map_dump_debug_logs(Pointer));
    }

    /// <summary>Gets the map's viewport options.</summary>
    public ViewportOptions GetViewportOptions()
    {
        var options = NativeMethods.mln_map_viewport_options_default();
        NativeStatus.Check(NativeMethods.mln_map_get_viewport_options(Pointer, &options));
        return MapStructs.ViewportOptionsFromNative(options);
    }

    /// <summary>Sets viewport options, applying only non-null descriptor fields.</summary>
    public void SetViewportOptions(ViewportOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        NativeStatus.Check(NativeMethods.mln_map_set_viewport_options(Pointer, &nativeOptions));
    }

    /// <summary>Gets tile tuning options.</summary>
    public TileOptions GetTileOptions()
    {
        var options = NativeMethods.mln_map_tile_options_default();
        NativeStatus.Check(NativeMethods.mln_map_get_tile_options(Pointer, &options));
        return MapStructs.TileOptionsFromNative(options);
    }

    /// <summary>Sets tile tuning options, applying only non-null descriptor fields.</summary>
    public void SetTileOptions(TileOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        NativeStatus.Check(NativeMethods.mln_map_set_tile_options(Pointer, &nativeOptions));
    }

    /// <summary>Gets the current camera descriptor.</summary>
    public CameraOptions GetCamera()
    {
        var camera = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(NativeMethods.mln_map_get_camera(Pointer, &camera));
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Moves immediately to the camera descriptor, applying only non-null fields.</summary>
    public void JumpTo(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        NativeStatus.Check(NativeMethods.mln_map_jump_to(Pointer, &nativeCamera));
    }

    /// <summary>Eases to the camera descriptor.</summary>
    public void EaseTo(CameraOptions camera)
    {
        EaseTo(camera, animation: null);
    }

    /// <summary>Eases to the camera descriptor with animation options.</summary>
    public void EaseTo(CameraOptions camera, AnimationOptions? animation)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_ease_to(
                Pointer,
                &nativeCamera,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Flies to the camera descriptor.</summary>
    public void FlyTo(CameraOptions camera)
    {
        FlyTo(camera, animation: null);
    }

    /// <summary>Flies to the camera descriptor with animation options.</summary>
    public void FlyTo(CameraOptions camera, AnimationOptions? animation)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_fly_to(
                Pointer,
                &nativeCamera,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Moves the map by a screen delta.</summary>
    public void MoveBy(double deltaX, double deltaY)
    {
        NativeStatus.Check(NativeMethods.mln_map_move_by(Pointer, deltaX, deltaY));
    }

    /// <summary>Moves the map by a screen delta with default animation.</summary>
    public void MoveByAnimated(double deltaX, double deltaY)
    {
        MoveByAnimated(deltaX, deltaY, animation: null);
    }

    /// <summary>Moves the map by a screen delta with animation options.</summary>
    public void MoveByAnimated(double deltaX, double deltaY, AnimationOptions? animation)
    {
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_move_by_animated(
                Pointer,
                deltaX,
                deltaY,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Scales the map around its default anchor.</summary>
    public void ScaleBy(double scale)
    {
        ScaleBy(scale, anchor: null);
    }

    /// <summary>Scales the map around a screen anchor.</summary>
    public void ScaleBy(double scale, ScreenPoint? anchor)
    {
        var nativeAnchor = anchor is { } value ? MapStructs.ToNative(value) : default;
        NativeStatus.Check(
            NativeMethods.mln_map_scale_by(Pointer, scale, anchor.HasValue ? &nativeAnchor : null)
        );
    }

    /// <summary>Scales the map around its default anchor with default animation.</summary>
    public void ScaleByAnimated(double scale)
    {
        ScaleByAnimated(scale, anchor: null, animation: null);
    }

    /// <summary>Scales the map around a screen anchor with default animation.</summary>
    public void ScaleByAnimated(double scale, ScreenPoint anchor)
    {
        ScaleByAnimated(scale, anchor, animation: null);
    }

    /// <summary>Scales the map around its default anchor with animation options.</summary>
    public void ScaleByAnimated(double scale, AnimationOptions animation)
    {
        ScaleByAnimated(scale, anchor: null, animation);
    }

    /// <summary>Scales the map around a screen anchor with animation options.</summary>
    public void ScaleByAnimated(double scale, ScreenPoint? anchor, AnimationOptions? animation)
    {
        var nativeAnchor = anchor is { } anchorValue ? MapStructs.ToNative(anchorValue) : default;
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_scale_by_animated(
                Pointer,
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
        NativeStatus.Check(NativeMethods.mln_map_rotate_by(Pointer, nativeFirst, nativeSecond));
    }

    /// <summary>Rotates around two screen points with default animation.</summary>
    public void RotateByAnimated(ScreenPoint first, ScreenPoint second)
    {
        RotateByAnimated(first, second, animation: null);
    }

    /// <summary>Rotates around two screen points with animation options.</summary>
    public void RotateByAnimated(ScreenPoint first, ScreenPoint second, AnimationOptions? animation)
    {
        var nativeFirst = MapStructs.ToNative(first);
        var nativeSecond = MapStructs.ToNative(second);
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_rotate_by_animated(
                Pointer,
                nativeFirst,
                nativeSecond,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Pitches the map by a delta in degrees.</summary>
    public void PitchBy(double pitch)
    {
        NativeStatus.Check(NativeMethods.mln_map_pitch_by(Pointer, pitch));
    }

    /// <summary>Pitches the map by a delta in degrees with default animation.</summary>
    public void PitchByAnimated(double pitch)
    {
        PitchByAnimated(pitch, animation: null);
    }

    /// <summary>Pitches the map by a delta in degrees with animation options.</summary>
    public void PitchByAnimated(double pitch, AnimationOptions? animation)
    {
        var nativeAnimation = animation is null ? default : MapStructs.ToNative(animation);
        NativeStatus.Check(
            NativeMethods.mln_map_pitch_by_animated(
                Pointer,
                pitch,
                animation is null ? null : &nativeAnimation
            )
        );
    }

    /// <summary>Cancels in-flight camera transitions.</summary>
    public void CancelTransitions()
    {
        NativeStatus.Check(NativeMethods.mln_map_cancel_transitions(Pointer));
    }

    /// <summary>Calculates a camera that fits geographic bounds.</summary>
    public CameraOptions CameraForLatLngBounds(LatLngBounds bounds)
    {
        return CameraForLatLngBounds(bounds, fitOptions: null);
    }

    /// <summary>Calculates a camera that fits geographic bounds and fit options.</summary>
    public CameraOptions CameraForLatLngBounds(LatLngBounds bounds, CameraFitOptions? fitOptions)
    {
        var nativeBounds = MapStructs.ToNative(bounds);
        var nativeFitOptions = fitOptions is null ? default : MapStructs.ToNative(fitOptions);
        var camera = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(
            NativeMethods.mln_map_camera_for_lat_lng_bounds(
                Pointer,
                nativeBounds,
                fitOptions is null ? null : &nativeFitOptions,
                &camera
            )
        );
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Calculates a camera that fits geographic coordinates.</summary>
    public CameraOptions CameraForLatLngs(IReadOnlyList<LatLng> coordinates)
    {
        return CameraForLatLngs(coordinates, fitOptions: null);
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
                    Pointer,
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
    public CameraOptions CameraForGeometry(Geometry geometry, CameraFitOptions? fitOptions = null)
    {
        using var nativeGeometry = NativeGeometry.From(geometry);
        var nativeFitOptions = fitOptions is null ? default : MapStructs.ToNative(fitOptions);
        var camera = NativeMethods.mln_camera_options_default();
        NativeStatus.Check(
            NativeMethods.mln_map_camera_for_geometry(
                Pointer,
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
            NativeMethods.mln_map_lat_lng_bounds_for_camera(Pointer, &nativeCamera, &bounds)
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
                Pointer,
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
        NativeStatus.Check(NativeMethods.mln_map_get_bounds(Pointer, &options));
        return MapStructs.BoundOptionsFromNative(options);
    }

    /// <summary>Sets map bounds constraints, applying only non-null descriptor fields.</summary>
    public void SetBounds(BoundOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        NativeStatus.Check(NativeMethods.mln_map_set_bounds(Pointer, &nativeOptions));
    }

    /// <summary>Gets free-camera options.</summary>
    public FreeCameraOptions GetFreeCameraOptions()
    {
        var options = NativeMethods.mln_free_camera_options_default();
        NativeStatus.Check(NativeMethods.mln_map_get_free_camera_options(Pointer, &options));
        return MapStructs.FreeCameraOptionsFromNative(options);
    }

    /// <summary>Sets free-camera options, applying only non-null descriptor fields.</summary>
    public void SetFreeCameraOptions(FreeCameraOptions options)
    {
        var nativeOptions = MapStructs.ToNative(options);
        NativeStatus.Check(NativeMethods.mln_map_set_free_camera_options(Pointer, &nativeOptions));
    }

    /// <summary>Converts a geographic coordinate to a screen pixel using the current map projection.</summary>
    public ScreenPoint PixelForLatLng(LatLng coordinate)
    {
        var nativeCoordinate = CoreStructs.ToNative(coordinate);
        mln_screen_point point = default;
        NativeStatus.Check(
            NativeMethods.mln_map_pixel_for_lat_lng(Pointer, nativeCoordinate, &point)
        );
        return MapStructs.FromNative(point);
    }

    /// <summary>Converts a screen pixel to a geographic coordinate using the current map projection.</summary>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        var nativePoint = MapStructs.ToNative(point);
        mln_lat_lng coordinate = default;
        NativeStatus.Check(
            NativeMethods.mln_map_lat_lng_for_pixel(Pointer, nativePoint, &coordinate)
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
                    Pointer,
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
                    Pointer,
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
        NativeStatus.Check(NativeMethods.mln_map_get_projection_mode(Pointer, &mode));
        return MapStructs.ProjectionModeOptionsFromNative(mode);
    }

    /// <summary>Sets projection mode options, applying only non-null descriptor fields.</summary>
    public void SetProjectionMode(ProjectionModeOptions mode)
    {
        var nativeMode = MapStructs.ToNative(mode);
        NativeStatus.Check(NativeMethods.mln_map_set_projection_mode(Pointer, &nativeMode));
    }

    /// <summary>Loads a style URL through MapLibre Native style APIs.</summary>
    public void SetStyleUrl(string url)
    {
        ArgumentNullException.ThrowIfNull(url);
        using var nativeUrl = NativeUtf8String.FromNullableString(url, nameof(url));
        NativeStatus.Check(NativeMethods.mln_map_set_style_url(Pointer, nativeUrl.Pointer));
    }

    /// <summary>Loads inline style JSON through MapLibre Native style APIs.</summary>
    public void SetStyleJson(string json)
    {
        ArgumentNullException.ThrowIfNull(json);
        using var nativeJson = NativeUtf8String.FromNullableString(json, nameof(json));
        NativeStatus.Check(NativeMethods.mln_map_set_style_json(Pointer, nativeJson.Pointer));
        ClearCustomGeometrySources();
    }

    /// <summary>Adds a style source from a JSON-like value.</summary>
    public void AddStyleSourceJson(string sourceId, JsonValue sourceJson)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeJson = NativeJsonValue.From(sourceJson);
        NativeStatus.Check(
            NativeMethods.mln_map_add_style_source_json(
                Pointer,
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
            NativeMethods.mln_map_remove_style_source(Pointer, nativeSourceId.Value, &removed)
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
            NativeMethods.mln_map_style_source_exists(Pointer, nativeSourceId.Value, &exists)
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
                Pointer,
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
            NativeMethods.mln_map_get_style_source_info(
                Pointer,
                nativeSourceId.Value,
                &info,
                &found
            )
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
                            Pointer,
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
        mln_style_id_list* list = null;
        NativeStatus.Check(NativeMethods.mln_map_list_style_source_ids(Pointer, &list));
        return CopyStyleIdList(list);
    }

    /// <summary>Adds a GeoJSON source that loads data from a URL.</summary>
    public void AddGeoJsonSourceUrl(string sourceId, string url)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        NativeStatus.Check(
            NativeMethods.mln_map_add_geojson_source_url(
                Pointer,
                nativeSourceId.Value,
                nativeUrl.Value
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
                Pointer,
                nativeSourceId.Value,
                nativeUrl.Value
            )
        );
    }

    /// <summary>Adds a GeoJSON source with inline data.</summary>
    public void AddGeoJsonSourceData(string sourceId, GeoJson data)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeData = NativeGeoJson.From(data);
        NativeStatus.Check(
            NativeMethods.mln_map_add_geojson_source_data(
                Pointer,
                nativeSourceId.Value,
                nativeData.Pointer
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
                Pointer,
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
                NativeMethods.mln_map_add_custom_geometry_source(
                    Pointer,
                    nativeSourceId.Value,
                    &descriptor
                )
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
                Pointer,
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
                Pointer,
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
                Pointer,
                nativeSourceId.Value,
                MapStructs.ToNative(bounds)
            )
        );
    }

    /// <summary>Adds a vector source that loads TileJSON from a URL.</summary>
    public void AddVectorSourceUrl(string sourceId, string url, TileSourceOptions? options = null)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_vector_source_url(
                Pointer,
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
        TileSourceOptions? options = null
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_vector_source_tiles(
                Pointer,
                nativeSourceId.Value,
                nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                nativeTiles.Count,
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Adds a raster source that loads TileJSON from a URL.</summary>
    public void AddRasterSourceUrl(string sourceId, string url, TileSourceOptions? options = null)
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_source_url(
                Pointer,
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
        TileSourceOptions? options = null
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_source_tiles(
                Pointer,
                nativeSourceId.Value,
                nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                nativeTiles.Count,
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Adds a raster DEM source that loads TileJSON from a URL.</summary>
    public void AddRasterDemSourceUrl(
        string sourceId,
        string url,
        TileSourceOptions? options = null
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_dem_source_url(
                Pointer,
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
        TileSourceOptions? options = null
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeTiles = NativeStringViewArray.From(tiles, nameof(tiles));
        using var nativeOptions = options is null ? null : NativeTileSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        NativeStatus.Check(
            NativeMethods.mln_map_add_raster_dem_source_tiles(
                Pointer,
                nativeSourceId.Value,
                nativeTiles.Count == 0 ? null : nativeTiles.Pointer,
                nativeTiles.Count,
                nativeOptions is null ? null : &optionsValue
            )
        );
    }

    /// <summary>Sets or replaces a style image.</summary>
    public void SetStyleImage(string imageId, StyleImage image)
    {
        ArgumentNullException.ThrowIfNull(image);
        SetStyleImage(imageId, image.Image, image.Options);
    }

    /// <summary>Sets or replaces a style image.</summary>
    public void SetStyleImage(
        string imageId,
        PremultipliedRgba8Image image,
        StyleImageOptions? options = null
    )
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        using var nativeImage = NativeStyleImage.From(image);
        var imageValue = nativeImage.Value;
        var nativeOptions = options is null ? default : StyleStructs.ToNative(options);
        NativeStatus.Check(
            NativeMethods.mln_map_set_style_image(
                Pointer,
                nativeImageId.Value,
                &imageValue,
                options is null ? null : &nativeOptions
            )
        );
    }

    /// <summary>Removes a style image and reports whether it existed.</summary>
    public bool RemoveStyleImage(string imageId)
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        bool removed = false;
        NativeStatus.Check(
            NativeMethods.mln_map_remove_style_image(Pointer, nativeImageId.Value, &removed)
        );
        return removed;
    }

    /// <summary>Whether a style image exists.</summary>
    public bool StyleImageExists(string imageId)
    {
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        bool exists = false;
        NativeStatus.Check(
            NativeMethods.mln_map_style_image_exists(Pointer, nativeImageId.Value, &exists)
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
            NativeMethods.mln_map_get_style_image_info(Pointer, nativeImageId.Value, &info, &found)
        );
        return found ? StyleStructs.FromNative(info) : null;
    }

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
                    Pointer,
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

        return new StyleImage(
            new PremultipliedRgba8Image(
                bytes,
                new TextureImageInfo(info.Width, info.Height, info.Stride, (ulong)byteLength)
            ),
            new StyleImageOptions { PixelRatio = info.PixelRatio, Sdf = info.Sdf }
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
                    Pointer,
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
                    Pointer,
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
                Pointer,
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
            NativeMethods.mln_map_set_image_source_image(Pointer, nativeSourceId.Value, &imageValue)
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
                    Pointer,
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
                    Pointer,
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
    public void AddHillshadeLayer(string layerId, string sourceId, string beforeLayerId = "")
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_hillshade_layer(
                Pointer,
                nativeLayerId.Value,
                nativeSourceId.Value,
                nativeBeforeLayerId.Value
            )
        );
    }

    /// <summary>Adds a color-relief layer for a raster DEM source.</summary>
    public void AddColorReliefLayer(string layerId, string sourceId, string beforeLayerId = "")
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_color_relief_layer(
                Pointer,
                nativeLayerId.Value,
                nativeSourceId.Value,
                nativeBeforeLayerId.Value
            )
        );
    }

    /// <summary>Adds a source-free location indicator layer.</summary>
    public void AddLocationIndicatorLayer(string layerId, string beforeLayerId = "")
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_location_indicator_layer(
                Pointer,
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
                Pointer,
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
                Pointer,
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
                Pointer,
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
                Pointer,
                nativeLayerId.Value,
                (uint)imageKind,
                nativeImageId.Value
            )
        );
    }

    /// <summary>Adds a style layer from a JSON-like value.</summary>
    public void AddStyleLayerJson(JsonValue layerJson, string beforeLayerId = "")
    {
        using var nativeJson = NativeJsonValue.From(layerJson);
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_add_style_layer_json(
                Pointer,
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
            NativeMethods.mln_map_remove_style_layer(Pointer, nativeLayerId.Value, &removed)
        );
        return removed;
    }

    /// <summary>Whether a style layer exists.</summary>
    public bool StyleLayerExists(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        bool exists = false;
        NativeStatus.Check(
            NativeMethods.mln_map_style_layer_exists(Pointer, nativeLayerId.Value, &exists)
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
                Pointer,
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
        mln_style_id_list* list = null;
        NativeStatus.Check(NativeMethods.mln_map_list_style_layer_ids(Pointer, &list));
        return CopyStyleIdList(list);
    }

    /// <summary>Moves a style layer before another layer, or to the top when beforeLayerId is empty.</summary>
    public void MoveStyleLayer(string layerId, string beforeLayerId = "")
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        NativeStatus.Check(
            NativeMethods.mln_map_move_style_layer(
                Pointer,
                nativeLayerId.Value,
                nativeBeforeLayerId.Value
            )
        );
    }

    /// <summary>Gets a full style-spec layer JSON snapshot when the layer exists.</summary>
    public JsonValue? GetStyleLayerJson(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        mln_json_snapshot* snapshot = null;
        bool found = false;
        NativeStatus.Check(
            NativeMethods.mln_map_get_style_layer_json(
                Pointer,
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
        NativeStatus.Check(NativeMethods.mln_map_set_style_light_json(Pointer, nativeJson.Pointer));
    }

    /// <summary>Sets one style light property from a JSON-like value.</summary>
    public void SetStyleLightProperty(string propertyName, JsonValue value)
    {
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeJsonValue.From(value);
        NativeStatus.Check(
            NativeMethods.mln_map_set_style_light_property(
                Pointer,
                nativePropertyName.Value,
                nativeValue.Pointer
            )
        );
    }

    /// <summary>Gets one style light property snapshot, or null when undefined.</summary>
    public JsonValue? GetStyleLightProperty(string propertyName)
    {
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        mln_json_snapshot* snapshot = null;
        NativeStatus.Check(
            NativeMethods.mln_map_get_style_light_property(
                Pointer,
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
                Pointer,
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
        mln_json_snapshot* snapshot = null;
        NativeStatus.Check(
            NativeMethods.mln_map_get_layer_property(
                Pointer,
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
                Pointer,
                nativeLayerId.Value,
                nativeFilter?.Pointer
            )
        );
    }

    /// <summary>Gets one layer filter snapshot, or null when no filter exists.</summary>
    public JsonValue? GetLayerFilter(string layerId)
    {
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        mln_json_snapshot* snapshot = null;
        NativeStatus.Check(
            NativeMethods.mln_map_get_layer_filter(Pointer, nativeLayerId.Value, &snapshot)
        );
        var value = ValueStructs.ReadJsonSnapshot(snapshot);
        return value is JsonValue.Null ? null : value;
    }

    /// <summary>Attaches this map to a Metal surface render target.</summary>
    public RenderSessionHandle AttachMetalSurface(MetalSurfaceDescriptor descriptor) =>
        RenderSessionHandle.AttachMetalSurface(this, descriptor);

    /// <summary>Attaches this map to a Vulkan surface render target.</summary>
    public RenderSessionHandle AttachVulkanSurface(VulkanSurfaceDescriptor descriptor) =>
        RenderSessionHandle.AttachVulkanSurface(this, descriptor);

    /// <summary>Attaches this map to an OpenGL surface render target.</summary>
    public RenderSessionHandle AttachOpenGLSurface(OpenGLSurfaceDescriptor descriptor) =>
        RenderSessionHandle.AttachOpenGLSurface(this, descriptor);

    /// <summary>Attaches this map to a session-owned Metal texture render target.</summary>
    public RenderSessionHandle AttachMetalOwnedTexture(MetalOwnedTextureDescriptor descriptor) =>
        RenderSessionHandle.AttachMetalOwnedTexture(this, descriptor);

    /// <summary>Attaches this map to a caller-owned Metal texture render target.</summary>
    public RenderSessionHandle AttachMetalBorrowedTexture(
        MetalBorrowedTextureDescriptor descriptor
    ) => RenderSessionHandle.AttachMetalBorrowedTexture(this, descriptor);

    /// <summary>Attaches this map to a session-owned Vulkan texture render target.</summary>
    public RenderSessionHandle AttachVulkanOwnedTexture(VulkanOwnedTextureDescriptor descriptor) =>
        RenderSessionHandle.AttachVulkanOwnedTexture(this, descriptor);

    /// <summary>Attaches this map to a caller-owned Vulkan texture render target.</summary>
    public RenderSessionHandle AttachVulkanBorrowedTexture(
        VulkanBorrowedTextureDescriptor descriptor
    ) => RenderSessionHandle.AttachVulkanBorrowedTexture(this, descriptor);

    /// <summary>Attaches this map to a session-owned OpenGL texture render target.</summary>
    public RenderSessionHandle AttachOpenGLOwnedTexture(OpenGLOwnedTextureDescriptor descriptor) =>
        RenderSessionHandle.AttachOpenGLOwnedTexture(this, descriptor);

    /// <summary>Attaches this map to a caller-owned OpenGL texture render target.</summary>
    public RenderSessionHandle AttachOpenGLBorrowedTexture(
        OpenGLBorrowedTextureDescriptor descriptor
    ) => RenderSessionHandle.AttachOpenGLBorrowedTexture(this, descriptor);

    /// <summary>Destroys the map on its owner thread.</summary>
    public void Close()
    {
        state.Close();
        runtime.UnregisterMap(this);
        ClearCustomGeometrySources();
    }

    internal int CustomGeometrySourceCountForTest => customGeometrySources.Count;

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

    private static string[] CopyStyleIdList(mln_style_id_list* list)
    {
        if (list is null)
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
