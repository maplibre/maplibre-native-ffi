namespace MaplibreNative {
    internal class MapProjectionNativeLease {
        private MapProjectionHandle owner;
        public Raw.MapProjection native { get; private set; }

        internal MapProjectionNativeLease (MapProjectionHandle owner, Raw.MapProjection native) {
            this.owner = owner;
            this.native = native;
        }

        ~MapProjectionNativeLease () {
            owner.release_native_lease ();
        }
    }

    public ProjectedMeters projected_meters_for_lat_lng (LatLng coordinate) throws Error {
        Raw.ProjectedMeters meters;
        check_status (Raw.projected_meters_for_lat_lng (coordinate.to_native (), out meters));
        return ProjectedMeters.from_native (meters);
    }

    public LatLng lat_lng_for_projected_meters (ProjectedMeters meters) throws Error {
        Raw.LatLng coordinate;
        check_status (Raw.lat_lng_for_projected_meters (meters.to_native (), out coordinate));
        return LatLng.from_native (coordinate);
    }

    public class MapProjectionHandle {
        private Raw.MapProjection native = (Raw.MapProjection) 0;
        private Mutex state_mutex;
        private Cond idle;
        private bool releasing;
        private uint active_native_leases;
        private unowned Thread<void*> owner_thread;

        public bool closed {
            get {
                state_mutex.lock ();
                var value = (uint64) native == 0;
                state_mutex.unlock ();
                return value;
            }
        }

        internal MapProjectionHandle (Raw.MapProjection native) {
            this.native = native;
            owner_thread = Thread.self<void*> ();
        }

        ~MapProjectionHandle () {
            state_mutex.lock ();
            var leaked = (uint64) native != 0;
            state_mutex.unlock ();
            if (leaked) {
                warning ("MapProjectionHandle finalized while live; call close() on the owner thread");
            }
        }

        internal MapProjectionNativeLease require_live () throws Error {
            state_mutex.lock ();
            if ((uint64) native == 0 || releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("map projection handle is closed");
            }
            active_native_leases++;
            var lease = new MapProjectionNativeLease (this, native);
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

        public void close () throws Error {
            ensure_owner_thread ();
            state_mutex.lock ();
            if ((uint64) native == 0) {
                state_mutex.unlock ();
                return;
            }
            if (releasing) {
                state_mutex.unlock ();
                clear_unknown_status ();
                throw new Error.INVALID_STATE ("map projection release is already in progress");
            }
            releasing = true;
            while (active_native_leases > 0) {
                idle.wait (state_mutex);
            }
            Raw.MapProjection closing = native;
            state_mutex.unlock ();

            var status = Raw.map_projection_destroy (closing);

            state_mutex.lock ();
            if (status == Raw.Status.OK) {
                native = (Raw.MapProjection) 0;
            }
            releasing = false;
            idle.broadcast ();
            state_mutex.unlock ();
            check_status (status);
        }

        private void ensure_owner_thread () throws Error {
            if (Thread.self<void*> () != owner_thread) {
                clear_unknown_status ();
                throw new Error.WRONG_THREAD ("map projection called from a thread other than its owner thread");
            }
        }

        public CameraOptions get_camera () throws Error {
            Raw.CameraOptions native_camera = {};
            native_camera.size = (uint32) sizeof (Raw.CameraOptions);
            var lease = require_live ();
            check_status (Raw.map_projection_get_camera (lease.native, &native_camera));
            return CameraOptions.from_native (native_camera);
        }

        public void set_camera (CameraOptions camera) throws Error {
            var native_camera = camera.to_native ();
            var lease = require_live ();
            check_status (Raw.map_projection_set_camera (lease.native, &native_camera));
        }

        public ScreenPoint pixel_for_lat_lng (LatLng coordinate) throws Error {
            Raw.ScreenPoint point;
            var lease = require_live ();
            check_status (Raw.map_projection_pixel_for_lat_lng (lease.native, coordinate.to_native (), out point));
            return ScreenPoint.from_native (point);
        }

        public LatLng lat_lng_for_pixel (ScreenPoint point) throws Error {
            Raw.LatLng coordinate;
            var lease = require_live ();
            check_status (Raw.map_projection_lat_lng_for_pixel (lease.native, point.to_native (), out coordinate));
            return LatLng.from_native (coordinate);
        }

        public void set_visible_coordinates (LatLng[] coordinates, EdgeInsets padding) throws Error {
            if (coordinates.length == 0) {
                clear_unknown_status ();
                throw new Error.INVALID_ARGUMENT ("visible coordinates are empty");
            }
            Raw.LatLng[] native_coordinates = new Raw.LatLng[coordinates.length];
            for (var i = 0; i < coordinates.length; i++) {
                native_coordinates[i] = coordinates[i].to_native ();
            }
            var lease = require_live ();
            check_status (Raw.map_projection_set_visible_coordinates (lease.native, native_coordinates, native_coordinates.length, padding.to_native ()));
        }

        public void set_visible_geometry (Geometry geometry, EdgeInsets padding) throws Error {
            var geometry_storage = geometry.copy ();
            var native_geometry = geometry_storage.to_native ();
            var lease = require_live ();
            check_status (Raw.map_projection_set_visible_geometry (lease.native, &native_geometry, padding.to_native ()));
        }
    }
}
