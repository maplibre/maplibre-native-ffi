using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Runtime;

namespace Maplibre.Native.Map;

/// <summary>Owner-thread map handle bound to a runtime.</summary>
public sealed unsafe class MapHandle : IDisposable
{
    private readonly RuntimeHandle runtime;
    private readonly NativeHandleState<mln_map> state;

    private MapHandle(RuntimeHandle runtime, mln_map* handle)
    {
        this.runtime = runtime;
        state = new NativeHandleState<mln_map>(
            handle,
            static handle => NativeMethods.mln_map_destroy(handle),
            nameof(MapHandle));
    }

    /// <summary>Creates a map from a runtime on the runtime owner thread.</summary>
    public static MapHandle Create(RuntimeHandle runtime, MapOptions? options = null)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        options ??= new MapOptions();
        var nativeOptions = options.ToNative();
        mln_map* map = null;

        NativeStatus.Check(NativeMethods.mln_map_create(runtime.Pointer, &nativeOptions, &map));
        return new MapHandle(runtime, map);
    }

    internal mln_map* Pointer => state.Pointer;

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
        NativeStatus.Check(NativeMethods.mln_map_set_rendering_stats_view_enabled(Pointer, enabled ? (byte)1 : (byte)0));
    }

    /// <summary>Whether the built-in rendering statistics overlay is enabled.</summary>
    public bool GetRenderingStatsViewEnabled()
    {
        bool enabled = false;
        NativeStatus.Check(NativeMethods.mln_map_get_rendering_stats_view_enabled(Pointer, &enabled));
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
        NativeStatus.Check(NativeMethods.mln_map_ease_to(Pointer, &nativeCamera, animation is null ? null : &nativeAnimation));
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
        NativeStatus.Check(NativeMethods.mln_map_fly_to(Pointer, &nativeCamera, animation is null ? null : &nativeAnimation));
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
        NativeStatus.Check(NativeMethods.mln_map_move_by_animated(Pointer, deltaX, deltaY, animation is null ? null : &nativeAnimation));
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
        NativeStatus.Check(NativeMethods.mln_map_scale_by(Pointer, scale, anchor.HasValue ? &nativeAnchor : null));
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
        NativeStatus.Check(NativeMethods.mln_map_scale_by_animated(Pointer, scale, anchor.HasValue ? &nativeAnchor : null, animation is null ? null : &nativeAnimation));
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
        NativeStatus.Check(NativeMethods.mln_map_rotate_by_animated(Pointer, nativeFirst, nativeSecond, animation is null ? null : &nativeAnimation));
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
        NativeStatus.Check(NativeMethods.mln_map_pitch_by_animated(Pointer, pitch, animation is null ? null : &nativeAnimation));
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
        NativeStatus.Check(NativeMethods.mln_map_camera_for_lat_lng_bounds(Pointer, nativeBounds, fitOptions is null ? null : &nativeFitOptions, &camera));
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Calculates a camera that fits geographic coordinates.</summary>
    public CameraOptions CameraForLatLngs(IReadOnlyList<LatLng> coordinates)
    {
        return CameraForLatLngs(coordinates, fitOptions: null);
    }

    /// <summary>Calculates a camera that fits geographic coordinates and fit options.</summary>
    public CameraOptions CameraForLatLngs(IReadOnlyList<LatLng> coordinates, CameraFitOptions? fitOptions)
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
            NativeStatus.Check(NativeMethods.mln_map_camera_for_lat_lngs(
                Pointer,
                nativeCoordinates.Length == 0 ? null : coordinatesPointer,
                (nuint)nativeCoordinates.Length,
                fitOptions is null ? null : &nativeFitOptions,
                &camera));
        }
        return MapStructs.CameraOptionsFromNative(camera);
    }

    /// <summary>Calculates geographic bounds for a camera.</summary>
    public LatLngBounds LatLngBoundsForCamera(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        mln_lat_lng_bounds bounds = default;
        NativeStatus.Check(NativeMethods.mln_map_lat_lng_bounds_for_camera(Pointer, &nativeCamera, &bounds));
        return MapStructs.FromNative(bounds);
    }

    /// <summary>Calculates unwrapped geographic bounds for a camera.</summary>
    public LatLngBounds LatLngBoundsForCameraUnwrapped(CameraOptions camera)
    {
        var nativeCamera = MapStructs.ToNative(camera);
        mln_lat_lng_bounds bounds = default;
        NativeStatus.Check(NativeMethods.mln_map_lat_lng_bounds_for_camera_unwrapped(Pointer, &nativeCamera, &bounds));
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
        NativeStatus.Check(NativeMethods.mln_map_pixel_for_lat_lng(Pointer, nativeCoordinate, &point));
        return MapStructs.FromNative(point);
    }

    /// <summary>Converts a screen pixel to a geographic coordinate using the current map projection.</summary>
    public LatLng LatLngForPixel(ScreenPoint point)
    {
        var nativePoint = MapStructs.ToNative(point);
        mln_lat_lng coordinate = default;
        NativeStatus.Check(NativeMethods.mln_map_lat_lng_for_pixel(Pointer, nativePoint, &coordinate));
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
            NativeStatus.Check(NativeMethods.mln_map_pixels_for_lat_lngs(Pointer, coordinatesPointer, (nuint)nativeCoordinates.Length, pointsPointer));
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
            NativeStatus.Check(NativeMethods.mln_map_lat_lngs_for_pixels(Pointer, pointsPointer, (nuint)nativePoints.Length, coordinatesPointer));
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
    }

    /// <summary>Destroys the map on its owner thread.</summary>
    public void Close()
    {
        state.Close();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        state.TryClose();
        GC.KeepAlive(runtime);
    }
}
