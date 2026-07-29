namespace MaplibreNative {
    public class MapOptions {
        public uint32 width { get; set; default = 256; }
        public uint32 height { get; set; default = 256; }
        public double scale_factor { get; set; default = 1.0; }
        public MapMode mode { get; set; default = MapMode.CONTINUOUS; }
        public bool fast_pfor_enabled { get; set; default = false; }

        public MapOptions copy () {
            var copied = new MapOptions ();
            copied.width = width;
            copied.height = height;
            copied.scale_factor = scale_factor;
            copied.mode = mode;
            copied.fast_pfor_enabled = fast_pfor_enabled;
            return copied;
        }

        public bool equal (MapOptions other) {
            return width == other.width
                && height == other.height
                && scale_factor == other.scale_factor
                && mode == other.mode
                && fast_pfor_enabled == other.fast_pfor_enabled;
        }

        internal Raw.MapOptions to_native () {
            Raw.MapOptions options = Raw.map_options_default ();
            options.width = width;
            options.height = height;
            options.scale_factor = scale_factor;
            options.map_mode = (uint32) mode;
            options.fast_pfor_enabled = fast_pfor_enabled;
            return options;
        }
    }

    public struct MapSize {
        public uint32 width;
        public uint32 height;
        public double scale_factor;

        public MapSize (uint32 width, uint32 height, double scale_factor) {
            this.width = width;
            this.height = height;
            this.scale_factor = scale_factor;
        }
    }

    public class MapViewportOptions {
        private bool has_north_orientation;
        private NorthOrientation north_orientation_value;
        private bool has_constrain_mode;
        private ConstrainMode constrain_mode_value;
        private bool has_viewport_mode;
        private ViewportMode viewport_mode_value;
        private bool has_frustum_offset;
        private EdgeInsets frustum_offset_value;

        public MapViewportOptions () {
        }

        internal MapViewportOptions.from_native (Raw.MapViewportOptions native) {
            if ((native.fields & (1U << 0)) != 0) {
                set_north_orientation ((NorthOrientation) native.north_orientation);
            }
            if ((native.fields & (1U << 1)) != 0) {
                set_constrain_mode ((ConstrainMode) native.constrain_mode);
            }
            if ((native.fields & (1U << 2)) != 0) {
                set_viewport_mode ((ViewportMode) native.viewport_mode);
            }
            if ((native.fields & (1U << 3)) != 0) {
                set_frustum_offset (EdgeInsets.from_native (native.frustum_offset));
            }
        }

        public void set_north_orientation (NorthOrientation value) {
            north_orientation_value = value;
            has_north_orientation = true;
        }

        public bool get_north_orientation (out NorthOrientation value) {
            value = north_orientation_value;
            return has_north_orientation;
        }

        public void set_constrain_mode (ConstrainMode value) {
            constrain_mode_value = value;
            has_constrain_mode = true;
        }

        public bool get_constrain_mode (out ConstrainMode value) {
            value = constrain_mode_value;
            return has_constrain_mode;
        }

        public void set_viewport_mode (ViewportMode value) {
            viewport_mode_value = value;
            has_viewport_mode = true;
        }

        public bool get_viewport_mode (out ViewportMode value) {
            value = viewport_mode_value;
            return has_viewport_mode;
        }

        public void set_frustum_offset (EdgeInsets value) {
            frustum_offset_value = value;
            has_frustum_offset = true;
        }

        public bool get_frustum_offset (out EdgeInsets value) {
            value = frustum_offset_value;
            return has_frustum_offset;
        }

        public MapViewportOptions copy () {
            var copied = new MapViewportOptions ();
            copied.has_north_orientation = has_north_orientation;
            copied.north_orientation_value = north_orientation_value;
            copied.has_constrain_mode = has_constrain_mode;
            copied.constrain_mode_value = constrain_mode_value;
            copied.has_viewport_mode = has_viewport_mode;
            copied.viewport_mode_value = viewport_mode_value;
            copied.has_frustum_offset = has_frustum_offset;
            copied.frustum_offset_value = frustum_offset_value;
            return copied;
        }

        public bool equal (MapViewportOptions other) {
            return has_north_orientation == other.has_north_orientation
                && (!has_north_orientation || north_orientation_value == other.north_orientation_value)
                && has_constrain_mode == other.has_constrain_mode
                && (!has_constrain_mode || constrain_mode_value == other.constrain_mode_value)
                && has_viewport_mode == other.has_viewport_mode
                && (!has_viewport_mode || viewport_mode_value == other.viewport_mode_value)
                && has_frustum_offset == other.has_frustum_offset
                && (!has_frustum_offset
                    || (frustum_offset_value.top == other.frustum_offset_value.top
                        && frustum_offset_value.left == other.frustum_offset_value.left
                        && frustum_offset_value.bottom == other.frustum_offset_value.bottom
                        && frustum_offset_value.right == other.frustum_offset_value.right));
        }

        internal Raw.MapViewportOptions to_native () {
            Raw.MapViewportOptions options = Raw.map_viewport_options_default ();
            if (has_north_orientation) {
                options.north_orientation = (uint32) north_orientation_value;
                options.fields |= 1U << 0;
            }
            if (has_constrain_mode) {
                options.constrain_mode = (uint32) constrain_mode_value;
                options.fields |= 1U << 1;
            }
            if (has_viewport_mode) {
                options.viewport_mode = (uint32) viewport_mode_value;
                options.fields |= 1U << 2;
            }
            if (has_frustum_offset) {
                options.frustum_offset = frustum_offset_value.to_native ();
                options.fields |= 1U << 3;
            }
            return options;
        }
    }

    public class MapTileOptions {
        private bool has_prefetch_zoom_delta;
        private uint32 prefetch_zoom_delta_value;
        private bool has_lod_min_radius;
        private double lod_min_radius_value;
        private bool has_lod_scale;
        private double lod_scale_value;
        private bool has_lod_pitch_threshold;
        private double lod_pitch_threshold_value;
        private bool has_lod_zoom_shift;
        private double lod_zoom_shift_value;
        private bool has_lod_mode;
        private TileLodMode lod_mode_value;

        public MapTileOptions () {
        }

        internal MapTileOptions.from_native (Raw.MapTileOptions native) {
            if ((native.fields & (1U << 0)) != 0) {
                set_prefetch_zoom_delta (native.prefetch_zoom_delta);
            }
            if ((native.fields & (1U << 1)) != 0) {
                set_lod_min_radius (native.lod_min_radius);
            }
            if ((native.fields & (1U << 2)) != 0) {
                set_lod_scale (native.lod_scale);
            }
            if ((native.fields & (1U << 3)) != 0) {
                set_lod_pitch_threshold (native.lod_pitch_threshold);
            }
            if ((native.fields & (1U << 4)) != 0) {
                set_lod_zoom_shift (native.lod_zoom_shift);
            }
            if ((native.fields & (1U << 5)) != 0) {
                set_lod_mode ((TileLodMode) native.lod_mode);
            }
        }

        public void set_prefetch_zoom_delta (uint32 value) {
            prefetch_zoom_delta_value = value;
            has_prefetch_zoom_delta = true;
        }

        public bool get_prefetch_zoom_delta (out uint32 value) {
            value = prefetch_zoom_delta_value;
            return has_prefetch_zoom_delta;
        }

        public void set_lod_min_radius (double value) {
            lod_min_radius_value = value;
            has_lod_min_radius = true;
        }

        public bool get_lod_min_radius (out double value) {
            value = lod_min_radius_value;
            return has_lod_min_radius;
        }

        public void set_lod_scale (double value) {
            lod_scale_value = value;
            has_lod_scale = true;
        }

        public bool get_lod_scale (out double value) {
            value = lod_scale_value;
            return has_lod_scale;
        }

        public void set_lod_pitch_threshold (double value) {
            lod_pitch_threshold_value = value;
            has_lod_pitch_threshold = true;
        }

        public bool get_lod_pitch_threshold (out double value) {
            value = lod_pitch_threshold_value;
            return has_lod_pitch_threshold;
        }

        public void set_lod_zoom_shift (double value) {
            lod_zoom_shift_value = value;
            has_lod_zoom_shift = true;
        }

        public bool get_lod_zoom_shift (out double value) {
            value = lod_zoom_shift_value;
            return has_lod_zoom_shift;
        }

        public void set_lod_mode (TileLodMode value) {
            lod_mode_value = value;
            has_lod_mode = true;
        }

        public bool get_lod_mode (out TileLodMode value) {
            value = lod_mode_value;
            return has_lod_mode;
        }

        public MapTileOptions copy () {
            var copied = new MapTileOptions ();
            copied.has_prefetch_zoom_delta = has_prefetch_zoom_delta;
            copied.prefetch_zoom_delta_value = prefetch_zoom_delta_value;
            copied.has_lod_min_radius = has_lod_min_radius;
            copied.lod_min_radius_value = lod_min_radius_value;
            copied.has_lod_scale = has_lod_scale;
            copied.lod_scale_value = lod_scale_value;
            copied.has_lod_pitch_threshold = has_lod_pitch_threshold;
            copied.lod_pitch_threshold_value = lod_pitch_threshold_value;
            copied.has_lod_zoom_shift = has_lod_zoom_shift;
            copied.lod_zoom_shift_value = lod_zoom_shift_value;
            copied.has_lod_mode = has_lod_mode;
            copied.lod_mode_value = lod_mode_value;
            return copied;
        }

        public bool equal (MapTileOptions other) {
            return has_prefetch_zoom_delta == other.has_prefetch_zoom_delta
                && (!has_prefetch_zoom_delta || prefetch_zoom_delta_value == other.prefetch_zoom_delta_value)
                && has_lod_min_radius == other.has_lod_min_radius
                && (!has_lod_min_radius || lod_min_radius_value == other.lod_min_radius_value)
                && has_lod_scale == other.has_lod_scale
                && (!has_lod_scale || lod_scale_value == other.lod_scale_value)
                && has_lod_pitch_threshold == other.has_lod_pitch_threshold
                && (!has_lod_pitch_threshold || lod_pitch_threshold_value == other.lod_pitch_threshold_value)
                && has_lod_zoom_shift == other.has_lod_zoom_shift
                && (!has_lod_zoom_shift || lod_zoom_shift_value == other.lod_zoom_shift_value)
                && has_lod_mode == other.has_lod_mode
                && (!has_lod_mode || lod_mode_value == other.lod_mode_value);
        }

        internal Raw.MapTileOptions to_native () {
            Raw.MapTileOptions options = Raw.map_tile_options_default ();
            if (has_prefetch_zoom_delta) {
                options.prefetch_zoom_delta = prefetch_zoom_delta_value;
                options.fields |= 1U << 0;
            }
            if (has_lod_min_radius) {
                options.lod_min_radius = lod_min_radius_value;
                options.fields |= 1U << 1;
            }
            if (has_lod_scale) {
                options.lod_scale = lod_scale_value;
                options.fields |= 1U << 2;
            }
            if (has_lod_pitch_threshold) {
                options.lod_pitch_threshold = lod_pitch_threshold_value;
                options.fields |= 1U << 3;
            }
            if (has_lod_zoom_shift) {
                options.lod_zoom_shift = lod_zoom_shift_value;
                options.fields |= 1U << 4;
            }
            if (has_lod_mode) {
                options.lod_mode = (uint32) lod_mode_value;
                options.fields |= 1U << 5;
            }
            return options;
        }
    }

    private CustomGeometrySourceRegistration[] leaked_custom_geometry_sources;
    private Mutex leaked_custom_geometry_source_roots_mutex;

    private void retain_leaked_custom_geometry_sources (CustomGeometrySourceRegistration[] registrations) {
        leaked_custom_geometry_source_roots_mutex.lock ();
        for (var index = 0; index < registrations.length; index++) {
            leaked_custom_geometry_sources += registrations[index];
        }
        leaked_custom_geometry_source_roots_mutex.unlock ();
    }

    private class MapNativeLease {
        private MapHandle owner;
        public unowned Raw.Map native { get; private set; }

        internal MapNativeLease (MapHandle owner, Raw.Map native) {
            this.owner = owner;
            this.native = native;
        }

        ~MapNativeLease () {
            owner.release_native_lease ();
        }
    }

    public class MapHandle {
        private RuntimeHandle runtime;
        private Raw.Map? native;
        private CustomGeometrySourceRegistration[] custom_geometry_sources = new CustomGeometrySourceRegistration[0];
        private Mutex state_mutex;
        private Cond idle;
        private bool releasing;
        private uint active_native_leases;

        public bool closed {
            get {
                state_mutex.lock ();
                var value = native == null;
                state_mutex.unlock ();
                return value;
            }
        }

        public MapHandle (RuntimeHandle runtime, MapOptions? options = null) throws Error {
            this.runtime = runtime;
            var options_snapshot = (options ?? new MapOptions ()).copy ();
            var native_options = options_snapshot.to_native ();
            Raw.Map created;
            var runtime_lease = runtime.require_live ();
            check_status (Raw.map_create (runtime_lease.native, &native_options, out created));
            native = (owned) created;
            runtime.register_map (this);
        }

        public MapSize get_size () throws Error {
            uint32 width;
            uint32 height;
            double scale_factor;
            check_status (Raw.map_get_size (require_live ().native, out width, out height, out scale_factor));
            return MapSize (width, height, scale_factor);
        }

        ~MapHandle () {
            state_mutex.lock ();
            var leaked = native != null;
            if (leaked) {
                retain_leaked_custom_geometry_sources (custom_geometry_sources);
            }
            state_mutex.unlock ();
            if (leaked) {
                warning ("MapHandle finalized while live; call close() on the owner thread");
            }
        }

        internal MapNativeLease require_live () throws Error {
            state_mutex.lock ();
            if (native == null || releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("map handle is closed");
            }
            active_native_leases++;
            var lease = new MapNativeLease (this, native);
            state_mutex.unlock ();
            return lease;
        }

        internal void release_native_lease () {
            state_mutex.lock ();
            active_native_leases--;
            if (releasing && active_native_leases == 0) {
                idle.broadcast ();
            }
            state_mutex.unlock ();
        }

        internal bool matches_native_source (void* source) {
            state_mutex.lock ();
            var matches = native != null && !releasing && (void*) native == source;
            state_mutex.unlock ();
            return matches;
        }

        internal uint custom_geometry_source_registration_count {
            get { return custom_geometry_sources.length; }
        }

        private void retain_custom_geometry_source (CustomGeometrySourceRegistration registration) {
            Utf8String source_id = registration.source_id;
            release_custom_geometry_source (source_id);
            var retained = new CustomGeometrySourceRegistration[custom_geometry_sources.length + 1];
            for (var index = 0; index < custom_geometry_sources.length; index++) {
                retained[index] = custom_geometry_sources[index];
            }
            retained[custom_geometry_sources.length] = registration;
            custom_geometry_sources = retained;
        }

        private void release_custom_geometry_source (Utf8String source_id) {
            CustomGeometrySourceRegistration? released = null;
            uint retained_count = 0;
            for (var index = 0; index < custom_geometry_sources.length; index++) {
                if (!custom_geometry_sources[index].source_id.equal (source_id)) {
                    retained_count++;
                }
            }
            var retained = new CustomGeometrySourceRegistration[retained_count];
            uint output_index = 0;
            for (var index = 0; index < custom_geometry_sources.length; index++) {
                if (!custom_geometry_sources[index].source_id.equal (source_id)) {
                    retained[output_index++] = custom_geometry_sources[index];
                } else {
                    released = custom_geometry_sources[index];
                }
            }
            custom_geometry_sources = retained;
            if (released != null) {
                released.close ();
            }
        }

        private void clear_custom_geometry_sources () {
            var released = custom_geometry_sources;
            custom_geometry_sources = new CustomGeometrySourceRegistration[0];
            for (var index = 0; index < released.length; index++) {
                released[index].close ();
            }
        }

        internal void release_replaced_custom_geometry_sources () throws Error {
            var lease = require_live ();
            var retained_flags = new bool[custom_geometry_sources.length];
            uint retained_count = 0;
            for (var index = 0; index < custom_geometry_sources.length; index++) {
                uint32 source_type;
                bool found;
                check_status (Raw.map_get_style_source_type (
                    lease.native,
                    custom_geometry_sources[index].source_id.to_native (),
                    out source_type,
                    out found
                ));
                retained_flags[index] = found && source_type == (uint32) Raw.StyleSourceType.CUSTOM_VECTOR;
                if (retained_flags[index]) {
                    retained_count++;
                }
            }

            var retained = new CustomGeometrySourceRegistration[retained_count];
            uint output_index = 0;
            for (var index = 0; index < custom_geometry_sources.length; index++) {
                if (retained_flags[index]) {
                    retained[output_index++] = custom_geometry_sources[index];
                } else {
                    custom_geometry_sources[index].close ();
                }
            }
            custom_geometry_sources = retained;
        }

        public void close () throws Error {
            state_mutex.lock ();
            if (native == null) {
                state_mutex.unlock ();
                return;
            }
            if (releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("map handle release is already in progress");
            }
            releasing = true;
            while (active_native_leases > 0) {
                idle.wait (state_mutex);
            }
            unowned Raw.Map closing = native;
            state_mutex.unlock ();

            var status = Raw.map_destroy (closing);

            state_mutex.lock ();
            if (status == Raw.Status.OK) {
                native = null;
            }
            releasing = false;
            idle.broadcast ();
            state_mutex.unlock ();
            check_status (status);
            runtime.unregister_map (this);
            clear_custom_geometry_sources ();
        }

        public void request_repaint () throws Error {
            check_status (Raw.map_request_repaint (require_live ().native));
        }

        public void request_still_image () throws Error {
            check_status (Raw.map_request_still_image (require_live ().native));
        }

        public void set_style_url (string url) throws Error {
            check_status (Raw.map_set_style_url (require_live ().native, c_string (url)));
        }

        public void set_style_json (string json) throws Error {
            check_status (Raw.map_set_style_json (require_live ().native, c_string (json)));
            clear_custom_geometry_sources ();
        }

        public void set_debug_options (MapDebugOptions options) throws Error {
            check_status (Raw.map_set_debug_options (require_live ().native, (uint32) options));
        }

        public MapDebugOptions get_debug_options () throws Error {
            uint32 options;
            check_status (Raw.map_get_debug_options (require_live ().native, out options));
            return (MapDebugOptions) options;
        }

        public void set_rendering_stats_view_enabled (bool enabled) throws Error {
            check_status (Raw.map_set_rendering_stats_view_enabled (require_live ().native, enabled));
        }

        public bool get_rendering_stats_view_enabled () throws Error {
            bool enabled;
            check_status (Raw.map_get_rendering_stats_view_enabled (require_live ().native, out enabled));
            return enabled;
        }

        public bool is_fully_loaded () throws Error {
            bool loaded;
            check_status (Raw.map_is_fully_loaded (require_live ().native, out loaded));
            return loaded;
        }

        public void dump_debug_logs () throws Error {
            check_status (Raw.map_dump_debug_logs (require_live ().native));
        }

        public MapViewportOptions get_viewport_options () throws Error {
            Raw.MapViewportOptions options = {};
            options.size = (uint32) sizeof (Raw.MapViewportOptions);
            check_status (Raw.map_get_viewport_options (require_live ().native, &options));
            return new MapViewportOptions.from_native (options);
        }

        public void set_viewport_options (MapViewportOptions options) throws Error {
            var native_options = options.to_native ();
            check_status (Raw.map_set_viewport_options (require_live ().native, &native_options));
        }

        public MapTileOptions get_tile_options () throws Error {
            Raw.MapTileOptions options = {};
            options.size = (uint32) sizeof (Raw.MapTileOptions);
            check_status (Raw.map_get_tile_options (require_live ().native, &options));
            return new MapTileOptions.from_native (options);
        }

        public void set_tile_options (MapTileOptions options) throws Error {
            var native_options = options.to_native ();
            check_status (Raw.map_set_tile_options (require_live ().native, &native_options));
        }

        public CameraOptions get_camera () throws Error {
            Raw.CameraOptions native_camera = {};
            native_camera.size = (uint32) sizeof (Raw.CameraOptions);
            check_status (Raw.map_get_camera (require_live ().native, &native_camera));
            return CameraOptions.from_native (native_camera);
        }

        public void jump_to (CameraOptions camera) throws Error {
            var native_camera = camera.to_native ();
            check_status (Raw.map_jump_to (require_live ().native, &native_camera));
        }

        public void ease_to (CameraOptions camera, AnimationOptions? animation = null) throws Error {
            var native_camera = camera.to_native ();
            Raw.AnimationOptions native_animation = {};
            Raw.AnimationOptions* animation_ptr = null;
            if (animation != null) {
                native_animation = animation.to_native ();
                animation_ptr = &native_animation;
            }
            check_status (Raw.map_ease_to (require_live ().native, &native_camera, animation_ptr));
        }

        public void fly_to (CameraOptions camera, AnimationOptions? animation = null) throws Error {
            var native_camera = camera.to_native ();
            Raw.AnimationOptions native_animation = {};
            Raw.AnimationOptions* animation_ptr = null;
            if (animation != null) {
                native_animation = animation.to_native ();
                animation_ptr = &native_animation;
            }
            check_status (Raw.map_fly_to (require_live ().native, &native_camera, animation_ptr));
        }

        public void move_by (double delta_x, double delta_y) throws Error {
            check_status (Raw.map_move_by (require_live ().native, delta_x, delta_y));
        }

        public void move_by_animated (double delta_x, double delta_y, AnimationOptions? animation = null) throws Error {
            Raw.AnimationOptions native_animation = {};
            Raw.AnimationOptions* animation_ptr = null;
            if (animation != null) {
                native_animation = animation.to_native ();
                animation_ptr = &native_animation;
            }
            check_status (Raw.map_move_by_animated (require_live ().native, delta_x, delta_y, animation_ptr));
        }

        public void scale_by (double scale) throws Error {
            check_status (Raw.map_scale_by (require_live ().native, scale, null));
        }

        public void scale_by_at (double scale, ScreenPoint anchor) throws Error {
            var native_anchor = anchor.to_native ();
            check_status (Raw.map_scale_by (require_live ().native, scale, &native_anchor));
        }

        public void scale_by_animated (double scale, AnimationOptions? animation = null) throws Error {
            Raw.AnimationOptions native_animation = {};
            Raw.AnimationOptions* animation_ptr = null;
            if (animation != null) {
                native_animation = animation.to_native ();
                animation_ptr = &native_animation;
            }
            check_status (Raw.map_scale_by_animated (require_live ().native, scale, null, animation_ptr));
        }

        public void scale_by_at_animated (double scale, ScreenPoint anchor, AnimationOptions? animation = null) throws Error {
            var native_anchor = anchor.to_native ();
            Raw.AnimationOptions native_animation = {};
            Raw.AnimationOptions* animation_ptr = null;
            if (animation != null) {
                native_animation = animation.to_native ();
                animation_ptr = &native_animation;
            }
            check_status (Raw.map_scale_by_animated (require_live ().native, scale, &native_anchor, animation_ptr));
        }

        public void rotate_by (ScreenPoint first, ScreenPoint second) throws Error {
            check_status (Raw.map_rotate_by (require_live ().native, first.to_native (), second.to_native ()));
        }

        public void rotate_by_animated (ScreenPoint first, ScreenPoint second, AnimationOptions? animation = null) throws Error {
            Raw.AnimationOptions native_animation = {};
            Raw.AnimationOptions* animation_ptr = null;
            if (animation != null) {
                native_animation = animation.to_native ();
                animation_ptr = &native_animation;
            }
            check_status (Raw.map_rotate_by_animated (require_live ().native, first.to_native (), second.to_native (), animation_ptr));
        }

        public void pitch_by (double pitch) throws Error {
            check_status (Raw.map_pitch_by (require_live ().native, pitch));
        }

        public void pitch_by_animated (double pitch, AnimationOptions? animation = null) throws Error {
            Raw.AnimationOptions native_animation = {};
            Raw.AnimationOptions* animation_ptr = null;
            if (animation != null) {
                native_animation = animation.to_native ();
                animation_ptr = &native_animation;
            }
            check_status (Raw.map_pitch_by_animated (require_live ().native, pitch, animation_ptr));
        }

        public void cancel_transitions () throws Error {
            check_status (Raw.map_cancel_transitions (require_live ().native));
        }

        public CameraOptions camera_for_lat_lng_bounds (LatLngBounds bounds, CameraFitOptions? fit_options = null) throws Error {
            Raw.CameraFitOptions native_fit_options = {};
            Raw.CameraFitOptions* fit_options_ptr = null;
            if (fit_options != null) {
                native_fit_options = fit_options.to_native ();
                fit_options_ptr = &native_fit_options;
            }
            Raw.CameraOptions out_camera = {};
            out_camera.size = (uint32) sizeof (Raw.CameraOptions);
            check_status (Raw.map_camera_for_lat_lng_bounds (require_live ().native, bounds.to_native (), fit_options_ptr, &out_camera));
            return CameraOptions.from_native (out_camera);
        }

        public CameraOptions camera_for_lat_lngs (LatLng[] coordinates, CameraFitOptions? fit_options = null) throws Error {
            if (coordinates.length == 0) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("coordinates are empty");
            }
            Raw.LatLng[] native_coordinates = new Raw.LatLng[coordinates.length];
            for (var i = 0; i < coordinates.length; i++) {
                native_coordinates[i] = coordinates[i].to_native ();
            }
            Raw.CameraFitOptions native_fit_options = {};
            Raw.CameraFitOptions* fit_options_ptr = null;
            if (fit_options != null) {
                native_fit_options = fit_options.to_native ();
                fit_options_ptr = &native_fit_options;
            }
            Raw.CameraOptions out_camera = {};
            out_camera.size = (uint32) sizeof (Raw.CameraOptions);
            check_status (Raw.map_camera_for_lat_lngs (require_live ().native, native_coordinates, native_coordinates.length, fit_options_ptr, &out_camera));
            return CameraOptions.from_native (out_camera);
        }

        public CameraOptions camera_for_geometry (Geometry geometry, CameraFitOptions? fit_options = null) throws Error {
            var geometry_storage = geometry.copy ();
            Raw.Geometry native_geometry = geometry_storage.to_native ();
            Raw.CameraFitOptions native_fit_options = {};
            Raw.CameraFitOptions* fit_options_ptr = null;
            if (fit_options != null) {
                native_fit_options = fit_options.to_native ();
                fit_options_ptr = &native_fit_options;
            }
            Raw.CameraOptions out_camera = {};
            out_camera.size = (uint32) sizeof (Raw.CameraOptions);
            check_status (Raw.map_camera_for_geometry (require_live ().native, &native_geometry, fit_options_ptr, &out_camera));
            return CameraOptions.from_native (out_camera);
        }

        public LatLngBounds lat_lng_bounds_for_camera (CameraOptions camera) throws Error {
            var native_camera = camera.to_native ();
            Raw.LatLngBounds bounds;
            check_status (Raw.map_lat_lng_bounds_for_camera (require_live ().native, &native_camera, out bounds));
            return LatLngBounds.from_native (bounds);
        }

        public LatLngBounds lat_lng_bounds_for_camera_unwrapped (CameraOptions camera) throws Error {
            var native_camera = camera.to_native ();
            Raw.LatLngBounds bounds;
            check_status (Raw.map_lat_lng_bounds_for_camera_unwrapped (require_live ().native, &native_camera, out bounds));
            return LatLngBounds.from_native (bounds);
        }

        public BoundOptions get_bounds () throws Error {
            Raw.BoundOptions options = {};
            options.size = (uint32) sizeof (Raw.BoundOptions);
            check_status (Raw.map_get_bounds (require_live ().native, &options));
            return new BoundOptions.from_native (options);
        }

        public void set_bounds (BoundOptions options) throws Error {
            var native_options = options.to_native ();
            check_status (Raw.map_set_bounds (require_live ().native, &native_options));
        }

        public FreeCameraOptions get_free_camera_options () throws Error {
            Raw.FreeCameraOptions options = {};
            options.size = (uint32) sizeof (Raw.FreeCameraOptions);
            check_status (Raw.map_get_free_camera_options (require_live ().native, &options));
            return new FreeCameraOptions.from_native (options);
        }

        public void set_free_camera_options (FreeCameraOptions options) throws Error {
            var native_options = options.to_native ();
            check_status (Raw.map_set_free_camera_options (require_live ().native, &native_options));
        }

        public ProjectionMode get_projection_mode () throws Error {
            Raw.ProjectionMode mode = {};
            mode.size = (uint32) sizeof (Raw.ProjectionMode);
            check_status (Raw.map_get_projection_mode (require_live ().native, &mode));
            return new ProjectionMode.from_native (mode);
        }

        public void set_projection_mode (ProjectionMode mode) throws Error {
            var native_mode = mode.to_native ();
            check_status (Raw.map_set_projection_mode (require_live ().native, &native_mode));
        }

        public ScreenPoint pixel_for_lat_lng (LatLng coordinate) throws Error {
            Raw.ScreenPoint point;
            check_status (Raw.map_pixel_for_lat_lng (require_live ().native, coordinate.to_native (), out point));
            return ScreenPoint.from_native (point);
        }

        public LatLng lat_lng_for_pixel (ScreenPoint point) throws Error {
            Raw.LatLng coordinate;
            check_status (Raw.map_lat_lng_for_pixel (require_live ().native, point.to_native (), out coordinate));
            return LatLng.from_native (coordinate);
        }

        public ScreenPoint[] pixels_for_lat_lngs (LatLng[] coordinates) throws Error {
            Raw.LatLng[] native_coordinates = new Raw.LatLng[coordinates.length];
            for (var i = 0; i < coordinates.length; i++) {
                native_coordinates[i] = coordinates[i].to_native ();
            }
            Raw.ScreenPoint[] native_points = new Raw.ScreenPoint[coordinates.length];
            check_status (Raw.map_pixels_for_lat_lngs (require_live ().native, native_coordinates, native_coordinates.length, native_points));
            ScreenPoint[] points = new ScreenPoint[coordinates.length];
            for (var i = 0; i < native_points.length; i++) {
                points[i] = ScreenPoint.from_native (native_points[i]);
            }
            return points;
        }

        public LatLng[] lat_lngs_for_pixels (ScreenPoint[] points) throws Error {
            Raw.ScreenPoint[] native_points = new Raw.ScreenPoint[points.length];
            for (var i = 0; i < points.length; i++) {
                native_points[i] = points[i].to_native ();
            }
            Raw.LatLng[] native_coordinates = new Raw.LatLng[points.length];
            check_status (Raw.map_lat_lngs_for_pixels (require_live ().native, native_points, native_points.length, native_coordinates));
            LatLng[] coordinates = new LatLng[points.length];
            for (var i = 0; i < native_coordinates.length; i++) {
                coordinates[i] = LatLng.from_native (native_coordinates[i]);
            }
            return coordinates;
        }

        public MapProjectionHandle create_projection () throws Error {
            Raw.MapProjection projection;
            check_status (Raw.map_projection_create (require_live ().native, out projection));
            return new MapProjectionHandle ((owned) projection);
        }

        public RenderSessionHandle attach_metal_owned_texture (MetalOwnedTextureDescriptor descriptor) throws Error {
            var descriptor_storage = descriptor.copy ();
            var native_descriptor = descriptor_storage.to_native ();
            Raw.RenderSession session;
            check_status (Raw.metal_owned_texture_attach (require_live ().native, &native_descriptor, out session));
            return new RenderSessionHandle (this, (owned) session);
        }

        public RenderSessionHandle attach_metal_borrowed_texture (MetalBorrowedTextureDescriptor descriptor) throws Error {
            var descriptor_storage = descriptor.copy ();
            var native_descriptor = descriptor_storage.to_native ();
            Raw.RenderSession session;
            check_status (Raw.metal_borrowed_texture_attach (require_live ().native, &native_descriptor, out session));
            return new RenderSessionHandle (this, (owned) session);
        }

        public RenderSessionHandle attach_vulkan_owned_texture (VulkanOwnedTextureDescriptor descriptor) throws Error {
            var descriptor_storage = descriptor.copy ();
            var native_descriptor = descriptor_storage.to_native ();
            Raw.RenderSession session;
            check_status (Raw.vulkan_owned_texture_attach (require_live ().native, &native_descriptor, out session));
            return new RenderSessionHandle (this, (owned) session);
        }

        public RenderSessionHandle attach_vulkan_borrowed_texture (VulkanBorrowedTextureDescriptor descriptor) throws Error {
            var descriptor_storage = descriptor.copy ();
            var native_descriptor = descriptor_storage.to_native ();
            Raw.RenderSession session;
            check_status (Raw.vulkan_borrowed_texture_attach (require_live ().native, &native_descriptor, out session));
            return new RenderSessionHandle (this, (owned) session);
        }

        public RenderSessionHandle attach_opengl_owned_texture (OpenGLOwnedTextureDescriptor descriptor) throws Error {
            var descriptor_storage = descriptor.copy ();
            var native_descriptor = descriptor_storage.to_native ();
            Raw.RenderSession session;
            check_status (Raw.opengl_owned_texture_attach (require_live ().native, &native_descriptor, out session));
            return new RenderSessionHandle (this, (owned) session);
        }

        public RenderSessionHandle attach_opengl_borrowed_texture (OpenGLBorrowedTextureDescriptor descriptor) throws Error {
            var descriptor_storage = descriptor.copy ();
            var native_descriptor = descriptor_storage.to_native ();
            Raw.RenderSession session;
            check_status (Raw.opengl_borrowed_texture_attach (require_live ().native, &native_descriptor, out session));
            return new RenderSessionHandle (this, (owned) session);
        }

        public RenderSessionHandle attach_metal_surface (MetalSurfaceDescriptor descriptor) throws Error {
            var descriptor_storage = descriptor.copy ();
            var native_descriptor = descriptor_storage.to_native ();
            Raw.RenderSession session;
            check_status (Raw.metal_surface_attach (require_live ().native, &native_descriptor, out session));
            return new RenderSessionHandle (this, (owned) session);
        }

        public RenderSessionHandle attach_vulkan_surface (VulkanSurfaceDescriptor descriptor) throws Error {
            var descriptor_storage = descriptor.copy ();
            var native_descriptor = descriptor_storage.to_native ();
            Raw.RenderSession session;
            check_status (Raw.vulkan_surface_attach (require_live ().native, &native_descriptor, out session));
            return new RenderSessionHandle (this, (owned) session);
        }

        public RenderSessionHandle attach_opengl_surface (OpenGLSurfaceDescriptor descriptor) throws Error {
            var descriptor_storage = descriptor.copy ();
            var native_descriptor = descriptor_storage.to_native ();
            Raw.RenderSession session;
            check_status (Raw.opengl_surface_attach (require_live ().native, &native_descriptor, out session));
            return new RenderSessionHandle (this, (owned) session);
        }

        public void add_geojson_source_url (string source_id, string url, GeoJsonSourceOptions? options = null) throws Error {
            add_geojson_source_url_utf8 (new Utf8String (source_id), new Utf8String (url), options);
        }

        public void add_geojson_source_url_utf8 (Utf8String source_id, Utf8String url, GeoJsonSourceOptions? options = null) throws Error {
            Raw.GeoJsonSourceOptions native_options = {};
            Raw.GeoJsonSourceOptions* native_options_pointer = null;
            Raw.JsonValue cluster_properties_storage = {};
            JsonValue? cluster_properties_owner = null;
            GeoJsonSourceOptions? options_storage = options == null ? null : options.copy ();
            if (options_storage != null) {
                native_options = options_storage.to_native (ref cluster_properties_storage, out cluster_properties_owner);
                native_options_pointer = &native_options;
            }
            check_status (Raw.map_add_geojson_source_url (require_live ().native, source_id.to_native (), url.to_native (), native_options_pointer));
        }

        public void add_geojson_source_data (string source_id, GeoJson data, GeoJsonSourceOptions? options = null) throws Error {
            add_geojson_source_data_utf8 (new Utf8String (source_id), data, options);
        }

        public void add_geojson_source_data_utf8 (Utf8String source_id, GeoJson data, GeoJsonSourceOptions? options = null) throws Error {
            var data_storage = data.copy ();
            var native_data = data_storage.to_native ();
            Raw.GeoJsonSourceOptions native_options = {};
            Raw.GeoJsonSourceOptions* native_options_pointer = null;
            Raw.JsonValue cluster_properties_storage = {};
            JsonValue? cluster_properties_owner = null;
            GeoJsonSourceOptions? options_storage = options == null ? null : options.copy ();
            if (options_storage != null) {
                native_options = options_storage.to_native (ref cluster_properties_storage, out cluster_properties_owner);
                native_options_pointer = &native_options;
            }
            check_status (Raw.map_add_geojson_source_data (require_live ().native, source_id.to_native (), &native_data, native_options_pointer));
        }

        public void set_geojson_source_url (string source_id, string url) throws Error {
            set_geojson_source_url_utf8 (new Utf8String (source_id), new Utf8String (url));
        }

        public void set_geojson_source_url_utf8 (Utf8String source_id, Utf8String url) throws Error {
            check_status (Raw.map_set_geojson_source_url (require_live ().native, source_id.to_native (), url.to_native ()));
        }

        public void set_geojson_source_data (string source_id, GeoJson data) throws Error {
            set_geojson_source_data_utf8 (new Utf8String (source_id), data);
        }

        public void set_geojson_source_data_utf8 (Utf8String source_id, GeoJson data) throws Error {
            var data_storage = data.copy ();
            var native_data = data_storage.to_native ();
            check_status (Raw.map_set_geojson_source_data (require_live ().native, source_id.to_native (), &native_data));
        }

        public void add_style_source_json (string source_id, JsonValue source_json) throws Error {
            add_style_source_json_utf8 (new Utf8String (source_id), source_json);
        }

        public void add_style_source_json_utf8 (Utf8String source_id, JsonValue source_json) throws Error {
            var source_json_storage = source_json.copy ();
            var native_source_json = source_json_storage.to_native ();
            check_status (Raw.map_add_style_source_json (require_live ().native, source_id.to_native (), &native_source_json));
        }

        public void add_vector_source_url (string source_id, string url, StyleTileSourceOptions? options = null) throws Error {
            add_vector_source_url_utf8 (new Utf8String (source_id), new Utf8String (url), options);
        }

        public void add_vector_source_url_utf8 (Utf8String source_id, Utf8String url, StyleTileSourceOptions? options = null) throws Error {
            Raw.StyleTileSourceOptions native_options = {};
            Raw.StyleTileSourceOptions* options_ptr = null;
            StyleTileSourceOptions? options_storage = null;
            if (options != null) {
                options_storage = options.copy ();
                native_options = options_storage.to_native ();
                options_ptr = &native_options;
            }
            check_status (Raw.map_add_vector_source_url (require_live ().native, source_id.to_native (), url.to_native (), options_ptr));
        }

        public void add_vector_source_tiles (string source_id, string[] tiles, StyleTileSourceOptions? options = null) throws Error {
            Utf8String[] utf8_tiles = new Utf8String[tiles.length];
            for (var index = 0; index < tiles.length; index++) {
                if (tiles[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("tile URL is null");
                }
                utf8_tiles[index] = new Utf8String (tiles[index]);
            }
            add_vector_source_tiles_utf8 (new Utf8String (source_id), utf8_tiles, options);
        }

        public void add_vector_source_tiles_utf8 (Utf8String source_id, Utf8String[] tiles, StyleTileSourceOptions? options = null) throws Error {
            Utf8String[] tile_storage;
            Raw.StringView[] tile_views = string_views_for_tiles_utf8 (tiles, out tile_storage);
            Raw.StyleTileSourceOptions native_options = {};
            Raw.StyleTileSourceOptions* options_ptr = null;
            StyleTileSourceOptions? options_storage = null;
            if (options != null) {
                options_storage = options.copy ();
                native_options = options_storage.to_native ();
                options_ptr = &native_options;
            }
            check_status (Raw.map_add_vector_source_tiles (require_live ().native, source_id.to_native (), tile_views, tile_views.length, options_ptr));
        }

        public void add_raster_source_url (string source_id, string url, StyleTileSourceOptions? options = null) throws Error {
            add_raster_source_url_utf8 (new Utf8String (source_id), new Utf8String (url), options);
        }

        public void add_raster_source_url_utf8 (Utf8String source_id, Utf8String url, StyleTileSourceOptions? options = null) throws Error {
            Raw.StyleTileSourceOptions native_options = {};
            Raw.StyleTileSourceOptions* options_ptr = null;
            StyleTileSourceOptions? options_storage = null;
            if (options != null) {
                options_storage = options.copy ();
                native_options = options_storage.to_native ();
                options_ptr = &native_options;
            }
            check_status (Raw.map_add_raster_source_url (require_live ().native, source_id.to_native (), url.to_native (), options_ptr));
        }

        public void add_raster_source_tiles (string source_id, string[] tiles, StyleTileSourceOptions? options = null) throws Error {
            Utf8String[] utf8_tiles = new Utf8String[tiles.length];
            for (var index = 0; index < tiles.length; index++) {
                if (tiles[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("tile URL is null");
                }
                utf8_tiles[index] = new Utf8String (tiles[index]);
            }
            add_raster_source_tiles_utf8 (new Utf8String (source_id), utf8_tiles, options);
        }

        public void add_raster_source_tiles_utf8 (Utf8String source_id, Utf8String[] tiles, StyleTileSourceOptions? options = null) throws Error {
            Utf8String[] tile_storage;
            Raw.StringView[] tile_views = string_views_for_tiles_utf8 (tiles, out tile_storage);
            Raw.StyleTileSourceOptions native_options = {};
            Raw.StyleTileSourceOptions* options_ptr = null;
            StyleTileSourceOptions? options_storage = null;
            if (options != null) {
                options_storage = options.copy ();
                native_options = options_storage.to_native ();
                options_ptr = &native_options;
            }
            check_status (Raw.map_add_raster_source_tiles (require_live ().native, source_id.to_native (), tile_views, tile_views.length, options_ptr));
        }

        public void add_raster_dem_source_url (string source_id, string url, StyleTileSourceOptions? options = null) throws Error {
            add_raster_dem_source_url_utf8 (new Utf8String (source_id), new Utf8String (url), options);
        }

        public void add_raster_dem_source_url_utf8 (Utf8String source_id, Utf8String url, StyleTileSourceOptions? options = null) throws Error {
            Raw.StyleTileSourceOptions native_options = {};
            Raw.StyleTileSourceOptions* options_ptr = null;
            StyleTileSourceOptions? options_storage = null;
            if (options != null) {
                options_storage = options.copy ();
                native_options = options_storage.to_native ();
                options_ptr = &native_options;
            }
            check_status (Raw.map_add_raster_dem_source_url (require_live ().native, source_id.to_native (), url.to_native (), options_ptr));
        }

        public void add_raster_dem_source_tiles (string source_id, string[] tiles, StyleTileSourceOptions? options = null) throws Error {
            Utf8String[] utf8_tiles = new Utf8String[tiles.length];
            for (var index = 0; index < tiles.length; index++) {
                if (tiles[index] == null) {
                    clear_unknown_status ();
                    throw new Error.INVALID_ARGUMENT ("tile URL is null");
                }
                utf8_tiles[index] = new Utf8String (tiles[index]);
            }
            add_raster_dem_source_tiles_utf8 (new Utf8String (source_id), utf8_tiles, options);
        }

        public void add_raster_dem_source_tiles_utf8 (Utf8String source_id, Utf8String[] tiles, StyleTileSourceOptions? options = null) throws Error {
            Utf8String[] tile_storage;
            Raw.StringView[] tile_views = string_views_for_tiles_utf8 (tiles, out tile_storage);
            Raw.StyleTileSourceOptions native_options = {};
            Raw.StyleTileSourceOptions* options_ptr = null;
            StyleTileSourceOptions? options_storage = null;
            if (options != null) {
                options_storage = options.copy ();
                native_options = options_storage.to_native ();
                options_ptr = &native_options;
            }
            check_status (Raw.map_add_raster_dem_source_tiles (require_live ().native, source_id.to_native (), tile_views, tile_views.length, options_ptr));
        }

        public void add_custom_geometry_source (string source_id, CustomGeometrySourceOptions options) throws Error {
            add_custom_geometry_source_utf8 (new Utf8String (source_id), options);
        }

        public void add_custom_geometry_source_utf8 (Utf8String source_id, CustomGeometrySourceOptions options) throws Error {
            var registration = new CustomGeometrySourceRegistration (source_id, options);
            var native_options = options.to_native (registration);
            check_status (Raw.map_add_custom_geometry_source (require_live ().native, source_id.to_native (), &native_options));
            retain_custom_geometry_source (registration);
        }

        public void set_custom_geometry_source_tile_data (string source_id, CanonicalTileId tile_id, GeoJson data) throws Error {
            set_custom_geometry_source_tile_data_utf8 (new Utf8String (source_id), tile_id, data);
        }

        public void set_custom_geometry_source_tile_data_utf8 (Utf8String source_id, CanonicalTileId tile_id, GeoJson data) throws Error {
            var data_storage = data.copy ();
            var native_data = data_storage.to_native ();
            check_status (Raw.map_set_custom_geometry_source_tile_data (require_live ().native, source_id.to_native (), tile_id.to_native (), &native_data));
        }

        public void invalidate_custom_geometry_source_tile (string source_id, CanonicalTileId tile_id) throws Error {
            invalidate_custom_geometry_source_tile_utf8 (new Utf8String (source_id), tile_id);
        }

        public void invalidate_custom_geometry_source_tile_utf8 (Utf8String source_id, CanonicalTileId tile_id) throws Error {
            check_status (Raw.map_invalidate_custom_geometry_source_tile (require_live ().native, source_id.to_native (), tile_id.to_native ()));
        }

        public void invalidate_custom_geometry_source_region (string source_id, LatLngBounds bounds) throws Error {
            invalidate_custom_geometry_source_region_utf8 (new Utf8String (source_id), bounds);
        }

        public void invalidate_custom_geometry_source_region_utf8 (Utf8String source_id, LatLngBounds bounds) throws Error {
            check_status (Raw.map_invalidate_custom_geometry_source_region (require_live ().native, source_id.to_native (), bounds.to_native ()));
        }

        public bool remove_style_source (string source_id) throws Error {
            return remove_style_source_utf8 (new Utf8String (source_id));
        }

        public bool remove_style_source_utf8 (Utf8String source_id) throws Error {
            bool removed;
            check_status (Raw.map_remove_style_source (require_live ().native, source_id.to_native (), out removed));
            if (removed) {
                release_custom_geometry_source (source_id);
            }
            return removed;
        }

        public bool style_source_exists (string source_id) throws Error {
            return style_source_exists_utf8 (new Utf8String (source_id));
        }

        public bool style_source_exists_utf8 (Utf8String source_id) throws Error {
            bool exists;
            check_status (Raw.map_style_source_exists (require_live ().native, source_id.to_native (), out exists));
            return exists;
        }

        public StyleSourceType? get_style_source_type (string source_id) throws Error {
            return get_style_source_type_utf8 (new Utf8String (source_id));
        }

        public StyleSourceType? get_style_source_type_utf8 (Utf8String source_id) throws Error {
            uint32 source_type;
            bool found;
            check_status (Raw.map_get_style_source_type (require_live ().native, source_id.to_native (), out source_type, out found));
            if (!found) {
                return null;
            }
            return style_source_type_from_raw (source_type);
        }

        public StyleSourceInfo? get_style_source_info (string source_id) throws Error {
            return get_style_source_info_utf8 (new Utf8String (source_id));
        }

        public StyleSourceInfo? get_style_source_info_utf8 (Utf8String source_id) throws Error {
            Raw.StyleSourceInfo info = {};
            info.size = (uint32) sizeof (Raw.StyleSourceInfo);
            bool found;
            check_status (Raw.map_get_style_source_info (require_live ().native, source_id.to_native (), &info, out found));
            return found ? new StyleSourceInfo (info) : null;
        }

        public string? copy_style_source_attribution (string source_id) throws Error {
            var copied = copy_style_source_attribution_utf8 (new Utf8String (source_id));
            return copied == null ? null : copied.to_string ();
        }

        public Utf8String? copy_style_source_attribution_utf8 (Utf8String source_id) throws Error {
            var info = get_style_source_info_utf8 (source_id);
            if (info == null || !info.has_attribution) {
                return null;
            }
            uint8[] bytes = new uint8[info.attribution_byte_length + 1];
            size_t attribution_size;
            bool found;
            check_status (Raw.map_copy_style_source_attribution (require_live ().native, source_id.to_native (), (char*) bytes, bytes.length, out attribution_size, out found));
            if (!found) {
                return null;
            }
            uint8[] attribution = new uint8[attribution_size];
            for (var index = 0; index < attribution_size; index++) {
                attribution[index] = bytes[index];
            }
            return new Utf8String.from_bytes (attribution);
        }

        public StringList list_style_source_ids () throws Error {
            Raw.StyleIdList list;
            check_status (Raw.map_list_style_source_ids (require_live ().native, out list));
            return copy_style_id_list ((owned) list);
        }

        public void add_hillshade_layer (string layer_id, string source_id, string before_layer_id = "") throws Error {
            add_hillshade_layer_utf8 (new Utf8String (layer_id), new Utf8String (source_id), new Utf8String (before_layer_id));
        }

        public void add_hillshade_layer_utf8 (Utf8String layer_id, Utf8String source_id, Utf8String? before_layer_id = null) throws Error {
            var before = before_layer_id ?? new Utf8String ("");
            check_status (Raw.map_add_hillshade_layer (require_live ().native, layer_id.to_native (), source_id.to_native (), before.to_native ()));
        }

        public void add_color_relief_layer (string layer_id, string source_id, string before_layer_id = "") throws Error {
            add_color_relief_layer_utf8 (new Utf8String (layer_id), new Utf8String (source_id), new Utf8String (before_layer_id));
        }

        public void add_color_relief_layer_utf8 (Utf8String layer_id, Utf8String source_id, Utf8String? before_layer_id = null) throws Error {
            var before = before_layer_id ?? new Utf8String ("");
            check_status (Raw.map_add_color_relief_layer (require_live ().native, layer_id.to_native (), source_id.to_native (), before.to_native ()));
        }

        public void add_location_indicator_layer (string layer_id, string before_layer_id = "") throws Error {
            add_location_indicator_layer_utf8 (new Utf8String (layer_id), new Utf8String (before_layer_id));
        }

        public void add_location_indicator_layer_utf8 (Utf8String layer_id, Utf8String? before_layer_id = null) throws Error {
            var before = before_layer_id ?? new Utf8String ("");
            check_status (Raw.map_add_location_indicator_layer (require_live ().native, layer_id.to_native (), before.to_native ()));
        }

        public void set_location_indicator_location (string layer_id, LatLng coordinate, double altitude) throws Error {
            set_location_indicator_location_utf8 (new Utf8String (layer_id), coordinate, altitude);
        }

        public void set_location_indicator_location_utf8 (Utf8String layer_id, LatLng coordinate, double altitude) throws Error {
            check_status (Raw.map_set_location_indicator_location (require_live ().native, layer_id.to_native (), coordinate.to_native (), altitude));
        }

        public void set_location_indicator_bearing (string layer_id, double bearing) throws Error {
            set_location_indicator_bearing_utf8 (new Utf8String (layer_id), bearing);
        }

        public void set_location_indicator_bearing_utf8 (Utf8String layer_id, double bearing) throws Error {
            check_status (Raw.map_set_location_indicator_bearing (require_live ().native, layer_id.to_native (), bearing));
        }

        public void set_location_indicator_accuracy_radius (string layer_id, double radius) throws Error {
            set_location_indicator_accuracy_radius_utf8 (new Utf8String (layer_id), radius);
        }

        public void set_location_indicator_accuracy_radius_utf8 (Utf8String layer_id, double radius) throws Error {
            check_status (Raw.map_set_location_indicator_accuracy_radius (require_live ().native, layer_id.to_native (), radius));
        }

        public void set_location_indicator_image_name (string layer_id, LocationIndicatorImageKind image_kind, string image_id) throws Error {
            set_location_indicator_image_name_utf8 (new Utf8String (layer_id), image_kind, new Utf8String (image_id));
        }

        public void set_location_indicator_image_name_utf8 (Utf8String layer_id, LocationIndicatorImageKind image_kind, Utf8String image_id) throws Error {
            check_status (Raw.map_set_location_indicator_image_name (require_live ().native, layer_id.to_native (), (uint32) image_kind, image_id.to_native ()));
        }

        public void add_style_layer_json (JsonValue layer_json, string before_layer_id = "") throws Error {
            add_style_layer_json_utf8 (layer_json, new Utf8String (before_layer_id));
        }

        public void add_style_layer_json_utf8 (JsonValue layer_json, Utf8String? before_layer_id = null) throws Error {
            var layer_json_storage = layer_json.copy ();
            var native_layer_json = layer_json_storage.to_native ();
            var before = before_layer_id ?? new Utf8String ("");
            check_status (Raw.map_add_style_layer_json (require_live ().native, &native_layer_json, before.to_native ()));
        }

        public bool remove_style_layer (string layer_id) throws Error {
            return remove_style_layer_utf8 (new Utf8String (layer_id));
        }

        public bool remove_style_layer_utf8 (Utf8String layer_id) throws Error {
            bool removed;
            check_status (Raw.map_remove_style_layer (require_live ().native, layer_id.to_native (), out removed));
            return removed;
        }

        public bool style_layer_exists (string layer_id) throws Error {
            return style_layer_exists_utf8 (new Utf8String (layer_id));
        }

        public bool style_layer_exists_utf8 (Utf8String layer_id) throws Error {
            bool exists;
            check_status (Raw.map_style_layer_exists (require_live ().native, layer_id.to_native (), out exists));
            return exists;
        }

        public string? get_style_layer_type (string layer_id) throws Error {
            var copied = get_style_layer_type_utf8 (new Utf8String (layer_id));
            return copied == null ? null : copied.to_string ();
        }

        public Utf8String? get_style_layer_type_utf8 (Utf8String layer_id) throws Error {
            Raw.StringView layer_type;
            bool found;
            check_status (Raw.map_get_style_layer_type (require_live ().native, layer_id.to_native (), out layer_type, out found));
            return found ? new Utf8String.from_bytes (copy_string_view_bytes (layer_type)) : null;
        }

        public StringList list_style_layer_ids () throws Error {
            Raw.StyleIdList list;
            check_status (Raw.map_list_style_layer_ids (require_live ().native, out list));
            return copy_style_id_list ((owned) list);
        }

        public void move_style_layer (string layer_id, string before_layer_id = "") throws Error {
            move_style_layer_utf8 (new Utf8String (layer_id), new Utf8String (before_layer_id));
        }

        public void move_style_layer_utf8 (Utf8String layer_id, Utf8String? before_layer_id = null) throws Error {
            var before = before_layer_id ?? new Utf8String ("");
            check_status (Raw.map_move_style_layer (require_live ().native, layer_id.to_native (), before.to_native ()));
        }

        public JsonValue? get_style_layer_json (string layer_id) throws Error {
            return get_style_layer_json_utf8 (new Utf8String (layer_id));
        }

        public JsonValue? get_style_layer_json_utf8 (Utf8String layer_id) throws Error {
            Raw.JsonSnapshot? snapshot;
            bool found;
            check_status (Raw.map_get_style_layer_json (require_live ().native, layer_id.to_native (), out snapshot, out found));
            if (!found) {
                return copy_json_snapshot ((owned) snapshot);
            }
            return copy_json_snapshot ((owned) snapshot);
        }

        public void set_style_light_json (JsonValue light_json) throws Error {
            var light_json_storage = light_json.copy ();
            var native_light_json = light_json_storage.to_native ();
            check_status (Raw.map_set_style_light_json (require_live ().native, &native_light_json));
        }

        public void set_style_light_property (string property_name, JsonValue value) throws Error {
            set_style_light_property_utf8 (new Utf8String (property_name), value);
        }

        public void set_style_light_property_utf8 (Utf8String property_name, JsonValue value) throws Error {
            var value_storage = value.copy ();
            var native_value = value_storage.to_native ();
            check_status (Raw.map_set_style_light_property (require_live ().native, property_name.to_native (), &native_value));
        }

        public JsonValue? get_style_light_property (string property_name) throws Error {
            return get_style_light_property_utf8 (new Utf8String (property_name));
        }

        public JsonValue? get_style_light_property_utf8 (Utf8String property_name) throws Error {
            Raw.JsonSnapshot? snapshot;
            check_status (Raw.map_get_style_light_property (require_live ().native, property_name.to_native (), out snapshot));
            return copy_json_snapshot ((owned) snapshot);
        }

        public void set_layer_property (string layer_id, string property_name, JsonValue value) throws Error {
            set_layer_property_utf8 (new Utf8String (layer_id), new Utf8String (property_name), value);
        }

        public void set_layer_property_utf8 (Utf8String layer_id, Utf8String property_name, JsonValue value) throws Error {
            var value_storage = value.copy ();
            var native_value = value_storage.to_native ();
            check_status (Raw.map_set_layer_property (require_live ().native, layer_id.to_native (), property_name.to_native (), &native_value));
        }

        public JsonValue? get_layer_property (string layer_id, string property_name) throws Error {
            return get_layer_property_utf8 (new Utf8String (layer_id), new Utf8String (property_name));
        }

        public JsonValue? get_layer_property_utf8 (Utf8String layer_id, Utf8String property_name) throws Error {
            Raw.JsonSnapshot? snapshot;
            check_status (Raw.map_get_layer_property (require_live ().native, layer_id.to_native (), property_name.to_native (), out snapshot));
            return copy_json_snapshot ((owned) snapshot);
        }

        public void set_layer_filter (string layer_id, JsonValue? filter = null) throws Error {
            set_layer_filter_utf8 (new Utf8String (layer_id), filter);
        }

        public void set_layer_filter_utf8 (Utf8String layer_id, JsonValue? filter = null) throws Error {
            Raw.JsonValue native_filter = {};
            Raw.JsonValue* filter_ptr = null;
            JsonValue? filter_storage = null;
            if (filter != null) {
                filter_storage = filter.copy ();
                native_filter = filter_storage.to_native ();
                filter_ptr = &native_filter;
            }
            check_status (Raw.map_set_layer_filter (require_live ().native, layer_id.to_native (), filter_ptr));
        }

        public JsonValue? get_layer_filter (string layer_id) throws Error {
            return get_layer_filter_utf8 (new Utf8String (layer_id));
        }

        public JsonValue? get_layer_filter_utf8 (Utf8String layer_id) throws Error {
            Raw.JsonSnapshot? snapshot;
            check_status (Raw.map_get_layer_filter (require_live ().native, layer_id.to_native (), out snapshot));
            return copy_json_snapshot ((owned) snapshot);
        }

        public void set_style_image (string image_id, PremultipliedRgba8Image image, StyleImageOptions? options = null) throws Error {
            set_style_image_utf8 (new Utf8String (image_id), image, options);
        }

        public void set_style_image_utf8 (Utf8String image_id, PremultipliedRgba8Image image, StyleImageOptions? options = null) throws Error {
            var native_image = image.to_native ();
            var native_options = (options ?? new StyleImageOptions ()).to_native ();
            check_status (Raw.map_set_style_image (require_live ().native, image_id.to_native (), &native_image, &native_options));
        }

        public bool remove_style_image (string image_id) throws Error {
            return remove_style_image_utf8 (new Utf8String (image_id));
        }

        public bool remove_style_image_utf8 (Utf8String image_id) throws Error {
            bool removed;
            check_status (Raw.map_remove_style_image (require_live ().native, image_id.to_native (), out removed));
            return removed;
        }

        public bool style_image_exists (string image_id) throws Error {
            return style_image_exists_utf8 (new Utf8String (image_id));
        }

        public bool style_image_exists_utf8 (Utf8String image_id) throws Error {
            bool exists;
            check_status (Raw.map_style_image_exists (require_live ().native, image_id.to_native (), out exists));
            return exists;
        }

        public StyleImageInfo? get_style_image_info (string image_id) throws Error {
            return get_style_image_info_utf8 (new Utf8String (image_id));
        }

        public StyleImageInfo? get_style_image_info_utf8 (Utf8String image_id) throws Error {
            Raw.StyleImageInfo info = Raw.style_image_info_default ();
            bool found;
            check_status (Raw.map_get_style_image_info (require_live ().native, image_id.to_native (), &info, out found));
            return found ? new StyleImageInfo (info) : null;
        }

        public uint8[]? copy_style_image_premultiplied_rgba8 (string image_id) throws Error {
            return copy_style_image_premultiplied_rgba8_utf8 (new Utf8String (image_id));
        }

        public uint8[]? copy_style_image_premultiplied_rgba8_utf8 (Utf8String image_id) throws Error {
            var info = get_style_image_info_utf8 (image_id);
            if (info == null) {
                return null;
            }
            uint8[] pixels = new uint8[info.byte_length];
            size_t byte_length;
            bool found;
            check_status (Raw.map_copy_style_image_premultiplied_rgba8 (require_live ().native, image_id.to_native (), pixels, pixels.length, out byte_length, out found));
            return found ? pixels : null;
        }

        public void add_image_source_url (string source_id, LatLng[] coordinates, string url) throws Error {
            add_image_source_url_utf8 (new Utf8String (source_id), coordinates, new Utf8String (url));
        }

        public void add_image_source_url_utf8 (Utf8String source_id, LatLng[] coordinates, Utf8String url) throws Error {
            Raw.LatLng[] native_coordinates = image_source_coordinates_to_native (coordinates);
            check_status (Raw.map_add_image_source_url (require_live ().native, source_id.to_native (), native_coordinates, native_coordinates.length, url.to_native ()));
        }

        public void add_image_source_image (string source_id, LatLng[] coordinates, PremultipliedRgba8Image image) throws Error {
            add_image_source_image_utf8 (new Utf8String (source_id), coordinates, image);
        }

        public void add_image_source_image_utf8 (Utf8String source_id, LatLng[] coordinates, PremultipliedRgba8Image image) throws Error {
            Raw.LatLng[] native_coordinates = image_source_coordinates_to_native (coordinates);
            var native_image = image.to_native ();
            check_status (Raw.map_add_image_source_image (require_live ().native, source_id.to_native (), native_coordinates, native_coordinates.length, &native_image));
        }

        public void set_image_source_url (string source_id, string url) throws Error {
            set_image_source_url_utf8 (new Utf8String (source_id), new Utf8String (url));
        }

        public void set_image_source_url_utf8 (Utf8String source_id, Utf8String url) throws Error {
            check_status (Raw.map_set_image_source_url (require_live ().native, source_id.to_native (), url.to_native ()));
        }

        public void set_image_source_image (string source_id, PremultipliedRgba8Image image) throws Error {
            set_image_source_image_utf8 (new Utf8String (source_id), image);
        }

        public void set_image_source_image_utf8 (Utf8String source_id, PremultipliedRgba8Image image) throws Error {
            var native_image = image.to_native ();
            check_status (Raw.map_set_image_source_image (require_live ().native, source_id.to_native (), &native_image));
        }

        public void set_image_source_coordinates (string source_id, LatLng[] coordinates) throws Error {
            set_image_source_coordinates_utf8 (new Utf8String (source_id), coordinates);
        }

        public void set_image_source_coordinates_utf8 (Utf8String source_id, LatLng[] coordinates) throws Error {
            Raw.LatLng[] native_coordinates = image_source_coordinates_to_native (coordinates);
            check_status (Raw.map_set_image_source_coordinates (require_live ().native, source_id.to_native (), native_coordinates, native_coordinates.length));
        }

        public LatLng[]? get_image_source_coordinates (string source_id) throws Error {
            return get_image_source_coordinates_utf8 (new Utf8String (source_id));
        }

        public LatLng[]? get_image_source_coordinates_utf8 (Utf8String source_id) throws Error {
            Raw.LatLng[] native_coordinates = new Raw.LatLng[4];
            size_t coordinate_count;
            bool found;
            check_status (Raw.map_get_image_source_coordinates (require_live ().native, source_id.to_native (), native_coordinates, native_coordinates.length, out coordinate_count, out found));
            if (!found) {
                return null;
            }
            LatLng[] coordinates = new LatLng[coordinate_count];
            for (size_t index = 0; index < coordinate_count; index++) {
                coordinates[index] = LatLng.from_native (native_coordinates[index]);
            }
            return coordinates;
        }
    }
}
