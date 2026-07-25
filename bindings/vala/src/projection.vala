namespace MaplibreNative {
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
        private Raw.MapProjection? native;

        public bool closed { get { return native == null; } }

        internal MapProjectionHandle (owned Raw.MapProjection native) {
            this.native = (owned) native;
        }

        ~MapProjectionHandle () {
            if (native != null) {
                warning ("MapProjectionHandle finalized while live; call close() on the owner thread");
            }
        }

        internal unowned Raw.MapProjection require_live () throws Error {
            if (native == null) {
                throw new Error.INVALID_STATE ("map projection handle is closed");
            }
            return native;
        }

        public void close () throws Error {
            if (native == null) {
                return;
            }
            unowned Raw.MapProjection closing = native;
            check_status (Raw.map_projection_destroy (closing));
            native = null;
        }

        public CameraOptions get_camera () throws Error {
            Raw.CameraOptions native_camera = {};
            native_camera.size = (uint32) sizeof (Raw.CameraOptions);
            check_status (Raw.map_projection_get_camera (require_live (), &native_camera));
            return CameraOptions.from_native (native_camera);
        }

        public void set_camera (CameraOptions camera) throws Error {
            var native_camera = camera.to_native ();
            check_status (Raw.map_projection_set_camera (require_live (), &native_camera));
        }

        public ScreenPoint pixel_for_lat_lng (LatLng coordinate) throws Error {
            Raw.ScreenPoint point;
            check_status (Raw.map_projection_pixel_for_lat_lng (require_live (), coordinate.to_native (), out point));
            return ScreenPoint.from_native (point);
        }

        public LatLng lat_lng_for_pixel (ScreenPoint point) throws Error {
            Raw.LatLng coordinate;
            check_status (Raw.map_projection_lat_lng_for_pixel (require_live (), point.to_native (), out coordinate));
            return LatLng.from_native (coordinate);
        }

        public void set_visible_coordinates (LatLng[] coordinates, EdgeInsets padding) throws Error {
            if (coordinates.length == 0) {
                throw new Error.INVALID_ARGUMENT ("visible coordinates are empty");
            }
            Raw.LatLng[] native_coordinates = new Raw.LatLng[coordinates.length];
            for (var i = 0; i < coordinates.length; i++) {
                native_coordinates[i] = coordinates[i].to_native ();
            }
            check_status (Raw.map_projection_set_visible_coordinates (require_live (), native_coordinates, native_coordinates.length, padding.to_native ()));
        }

        public void set_visible_geometry (Geometry geometry, EdgeInsets padding) throws Error {
            var native_geometry = geometry.to_native ();
            check_status (Raw.map_projection_set_visible_geometry (require_live (), &native_geometry, padding.to_native ()));
        }
    }
}
