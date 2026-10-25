// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_map_projection` native handle.
    ///
    /// A standalone projection of one map view.
    ///
    /// See `mln_map_projection` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct MapProjectionHandle(mln_map_projection) dispose |raw| { unsafe { sys::mln_map_projection_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl MapProjectionHandle {
    /// Copies the projection camera into out_camera.
    ///
    /// See `mln_map_projection_get_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn get_camera(&self) -> Result<CameraOptions> {
        let mut call = self.inner.call();
        let mut out_camera: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
        out_camera.size = std::mem::size_of::<sys::mln_camera_options>() as _;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_get_camera(projection, &mut out_camera, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_camera) }?)
    }

    /// Converts a screen point to a geographic coordinate.
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

    /// Converts a screen point to an unwrapped geographic coordinate.
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

    /// Converts screen points to geographic coordinates.
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

    /// Converts screen points to unwrapped geographic coordinates.
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

    /// Reads the ground distance covered by one logical map pixel at a latitude
    /// for the helper camera zoom.
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

    /// Converts a geographic coordinate to a screen point.
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

    /// Converts geographic coordinates to screen points.
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

    /// Releases a standalone projection.
    ///
    /// See `mln_map_projection_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.inner
            .close(|projection| {
                let mut call = Call::new(projection);
                call.run(|projection| unsafe { sys::mln_map_projection_release(projection) });
                Ok(())
            })
            .map_err(|error| (self, error))
    }

    /// Applies a camera update to a standalone projection.
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
