namespace MaplibreNative {
    public class AnimationOptions {
        private bool has_duration_ms;
        private double duration_ms_value;
        private bool has_velocity;
        private double velocity_value;
        private bool has_min_zoom;
        private double min_zoom_value;
        private bool has_easing;
        private UnitBezier easing_value;

        public AnimationOptions () {
        }

        public void set_duration_ms (double value) {
            duration_ms_value = value;
            has_duration_ms = true;
        }

        public bool get_duration_ms (out double value) {
            value = duration_ms_value;
            return has_duration_ms;
        }

        public void set_velocity (double value) {
            velocity_value = value;
            has_velocity = true;
        }

        public bool get_velocity (out double value) {
            value = velocity_value;
            return has_velocity;
        }

        public void set_min_zoom (double value) {
            min_zoom_value = value;
            has_min_zoom = true;
        }

        public bool get_min_zoom (out double value) {
            value = min_zoom_value;
            return has_min_zoom;
        }

        public void set_easing (UnitBezier value) {
            easing_value = value;
            has_easing = true;
        }

        public bool get_easing (out UnitBezier value) {
            value = easing_value;
            return has_easing;
        }

        internal Raw.AnimationOptions to_native () {
            Raw.AnimationOptions options = {};
            options.size = (uint32) sizeof (Raw.AnimationOptions);
            if (has_duration_ms) {
                options.duration_ms = duration_ms_value;
                options.fields |= 1U << 0;
            }
            if (has_velocity) {
                options.velocity = velocity_value;
                options.fields |= 1U << 1;
            }
            if (has_min_zoom) {
                options.min_zoom = min_zoom_value;
                options.fields |= 1U << 2;
            }
            if (has_easing) {
                options.easing = easing_value.to_native ();
                options.fields |= 1U << 3;
            }
            return options;
        }
    }

    public class CameraFitOptions {
        private bool has_padding;
        private EdgeInsets padding_value;
        private bool has_bearing;
        private double bearing_value;
        private bool has_pitch;
        private double pitch_value;

        public CameraFitOptions () {
        }

        public void set_padding (EdgeInsets value) {
            padding_value = value;
            has_padding = true;
        }

        public bool get_padding (out EdgeInsets value) {
            value = padding_value;
            return has_padding;
        }

        public void set_bearing (double value) {
            bearing_value = value;
            has_bearing = true;
        }

        public bool get_bearing (out double value) {
            value = bearing_value;
            return has_bearing;
        }

        public void set_pitch (double value) {
            pitch_value = value;
            has_pitch = true;
        }

        public bool get_pitch (out double value) {
            value = pitch_value;
            return has_pitch;
        }

        internal Raw.CameraFitOptions to_native () {
            Raw.CameraFitOptions options = {};
            options.size = (uint32) sizeof (Raw.CameraFitOptions);
            if (has_padding) {
                options.padding = padding_value.to_native ();
                options.fields |= 1U << 0;
            }
            if (has_bearing) {
                options.bearing = bearing_value;
                options.fields |= 1U << 1;
            }
            if (has_pitch) {
                options.pitch = pitch_value;
                options.fields |= 1U << 2;
            }
            return options;
        }
    }

    public class BoundOptions {
        private bool has_bounds;
        private LatLngBounds bounds_value;
        private bool has_min_zoom;
        private double min_zoom_value;
        private bool has_max_zoom;
        private double max_zoom_value;
        private bool has_min_pitch;
        private double min_pitch_value;
        private bool has_max_pitch;
        private double max_pitch_value;

        public BoundOptions () {
        }

        internal BoundOptions.from_native (Raw.BoundOptions native) {
            if ((native.fields & (1U << 0)) != 0) {
                set_bounds (LatLngBounds.from_native (native.bounds));
            }
            if ((native.fields & (1U << 1)) != 0) {
                set_min_zoom (native.min_zoom);
            }
            if ((native.fields & (1U << 2)) != 0) {
                set_max_zoom (native.max_zoom);
            }
            if ((native.fields & (1U << 3)) != 0) {
                set_min_pitch (native.min_pitch);
            }
            if ((native.fields & (1U << 4)) != 0) {
                set_max_pitch (native.max_pitch);
            }
        }

        public void set_bounds (LatLngBounds value) {
            bounds_value = value;
            has_bounds = true;
        }

        public bool get_bounds (out LatLngBounds value) {
            value = bounds_value;
            return has_bounds;
        }

        public void set_min_zoom (double value) {
            min_zoom_value = value;
            has_min_zoom = true;
        }

        public bool get_min_zoom (out double value) {
            value = min_zoom_value;
            return has_min_zoom;
        }

        public void set_max_zoom (double value) {
            max_zoom_value = value;
            has_max_zoom = true;
        }

        public bool get_max_zoom (out double value) {
            value = max_zoom_value;
            return has_max_zoom;
        }

        public void set_min_pitch (double value) {
            min_pitch_value = value;
            has_min_pitch = true;
        }

