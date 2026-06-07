using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Map;

namespace Maplibre.Native.Internal.Struct;

internal static class MapStructs
{
    internal static mln_edge_insets ToNative(EdgeInsets value) =>
        new()
        {
            top = value.Top,
            left = value.Left,
            bottom = value.Bottom,
            right = value.Right,
        };

    internal static EdgeInsets FromNative(mln_edge_insets value) =>
        new(value.top, value.left, value.bottom, value.right);

    internal static mln_screen_point ToNative(ScreenPoint value) =>
        new() { x = value.X, y = value.Y };

    internal static ScreenPoint FromNative(mln_screen_point value) => new(value.x, value.y);

    internal static mln_lat_lng_bounds ToNative(LatLngBounds value) =>
        new()
        {
            southwest = CoreStructs.ToNative(value.Southwest),
            northeast = CoreStructs.ToNative(value.Northeast),
        };

    internal static LatLngBounds FromNative(mln_lat_lng_bounds value) =>
        new(CoreStructs.FromNative(value.southwest), CoreStructs.FromNative(value.northeast));

    internal static mln_vec3 ToNative(Vec3 value) =>
        new()
        {
            x = value.X,
            y = value.Y,
            z = value.Z,
        };

    internal static Vec3 FromNative(mln_vec3 value) => new(value.x, value.y, value.z);

    internal static mln_quaternion ToNative(Quaternion value) =>
        new()
        {
            x = value.X,
            y = value.Y,
            z = value.Z,
            w = value.W,
        };

    internal static Quaternion FromNative(mln_quaternion value) =>
        new(value.x, value.y, value.z, value.w);

    internal static mln_camera_options ToNative(CameraOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_camera_options_default();
        if (options.Center is { } center)
        {
            native.fields |= (uint)mln_camera_option_field.MLN_CAMERA_OPTION_CENTER;
            native.latitude = center.Latitude;
            native.longitude = center.Longitude;
        }
        if (options.CenterAltitude is { } centerAltitude)
        {
            native.fields |= (uint)mln_camera_option_field.MLN_CAMERA_OPTION_CENTER_ALTITUDE;
            native.center_altitude = centerAltitude;
        }
        if (options.Padding is { } padding)
        {
            native.fields |= (uint)mln_camera_option_field.MLN_CAMERA_OPTION_PADDING;
            native.padding = ToNative(padding);
        }
        if (options.Anchor is { } anchor)
        {
            native.fields |= (uint)mln_camera_option_field.MLN_CAMERA_OPTION_ANCHOR;
            native.anchor = ToNative(anchor);
        }
        if (options.Zoom is { } zoom)
        {
            native.fields |= (uint)mln_camera_option_field.MLN_CAMERA_OPTION_ZOOM;
            native.zoom = zoom;
        }
        if (options.Bearing is { } bearing)
        {
            native.fields |= (uint)mln_camera_option_field.MLN_CAMERA_OPTION_BEARING;
            native.bearing = bearing;
        }
        if (options.Pitch is { } pitch)
        {
            native.fields |= (uint)mln_camera_option_field.MLN_CAMERA_OPTION_PITCH;
            native.pitch = pitch;
        }
        if (options.Roll is { } roll)
        {
            native.fields |= (uint)mln_camera_option_field.MLN_CAMERA_OPTION_ROLL;
            native.roll = roll;
        }
        if (options.FieldOfView is { } fieldOfView)
        {
            native.fields |= (uint)mln_camera_option_field.MLN_CAMERA_OPTION_FOV;
            native.field_of_view = fieldOfView;
        }
        return native;
    }

    internal static mln_camera_fit_options ToNative(CameraFitOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_camera_fit_options_default();
        if (options.Padding is { } padding)
        {
            native.fields |= (uint)mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_PADDING;
            native.padding = ToNative(padding);
        }
        if (options.Bearing is { } bearing)
        {
            native.fields |= (uint)mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_BEARING;
            native.bearing = bearing;
        }
        if (options.Pitch is { } pitch)
        {
            native.fields |= (uint)mln_camera_fit_option_field.MLN_CAMERA_FIT_OPTION_PITCH;
            native.pitch = pitch;
        }
        return native;
    }

