// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_map_projection` native handle.
    pub struct MapProjectionHandle(mln_map_projection) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_map_projection_close(raw, out_diagnostic) });
}

impl MapProjectionHandle {
    /// Calls `mln_map_projection_close`.
    pub fn close(&self) -> Result<()> {
        self.inner.close(|projection| {
            let mut call = Call::new(projection, None);
            call.status(|projection, out_diagnostic| unsafe {
                sys::mln_map_projection_close(projection, out_diagnostic)
            })
        })
    }

    /// Calls `mln_map_projection_get_camera`.
    pub fn get_camera(&self) -> Result<CameraOptions> {
        let mut call = self.inner.call("mln_map_projection_get_camera")?;
        let mut out_camera: sys::mln_camera_options = unsafe { sys::mln_camera_options_default() };
        out_camera.size = std::mem::size_of::<sys::mln_camera_options>() as _;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_get_camera(projection, &mut out_camera, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_camera) }?)
    }

    /// Calls `mln_map_projection_lat_lng_for_pixel`.
    pub fn lat_lng_for_pixel(&self, point: ScreenPoint) -> Result<LatLng> {
        let mut call = self.inner.call("mln_map_projection_lat_lng_for_pixel")?;
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

    /// Calls `mln_map_projection_lat_lng_for_pixel_unwrapped`.
    pub fn lat_lng_for_pixel_unwrapped(&self, point: ScreenPoint) -> Result<LatLng> {
        let mut call = self
            .inner
            .call("mln_map_projection_lat_lng_for_pixel_unwrapped")?;
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

    /// Calls `mln_map_projection_meters_per_pixel_at_latitude`.
    pub fn meters_per_pixel_at_latitude(&self, latitude: f64) -> Result<f64> {
        let mut call = self
            .inner
            .call("mln_map_projection_meters_per_pixel_at_latitude")?;
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

    /// Calls `mln_map_projection_pixel_for_lat_lng`.
    pub fn pixel_for_lat_lng(&self, coordinate: LatLng) -> Result<ScreenPoint> {
        let mut call = self.inner.call("mln_map_projection_pixel_for_lat_lng")?;
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

    /// Calls `mln_map_projection_set_camera`.
    pub fn set_camera(&self, camera: &CameraOptions) -> Result<()> {
        let mut call = self.inner.call("mln_map_projection_set_camera")?;
        let camera = call.reference(&camera)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_set_camera(projection, camera, out_diagnostic)
        })?;
        Ok(())
    }

    /// Calls `mln_map_projection_set_visible_coordinates`.
    pub fn set_visible_coordinates(
        &self,
        coordinates: &[LatLng],
        padding: EdgeInsets,
    ) -> Result<()> {
        let mut call = self
            .inner
            .call("mln_map_projection_set_visible_coordinates")?;
        let coordinate_count = convert::count(coordinates.len())?;
        let coordinates = call.array(coordinates)?;
        let padding = call.input(&padding)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_set_visible_coordinates(
                projection,
                coordinates,
                coordinate_count,
                padding,
                out_diagnostic,
            )
        })?;
        Ok(())
    }

    /// Calls `mln_map_projection_set_visible_geometry`.
    pub fn set_visible_geometry(&self, geometry: &[u8], padding: EdgeInsets) -> Result<()> {
        let mut call = self.inner.call("mln_map_projection_set_visible_geometry")?;
        let geometry = call.input(&geometry)?;
        let padding = call.input(&padding)?;
        call.status(|projection, out_diagnostic| unsafe {
            sys::mln_map_projection_set_visible_geometry(
                projection,
                geometry,
                padding,
                out_diagnostic,
            )
        })?;
        Ok(())
    }
}
