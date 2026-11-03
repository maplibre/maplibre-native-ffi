// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A camera view for converting between screen points and coordinates.
    ///
    /// See `mln_map_projection` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct MapProjectionHandle(mln_map_projection) dispose |raw| { unsafe { sys::mln_map_projection_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl MapProjectionHandle {
    /// Creates a projection that converts coordinates for one view of a map.
    ///
    /// Create it from the `view` of an `MapSnapshot` or an `RenderFrameResult`.
    /// The projection doesn't follow the map, so create a new one after the
    /// map's camera or size changes.
    ///
    /// See `mln_map_projection_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn create(view: &MapView) -> Result<MapProjectionHandle> {
        let mut call = Call::global();
        crate::abi::validate_abi_version()?;
        let mut out_projection = sys::mln_map_projection(0);
        let view = call.reference(&view)?;
        call.status(|_, out_diagnostic| unsafe {
            sys::mln_map_projection_create(view, &mut out_projection, out_diagnostic)
        })?;
        Ok(MapProjectionHandle::adopt(out_projection)?)
    }

    /// Reads the projection's camera.
    ///
    /// See `mln_map_projection_get_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn get_camera(&self) -> Result<MapProjectionCameraResult> {
        let mut call = self.inner.call();
        let mut out_result: sys::mln_map_projection_camera_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_map_projection_camera_result>() as _;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_get_camera(projection, &mut out_result, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_result) }?)
    }

    /// Finds the coordinate at a screen point.
    ///
    /// The longitude stays between -180 and 180.
    ///
    /// See `mln_map_projection_lat_lng_for_pixel` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<LatLng> {
        let mut call = self.inner.call();
        let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
        let point = call.input(&point)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_lat_lng_for_pixel(
                projection,
                point,
                &mut out_coordinate,
                out_diagnostic,
            )
        })?;
        Ok(unsafe { from_native(out_coordinate) }?)
    }

    /// Finds the coordinate at a screen point, without wrapping its longitude.
    ///
    /// The longitude can fall outside -180 to 180, to show which copy of the
    /// world the point is in.
    ///
    /// See `mln_map_projection_lat_lng_for_pixel_unwrapped` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn lat_lng_for_pixel_unwrapped(&self, point: ScreenPoint) -> Result<LatLng> {
        let mut call = self.inner.call();
        let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
        let point = call.input(&point)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_lat_lng_for_pixel_unwrapped(
                projection,
                point,
                &mut out_coordinate,
                out_diagnostic,
            )
        })?;
        Ok(unsafe { from_native(out_coordinate) }?)
    }

    /// Finds the coordinates at several screen points, as
    /// `MapProjectionHandle::lat_lng_for_pixel()` does.
    ///
    /// See `mln_map_projection_lat_lngs_for_pixels` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn lat_lngs_for_pixels(&self, points: &[ScreenPoint]) -> Result<Vec<LatLng>> {
        let mut call = self.inner.call();
        let point_count = convert::count(points.len())?;
        let mut out_coordinates: Vec<sys::mln_lat_lng> = (0..points.len())
            .map(|_| unsafe { std::mem::zeroed() })
            .collect();
        let points = call.array(points)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_lat_lngs_for_pixels(
                projection,
                points,
                point_count,
                out_coordinates.as_mut_ptr(),
                out_diagnostic,
            )
        })?;
        Ok(out_coordinates
            .into_iter()
            .map(|item| Ok(unsafe { from_native(item) }?))
            .collect::<Result<Vec<_>>>()?)
    }

    /// Finds the coordinates at several screen points, as
    /// `MapProjectionHandle::lat_lng_for_pixel_unwrapped()` does.
    ///
    /// See `mln_map_projection_lat_lngs_for_pixels_unwrapped` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn lat_lngs_for_pixels_unwrapped(&self, points: &[ScreenPoint]) -> Result<Vec<LatLng>> {
        let mut call = self.inner.call();
        let point_count = convert::count(points.len())?;
        let mut out_coordinates: Vec<sys::mln_lat_lng> = (0..points.len())
            .map(|_| unsafe { std::mem::zeroed() })
            .collect();
        let points = call.array(points)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_lat_lngs_for_pixels_unwrapped(
                projection,
                points,
                point_count,
                out_coordinates.as_mut_ptr(),
                out_diagnostic,
            )
        })?;
        Ok(out_coordinates
            .into_iter()
            .map(|item| Ok(unsafe { from_native(item) }?))
            .collect::<Result<Vec<_>>>()?)
    }

    /// Computes how many meters one screen pixel covers at `latitude`, at the
    /// projection's zoom.
    ///
    /// The result ignores the camera's pitch.
    ///
    /// See `mln_map_projection_meters_per_pixel_at_latitude` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn meters_per_pixel_at_latitude(&self, latitude: f64) -> Result<f64> {
        let mut call = self.inner.call();
        let mut out_meters_per_pixel: f64 = Default::default();
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_meters_per_pixel_at_latitude(
                projection,
                latitude,
                &mut out_meters_per_pixel,
                out_diagnostic,
            )
        })?;
        Ok(out_meters_per_pixel)
    }

    /// Finds the screen point of a coordinate.
    ///
    /// The point is in the copy of the world nearest the camera's center.
    ///
    /// See `mln_map_projection_pixel_for_lat_lng` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        let mut call = self.inner.call();
        let mut out_point: sys::mln_screen_point = unsafe { std::mem::zeroed() };
        let coordinate = call.input(&coordinate)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_pixel_for_lat_lng(
                projection,
                coordinate,
                &mut out_point,
                out_diagnostic,
            )
        })?;
        Ok(unsafe { from_native(out_point) }?)
    }

    /// Finds the screen points of several coordinates, as
    /// `MapProjectionHandle::pixel_for_lat_lng()` does.
    ///
    /// See `mln_map_projection_pixels_for_lat_lngs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn pixels_for_lat_lngs(&self, coordinates: &[LatLng]) -> Result<Vec<ScreenPoint>> {
        let mut call = self.inner.call();
        let coordinate_count = convert::count(coordinates.len())?;
        let mut out_points: Vec<sys::mln_screen_point> = (0..coordinates.len())
            .map(|_| unsafe { std::mem::zeroed() })
            .collect();
        let coordinates = call.array(coordinates)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_pixels_for_lat_lngs(
                projection,
                coordinates,
                coordinate_count,
                out_points.as_mut_ptr(),
                out_diagnostic,
            )
        })?;
        Ok(out_points
            .into_iter()
            .map(|item| Ok(unsafe { from_native(item) }?))
            .collect::<Result<Vec<_>>>()?)
    }

    /// Releases a projection.
    ///
    /// See `mln_map_projection_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|projection| {
            let mut call = Call::new(projection);
            call.run(|projection| unsafe { sys::mln_map_projection_release(projection) });
            Ok(())
        })
    }

    /// Moves the projection's camera, without changing the map.
    ///
    /// Camera fields that `camera` doesn't set keep their values.
    ///
    /// See `mln_map_projection_set_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn set_camera(&self, camera: &CameraOptions) -> Result<()> {
        let mut call = self.inner.call();
        let camera = call.reference(&camera)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_set_camera(projection, camera, out_diagnostic)
        })?;
        Ok(())
    }
}