    internal static mln_animation_options ToNative(AnimationOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_animation_options_default();
        if (options.Duration is { } duration)
        {
            native.fields |= (uint)mln_animation_option_field.MLN_ANIMATION_OPTION_DURATION;
            native.duration_ms = duration;
        }
        if (options.Velocity is { } velocity)
        {
            native.fields |= (uint)mln_animation_option_field.MLN_ANIMATION_OPTION_VELOCITY;
            native.velocity = velocity;
        }
        if (options.MinimumZoom is { } minimumZoom)
        {
            native.fields |= (uint)mln_animation_option_field.MLN_ANIMATION_OPTION_MIN_ZOOM;
            native.min_zoom = minimumZoom;
        }
        if (options.Easing is { } easing)
        {
            native.fields |= (uint)mln_animation_option_field.MLN_ANIMATION_OPTION_EASING;
            native.easing = new mln_unit_bezier
            {
                x1 = easing.X1,
                y1 = easing.Y1,
                x2 = easing.X2,
                y2 = easing.Y2,
            };
        }
        return native;
    }

    internal static CameraOptions CameraOptionsFromNative(mln_camera_options native)
    {
        var options = new CameraOptions();
        if (Has(native.fields, mln_camera_option_field.MLN_CAMERA_OPTION_CENTER))
        {
            options.Center = new LatLng(native.latitude, native.longitude);
        }
        if (Has(native.fields, mln_camera_option_field.MLN_CAMERA_OPTION_CENTER_ALTITUDE))
        {
            options.CenterAltitude = native.center_altitude;
        }
        if (Has(native.fields, mln_camera_option_field.MLN_CAMERA_OPTION_PADDING))
        {
            options.Padding = FromNative(native.padding);
        }
        if (Has(native.fields, mln_camera_option_field.MLN_CAMERA_OPTION_ANCHOR))
        {
            options.Anchor = FromNative(native.anchor);
        }
        if (Has(native.fields, mln_camera_option_field.MLN_CAMERA_OPTION_ZOOM))
        {
            options.Zoom = native.zoom;
        }
        if (Has(native.fields, mln_camera_option_field.MLN_CAMERA_OPTION_BEARING))
        {
            options.Bearing = native.bearing;
        }
        if (Has(native.fields, mln_camera_option_field.MLN_CAMERA_OPTION_PITCH))
        {
            options.Pitch = native.pitch;
        }
        if (Has(native.fields, mln_camera_option_field.MLN_CAMERA_OPTION_ROLL))
        {
            options.Roll = native.roll;
        }
        if (Has(native.fields, mln_camera_option_field.MLN_CAMERA_OPTION_FOV))
        {
            options.FieldOfView = native.field_of_view;
        }
        return options;
    }

    internal static mln_map_viewport_options ToNative(ViewportOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_map_viewport_options_default();
        if (options.NorthOrientation is { } northOrientation)
        {
            native.fields |= (uint)
                mln_map_viewport_option_field.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
            native.north_orientation = (uint)northOrientation;
        }
        if (options.ConstrainMode is { } constrainMode)
        {
            native.fields |= (uint)
                mln_map_viewport_option_field.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
            native.constrain_mode = (uint)constrainMode;
        }
        if (options.ViewportMode is { } viewportMode)
        {
            native.fields |= (uint)
                mln_map_viewport_option_field.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
            native.viewport_mode = (uint)viewportMode;
        }
        if (options.FrustumOffset is { } frustumOffset)
        {
            native.fields |= (uint)
                mln_map_viewport_option_field.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
            native.frustum_offset = ToNative(frustumOffset);
        }
        return native;
    }