        public bool get_min_pitch (out double value) {
            value = min_pitch_value;
            return has_min_pitch;
        }

        public void set_max_pitch (double value) {
            max_pitch_value = value;
            has_max_pitch = true;
        }

        public bool get_max_pitch (out double value) {
            value = max_pitch_value;
            return has_max_pitch;
        }

        internal Raw.BoundOptions to_native () {
            Raw.BoundOptions options = {};
            options.size = (uint32) sizeof (Raw.BoundOptions);
            if (has_bounds) {
                options.bounds = bounds_value.to_native ();
                options.fields |= 1U << 0;
            }
            if (has_min_zoom) {
                options.min_zoom = min_zoom_value;
                options.fields |= 1U << 1;
            }
            if (has_max_zoom) {
                options.max_zoom = max_zoom_value;
                options.fields |= 1U << 2;
            }
            if (has_min_pitch) {
                options.min_pitch = min_pitch_value;
                options.fields |= 1U << 3;
            }
            if (has_max_pitch) {
                options.max_pitch = max_pitch_value;
                options.fields |= 1U << 4;
            }
            return options;
        }
    }

    public class FreeCameraOptions {
        private bool has_position;
        private Vec3 position_value;
        private bool has_orientation;
        private Quaternion orientation_value;

        public FreeCameraOptions () {
        }

        internal FreeCameraOptions.from_native (Raw.FreeCameraOptions native) {
            if ((native.fields & (1U << 0)) != 0) {
                set_position (Vec3.from_native (native.position));
            }
            if ((native.fields & (1U << 1)) != 0) {
                set_orientation (Quaternion.from_native (native.orientation));
            }
        }

        public void set_position (Vec3 value) {
            position_value = value;
            has_position = true;
        }

        public bool get_position (out Vec3 value) {
            value = position_value;
            return has_position;
        }

        public void set_orientation (Quaternion value) {
            orientation_value = value;
            has_orientation = true;
        }

        public bool get_orientation (out Quaternion value) {
            value = orientation_value;
            return has_orientation;
        }

        internal Raw.FreeCameraOptions to_native () {
            Raw.FreeCameraOptions options = {};
            options.size = (uint32) sizeof (Raw.FreeCameraOptions);
            if (has_position) {
                options.position = position_value.to_native ();
                options.fields |= 1U << 0;
            }
            if (has_orientation) {
                options.orientation = orientation_value.to_native ();
                options.fields |= 1U << 1;
            }
            return options;
        }
    }

    public class ProjectionMode {
        private bool has_axonometric;
        private bool axonometric_value;
        private bool has_x_skew;
        private double x_skew_value;
        private bool has_y_skew;
        private double y_skew_value;

        public ProjectionMode () {
        }

        internal ProjectionMode.from_native (Raw.ProjectionMode native) {
            if ((native.fields & (1U << 0)) != 0) {
                set_axonometric (native.axonometric);
            }
            if ((native.fields & (1U << 1)) != 0) {
                set_x_skew (native.x_skew);
            }
            if ((native.fields & (1U << 2)) != 0) {
                set_y_skew (native.y_skew);
            }
        }

        public void set_axonometric (bool value) {
            axonometric_value = value;
            has_axonometric = true;
        }

        public bool get_axonometric (out bool value) {
            value = axonometric_value;
            return has_axonometric;
        }

        public void set_x_skew (double value) {
            x_skew_value = value;
            has_x_skew = true;
        }

        public bool get_x_skew (out double value) {
            value = x_skew_value;
            return has_x_skew;
        }

        public void set_y_skew (double value) {
            y_skew_value = value;
            has_y_skew = true;
        }

        public bool get_y_skew (out double value) {
            value = y_skew_value;
            return has_y_skew;
        }

        internal Raw.ProjectionMode to_native () {
            Raw.ProjectionMode mode = {};
            mode.size = (uint32) sizeof (Raw.ProjectionMode);
            if (has_axonometric) {
                mode.axonometric = axonometric_value;
                mode.fields |= 1U << 0;
            }
            if (has_x_skew) {
                mode.x_skew = x_skew_value;
                mode.fields |= 1U << 1;
            }
            if (has_y_skew) {
                mode.y_skew = y_skew_value;
                mode.fields |= 1U << 2;
            }
            return mode;
        }
    }

    public class CameraOptions {
        private bool has_center;
        private LatLng center_value;
        private bool has_center_altitude;
        private double center_altitude_value;
        private bool has_padding;
        private EdgeInsets padding_value;
        private bool has_anchor;
        private ScreenPoint anchor_value;
        private bool has_zoom;
        private double zoom_value;
        private bool has_bearing;
        private double bearing_value;
        private bool has_pitch;
        private double pitch_value;
        private bool has_roll;
        private double roll_value;
        private bool has_field_of_view;
        private double field_of_view_value;

        public CameraOptions () {
        }

        public void set_center (LatLng center) {
            center_value = center;
            has_center = true;
        }

