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
        private bool has_transition_id;
        private uint64 transition_id_value;

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

        public void set_transition_id (uint64 value) {
            transition_id_value = value;
            has_transition_id = true;
        }

        public bool get_transition_id (out uint64 value) {
            value = transition_id_value;
            return has_transition_id;
        }

        public AnimationOptions copy () {
            var copied = new AnimationOptions ();
            copied.has_duration_ms = has_duration_ms;
            copied.duration_ms_value = duration_ms_value;
            copied.has_velocity = has_velocity;
            copied.velocity_value = velocity_value;
            copied.has_min_zoom = has_min_zoom;
            copied.min_zoom_value = min_zoom_value;
            copied.has_easing = has_easing;
            copied.easing_value = easing_value;
            copied.has_transition_id = has_transition_id;
            copied.transition_id_value = transition_id_value;
            return copied;
        }

        public bool equal (AnimationOptions other) {
            return has_duration_ms == other.has_duration_ms
                && (!has_duration_ms || duration_ms_value == other.duration_ms_value)
                && has_velocity == other.has_velocity
                && (!has_velocity || velocity_value == other.velocity_value)
                && has_min_zoom == other.has_min_zoom
                && (!has_min_zoom || min_zoom_value == other.min_zoom_value)
                && has_easing == other.has_easing
                && (!has_easing
                    || (easing_value.x1 == other.easing_value.x1
                        && easing_value.y1 == other.easing_value.y1
                        && easing_value.x2 == other.easing_value.x2
                        && easing_value.y2 == other.easing_value.y2))
                && has_transition_id == other.has_transition_id
                && (!has_transition_id || transition_id_value == other.transition_id_value);
        }

        internal Raw.AnimationOptions to_native () {
            Raw.AnimationOptions options = Raw.animation_options_default ();
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
            if (has_transition_id) {
                options.transition_id = transition_id_value;
                options.fields |= 1U << 4;
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

        public CameraFitOptions copy () {
            var copied = new CameraFitOptions ();
            copied.has_padding = has_padding;
            copied.padding_value = padding_value;
            copied.has_bearing = has_bearing;
            copied.bearing_value = bearing_value;
            copied.has_pitch = has_pitch;
            copied.pitch_value = pitch_value;
            return copied;
        }

        public bool equal (CameraFitOptions other) {
            return has_padding == other.has_padding
                && (!has_padding
                    || (padding_value.top == other.padding_value.top
                        && padding_value.left == other.padding_value.left
                        && padding_value.bottom == other.padding_value.bottom
                        && padding_value.right == other.padding_value.right))
                && has_bearing == other.has_bearing
                && (!has_bearing || bearing_value == other.bearing_value)
                && has_pitch == other.has_pitch
                && (!has_pitch || pitch_value == other.pitch_value);
        }

        internal Raw.CameraFitOptions to_native () {
            Raw.CameraFitOptions options = Raw.camera_fit_options_default ();
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
        private bool unbounded;
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
            } else if ((native.fields & (1U << 5)) != 0) {
                set_unbounded ();
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
            unbounded = false;
        }

        public void set_unbounded () {
            has_bounds = false;
            unbounded = true;
        }

        public bool is_unbounded () {
            return unbounded;
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

        public BoundOptions copy () {
            var copied = new BoundOptions ();
            copied.has_bounds = has_bounds;
            copied.bounds_value = bounds_value;
            copied.unbounded = unbounded;
            copied.has_min_zoom = has_min_zoom;
            copied.min_zoom_value = min_zoom_value;
            copied.has_max_zoom = has_max_zoom;
            copied.max_zoom_value = max_zoom_value;
            copied.has_min_pitch = has_min_pitch;
            copied.min_pitch_value = min_pitch_value;
            copied.has_max_pitch = has_max_pitch;
            copied.max_pitch_value = max_pitch_value;
            return copied;
        }

        public bool equal (BoundOptions other) {
            return has_bounds == other.has_bounds
                && unbounded == other.unbounded
                && (!has_bounds
                    || (bounds_value.southwest.latitude == other.bounds_value.southwest.latitude
                        && bounds_value.southwest.longitude == other.bounds_value.southwest.longitude
                        && bounds_value.northeast.latitude == other.bounds_value.northeast.latitude
                        && bounds_value.northeast.longitude == other.bounds_value.northeast.longitude))
                && has_min_zoom == other.has_min_zoom
                && (!has_min_zoom || min_zoom_value == other.min_zoom_value)
                && has_max_zoom == other.has_max_zoom
                && (!has_max_zoom || max_zoom_value == other.max_zoom_value)
                && has_min_pitch == other.has_min_pitch
                && (!has_min_pitch || min_pitch_value == other.min_pitch_value)
                && has_max_pitch == other.has_max_pitch
                && (!has_max_pitch || max_pitch_value == other.max_pitch_value);
        }

        internal Raw.BoundOptions to_native () {
            Raw.BoundOptions options = Raw.bound_options_default ();
            if (has_bounds) {
                options.bounds = bounds_value.to_native ();
                options.fields |= 1U << 0;
            } else if (unbounded) {
                options.fields |= 1U << 5;
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

        public FreeCameraOptions copy () {
            var copied = new FreeCameraOptions ();
            copied.has_position = has_position;
            copied.position_value = position_value;
            copied.has_orientation = has_orientation;
            copied.orientation_value = orientation_value;
            return copied;
        }

        public bool equal (FreeCameraOptions other) {
            return has_position == other.has_position
                && (!has_position
                    || (position_value.x == other.position_value.x
                        && position_value.y == other.position_value.y
                        && position_value.z == other.position_value.z))
                && has_orientation == other.has_orientation
                && (!has_orientation
                    || (orientation_value.x == other.orientation_value.x
                        && orientation_value.y == other.orientation_value.y
                        && orientation_value.z == other.orientation_value.z
                        && orientation_value.w == other.orientation_value.w));
        }

        internal Raw.FreeCameraOptions to_native () {
            Raw.FreeCameraOptions options = Raw.free_camera_options_default ();
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

        public ProjectionMode copy () {
            var copied = new ProjectionMode ();
            copied.has_axonometric = has_axonometric;
            copied.axonometric_value = axonometric_value;
            copied.has_x_skew = has_x_skew;
            copied.x_skew_value = x_skew_value;
            copied.has_y_skew = has_y_skew;
            copied.y_skew_value = y_skew_value;
            return copied;
        }

        public bool equal (ProjectionMode other) {
            return has_axonometric == other.has_axonometric
                && (!has_axonometric || axonometric_value == other.axonometric_value)
                && has_x_skew == other.has_x_skew
                && (!has_x_skew || x_skew_value == other.x_skew_value)
                && has_y_skew == other.has_y_skew
                && (!has_y_skew || y_skew_value == other.y_skew_value);
        }

        internal Raw.ProjectionMode to_native () {
            Raw.ProjectionMode mode = Raw.projection_mode_default ();
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

        public CameraOptions copy () {
            var copied = new CameraOptions ();
            copied.has_center = has_center;
            copied.center_value = center_value;
            copied.has_center_altitude = has_center_altitude;
            copied.center_altitude_value = center_altitude_value;
            copied.has_padding = has_padding;
            copied.padding_value = padding_value;
            copied.has_anchor = has_anchor;
            copied.anchor_value = anchor_value;
            copied.has_zoom = has_zoom;
            copied.zoom_value = zoom_value;
            copied.has_bearing = has_bearing;
            copied.bearing_value = bearing_value;
            copied.has_pitch = has_pitch;
            copied.pitch_value = pitch_value;
            copied.has_roll = has_roll;
            copied.roll_value = roll_value;
            copied.has_field_of_view = has_field_of_view;
            copied.field_of_view_value = field_of_view_value;
            return copied;
        }

        public bool equal (CameraOptions other) {
            return has_center == other.has_center
                && (!has_center || (center_value.latitude == other.center_value.latitude && center_value.longitude == other.center_value.longitude))
                && has_center_altitude == other.has_center_altitude
                && (!has_center_altitude || center_altitude_value == other.center_altitude_value)
                && has_padding == other.has_padding
                && (!has_padding || (padding_value.top == other.padding_value.top && padding_value.left == other.padding_value.left && padding_value.bottom == other.padding_value.bottom && padding_value.right == other.padding_value.right))
                && has_anchor == other.has_anchor
                && (!has_anchor || (anchor_value.x == other.anchor_value.x && anchor_value.y == other.anchor_value.y))
                && has_zoom == other.has_zoom
                && (!has_zoom || zoom_value == other.zoom_value)
                && has_bearing == other.has_bearing
                && (!has_bearing || bearing_value == other.bearing_value)
                && has_pitch == other.has_pitch
                && (!has_pitch || pitch_value == other.pitch_value)
                && has_roll == other.has_roll
                && (!has_roll || roll_value == other.roll_value)
                && has_field_of_view == other.has_field_of_view
                && (!has_field_of_view || field_of_view_value == other.field_of_view_value);
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