    internal static mln_bound_options ToNative(BoundOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_bound_options_default();
        if (options.Bounds is { } bounds)
        {
            native.fields |= (uint)mln_bound_option_field.MLN_BOUND_OPTION_BOUNDS;
            native.bounds = ToNative(bounds);
        }
        if (options.MinimumZoom is { } minimumZoom)
        {
            native.fields |= (uint)mln_bound_option_field.MLN_BOUND_OPTION_MIN_ZOOM;
            native.min_zoom = minimumZoom;
        }
        if (options.MaximumZoom is { } maximumZoom)
        {
            native.fields |= (uint)mln_bound_option_field.MLN_BOUND_OPTION_MAX_ZOOM;
            native.max_zoom = maximumZoom;
        }
        if (options.MinimumPitch is { } minimumPitch)
        {
            native.fields |= (uint)mln_bound_option_field.MLN_BOUND_OPTION_MIN_PITCH;
            native.min_pitch = minimumPitch;
        }
        if (options.MaximumPitch is { } maximumPitch)
        {
            native.fields |= (uint)mln_bound_option_field.MLN_BOUND_OPTION_MAX_PITCH;
            native.max_pitch = maximumPitch;
        }
        return native;
    }

    internal static BoundOptions BoundOptionsFromNative(mln_bound_options native)
    {
        var options = new BoundOptions();
        if (Has(native.fields, mln_bound_option_field.MLN_BOUND_OPTION_BOUNDS))
        {
            options.Bounds = FromNative(native.bounds);
        }
        if (Has(native.fields, mln_bound_option_field.MLN_BOUND_OPTION_MIN_ZOOM))
        {
            options.MinimumZoom = native.min_zoom;
        }
        if (Has(native.fields, mln_bound_option_field.MLN_BOUND_OPTION_MAX_ZOOM))
        {
            options.MaximumZoom = native.max_zoom;
        }
        if (Has(native.fields, mln_bound_option_field.MLN_BOUND_OPTION_MIN_PITCH))
        {
            options.MinimumPitch = native.min_pitch;
        }
        if (Has(native.fields, mln_bound_option_field.MLN_BOUND_OPTION_MAX_PITCH))
        {
            options.MaximumPitch = native.max_pitch;
        }
        return options;
    }

    internal static mln_free_camera_options ToNative(FreeCameraOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_free_camera_options_default();
        if (options.Position is { } position)
        {
            native.fields |= (uint)mln_free_camera_option_field.MLN_FREE_CAMERA_OPTION_POSITION;
            native.position = ToNative(position);
        }
        if (options.Orientation is { } orientation)
        {
            native.fields |= (uint)mln_free_camera_option_field.MLN_FREE_CAMERA_OPTION_ORIENTATION;
            native.orientation = ToNative(orientation);
        }
        return native;
    }

    internal static FreeCameraOptions FreeCameraOptionsFromNative(mln_free_camera_options native)
    {
        var options = new FreeCameraOptions();
        if (Has(native.fields, mln_free_camera_option_field.MLN_FREE_CAMERA_OPTION_POSITION))
        {
            options.Position = FromNative(native.position);
        }
        if (Has(native.fields, mln_free_camera_option_field.MLN_FREE_CAMERA_OPTION_ORIENTATION))
        {
            options.Orientation = FromNative(native.orientation);
        }
        return options;
    }

    internal static mln_projection_mode ToNative(ProjectionModeOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_projection_mode_default();
        if (options.Axonometric is { } axonometric)
        {
            native.fields |= (uint)mln_projection_mode_field.MLN_PROJECTION_MODE_AXONOMETRIC;
            native.axonometric = axonometric ? (byte)1 : (byte)0;
        }
        if (options.XSkew is { } xSkew)
        {
            native.fields |= (uint)mln_projection_mode_field.MLN_PROJECTION_MODE_X_SKEW;
            native.x_skew = xSkew;
        }
        if (options.YSkew is { } ySkew)
        {
            native.fields |= (uint)mln_projection_mode_field.MLN_PROJECTION_MODE_Y_SKEW;
            native.y_skew = ySkew;
        }
        return native;
    }