        public void set_center_altitude (double center_altitude) {
            center_altitude_value = center_altitude;
            has_center_altitude = true;
        }

        public void set_padding (EdgeInsets padding) {
            padding_value = padding;
            has_padding = true;
        }

        public void set_anchor (ScreenPoint anchor) {
            anchor_value = anchor;
            has_anchor = true;
        }

        public void set_zoom (double zoom) {
            zoom_value = zoom;
            has_zoom = true;
        }

        public void set_bearing (double bearing) {
            bearing_value = bearing;
            has_bearing = true;
        }

        public void set_pitch (double pitch) {
            pitch_value = pitch;
            has_pitch = true;
        }

        public void set_roll (double roll) {
            roll_value = roll;
            has_roll = true;
        }

        public void set_field_of_view (double field_of_view) {
            field_of_view_value = field_of_view;
            has_field_of_view = true;
        }

        public bool get_center (out LatLng center) {
            center = center_value;
            return has_center;
        }

        public bool get_center_altitude (out double center_altitude) {
            center_altitude = center_altitude_value;
            return has_center_altitude;
        }

        public bool get_padding (out EdgeInsets padding) {
            padding = padding_value;
            return has_padding;
        }

        public bool get_anchor (out ScreenPoint anchor) {
            anchor = anchor_value;
            return has_anchor;
        }

        public bool get_zoom (out double zoom) {
            zoom = zoom_value;
            return has_zoom;
        }

        public bool get_bearing (out double bearing) {
            bearing = bearing_value;
            return has_bearing;
        }

        public bool get_pitch (out double pitch) {
            pitch = pitch_value;
            return has_pitch;
        }

        public bool get_roll (out double roll) {
            roll = roll_value;
            return has_roll;
        }

        public bool get_field_of_view (out double field_of_view) {
            field_of_view = field_of_view_value;
            return has_field_of_view;
        }

        internal Raw.CameraOptions to_native () {
            Raw.CameraOptions options = Raw.camera_options_default ();
            if (has_center) {
                options.latitude = center_value.latitude;
                options.longitude = center_value.longitude;
                options.fields |= (uint32) Raw.CameraOptionField.CENTER;
            }
            if (has_zoom) {
                options.zoom = zoom_value;
                options.fields |= (uint32) Raw.CameraOptionField.ZOOM;
            }
            if (has_bearing) {
                options.bearing = bearing_value;
                options.fields |= (uint32) Raw.CameraOptionField.BEARING;
            }
            if (has_pitch) {
                options.pitch = pitch_value;
                options.fields |= (uint32) Raw.CameraOptionField.PITCH;
            }
            if (has_center_altitude) {
                options.center_altitude = center_altitude_value;
                options.fields |= (uint32) Raw.CameraOptionField.CENTER_ALTITUDE;
            }
            if (has_padding) {
                options.padding = padding_value.to_native ();
                options.fields |= (uint32) Raw.CameraOptionField.PADDING;
            }
            if (has_anchor) {
                options.anchor = anchor_value.to_native ();
                options.fields |= (uint32) Raw.CameraOptionField.ANCHOR;
            }
            if (has_roll) {
                options.roll = roll_value;
                options.fields |= (uint32) Raw.CameraOptionField.ROLL;
            }
            if (has_field_of_view) {
                options.field_of_view = field_of_view_value;
                options.fields |= (uint32) Raw.CameraOptionField.FOV;
            }
            return options;
        }

        internal static CameraOptions from_native (Raw.CameraOptions native) {
            var camera = new CameraOptions ();
            if ((native.fields & (uint32) Raw.CameraOptionField.CENTER) != 0) {
                camera.set_center (LatLng (native.latitude, native.longitude));
            }
            if ((native.fields & (uint32) Raw.CameraOptionField.ZOOM) != 0) {
                camera.set_zoom (native.zoom);
            }
            if ((native.fields & (uint32) Raw.CameraOptionField.BEARING) != 0) {
                camera.set_bearing (native.bearing);
            }
            if ((native.fields & (uint32) Raw.CameraOptionField.PITCH) != 0) {
                camera.set_pitch (native.pitch);
            }
            if ((native.fields & (uint32) Raw.CameraOptionField.CENTER_ALTITUDE) != 0) {
                camera.set_center_altitude (native.center_altitude);
            }
            if ((native.fields & (uint32) Raw.CameraOptionField.PADDING) != 0) {
                camera.set_padding (EdgeInsets.from_native (native.padding));
            }
            if ((native.fields & (uint32) Raw.CameraOptionField.ANCHOR) != 0) {
                camera.set_anchor (ScreenPoint.from_native (native.anchor));
            }
            if ((native.fields & (uint32) Raw.CameraOptionField.ROLL) != 0) {
                camera.set_roll (native.roll);
            }
            if ((native.fields & (uint32) Raw.CameraOptionField.FOV) != 0) {
                camera.set_field_of_view (native.field_of_view);
            }
            return camera;
        }
    }
}