    internal static ProjectionModeOptions ProjectionModeOptionsFromNative(
        mln_projection_mode native
    )
    {
        var options = new ProjectionModeOptions();
        if (Has(native.fields, mln_projection_mode_field.MLN_PROJECTION_MODE_AXONOMETRIC))
        {
            options.Axonometric = native.axonometric != 0;
        }
        if (Has(native.fields, mln_projection_mode_field.MLN_PROJECTION_MODE_X_SKEW))
        {
            options.XSkew = native.x_skew;
        }
        if (Has(native.fields, mln_projection_mode_field.MLN_PROJECTION_MODE_Y_SKEW))
        {
            options.YSkew = native.y_skew;
        }
        return options;
    }

    internal static ViewportOptions ViewportOptionsFromNative(mln_map_viewport_options native)
    {
        var options = new ViewportOptions();
        if (
            Has(
                native.fields,
                mln_map_viewport_option_field.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
            )
        )
        {
            options.NorthOrientation = (NorthOrientation)native.north_orientation;
        }
        if (
            Has(native.fields, mln_map_viewport_option_field.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE)
        )
        {
            options.ConstrainMode = (ConstrainMode)native.constrain_mode;
        }
        if (Has(native.fields, mln_map_viewport_option_field.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE))
        {
            options.ViewportMode = (ViewportMode)native.viewport_mode;
        }
        if (
            Has(native.fields, mln_map_viewport_option_field.MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET)
        )
        {
            options.FrustumOffset = FromNative(native.frustum_offset);
        }
        return options;
    }

    internal static mln_map_tile_options ToNative(TileOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_map_tile_options_default();
        if (options.PrefetchZoomDelta is { } prefetchZoomDelta)
        {
            native.fields |= (uint)
                mln_map_tile_option_field.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
            native.prefetch_zoom_delta = prefetchZoomDelta;
        }
        if (options.LodMinimumRadius is { } lodMinimumRadius)
        {
            native.fields |= (uint)mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
            native.lod_min_radius = lodMinimumRadius;
        }
        if (options.LodScale is { } lodScale)
        {
            native.fields |= (uint)mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_SCALE;
            native.lod_scale = lodScale;
        }
        if (options.LodPitchThreshold is { } lodPitchThreshold)
        {
            native.fields |= (uint)
                mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
            native.lod_pitch_threshold = lodPitchThreshold;
        }
        if (options.LodZoomShift is { } lodZoomShift)
        {
            native.fields |= (uint)mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
            native.lod_zoom_shift = lodZoomShift;
        }
        if (options.LodMode is { } lodMode)
        {
            native.fields |= (uint)mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_MODE;
            native.lod_mode = (uint)lodMode;
        }
        return native;
    }

    internal static TileOptions TileOptionsFromNative(mln_map_tile_options native)
    {
        var options = new TileOptions();
        if (Has(native.fields, mln_map_tile_option_field.MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA))
        {
            options.PrefetchZoomDelta = native.prefetch_zoom_delta;
        }
        if (Has(native.fields, mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS))
        {
            options.LodMinimumRadius = native.lod_min_radius;
        }
        if (Has(native.fields, mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_SCALE))
        {
            options.LodScale = native.lod_scale;
        }
        if (Has(native.fields, mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD))
        {
            options.LodPitchThreshold = native.lod_pitch_threshold;
        }
        if (Has(native.fields, mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT))
        {
            options.LodZoomShift = native.lod_zoom_shift;
        }
        if (Has(native.fields, mln_map_tile_option_field.MLN_MAP_TILE_OPTION_LOD_MODE))
        {
            options.LodMode = (TileLodMode)native.lod_mode;
        }
        return options;
    }

    private static bool Has(uint fields, mln_camera_option_field field) =>
        (fields & (uint)field) != 0;

    private static bool Has(uint fields, mln_bound_option_field field) =>
        (fields & (uint)field) != 0;

    private static bool Has(uint fields, mln_free_camera_option_field field) =>
        (fields & (uint)field) != 0;

    private static bool Has(uint fields, mln_projection_mode_field field) =>
        (fields & (uint)field) != 0;

    private static bool Has(uint fields, mln_map_viewport_option_field field) =>
        (fields & (uint)field) != 0;

    private static bool Has(uint fields, mln_map_tile_option_field field) =>
        (fields & (uint)field) != 0;
}
