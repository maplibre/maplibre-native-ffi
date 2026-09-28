// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

#[derive(Debug)]
pub(crate) struct MapProjectionHandleState {
    pub(crate) handle: crate::handle::ConcurrentNativeHandle<sys::mln_map_projection>,
    id: u64,
}
impl MapProjectionHandleState {
    pub(crate) fn native(&self) -> Result<sys::mln_map_projection> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("MapProjectionHandle"))
    }
}
impl Drop for MapProjectionHandleState {
    fn drop(&mut self) {
        self.handle
            .finalize_with(|raw| unsafe { maplibre_core::generated::map_projection_dispose(raw) });
    }
}
/// Owns one `mln_map_projection` native handle.
pub struct MapProjectionHandle {
    pub(crate) inner: std::sync::Arc<MapProjectionHandleState>,
}
impl std::fmt::Debug for MapProjectionHandle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("MapProjectionHandle")
            .field("closed", &self.is_closed())
            .finish()
    }
}
impl MapProjectionHandle {
    pub(crate) fn from_native(raw: sys::mln_map_projection) -> Result<Self> {
        // SAFETY: raw came from an accepted ownership transfer of this handle type.
        let handle = unsafe {
            crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_map_projection")
        }?;
        Ok(Self {
            inner: std::sync::Arc::new(MapProjectionHandleState { handle, id: raw.0 }),
        })
    }

    /// Returns the native handle value, which event sources report for this handle.
    pub fn id(&self) -> u64 {
        self.inner.id
    }

    /// Reports whether an explicit release, close, or disposal consumed this handle.
    pub fn is_closed(&self) -> bool {
        self.inner.handle.is_closed()
    }
}

impl MapProjectionHandle {
    /// Calls `mln_map_projection_close` using its header execution and ownership contract.
    pub fn close(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            maplibre_core::check(unsafe { sys::mln_map_projection_close(native) })?;
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }

    /// Calls `mln_map_projection_get_camera` using its header execution and ownership contract.
    pub fn get_camera(&self) -> Result<maplibre_core::generated::CameraOptions> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_projection_get_camera", native.0)?;
        let mut binding_arg_1: sys::mln_camera_options =
            maplibre_core::generated::CameraOptions::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_map_projection_get_camera(native, &mut binding_arg_1)
        })?;
        Ok(maplibre_core::generated::CameraOptions::from_native(
            binding_arg_1,
        ))
    }

    /// Calls `mln_map_projection_lat_lng_for_pixel` using its header execution and ownership contract.
    pub fn lat_lng_for_pixel(
        &self,
        binding_arg_1: maplibre_core::generated::ScreenPoint,
    ) -> Result<maplibre_core::generated::LatLng> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_projection_lat_lng_for_pixel", native.0)?;
        let mut binding_arg_2: sys::mln_lat_lng =
            maplibre_core::generated::LatLng::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_map_projection_lat_lng_for_pixel(
                native,
                binding_arg_1.to_native(),
                &mut binding_arg_2,
            )
        })?;
        Ok(maplibre_core::generated::LatLng::from_native(binding_arg_2))
    }

    /// Calls `mln_map_projection_lat_lng_for_pixel_unwrapped` using its header execution and ownership contract.
    pub fn lat_lng_for_pixel_unwrapped(
        &self,
        binding_arg_1: maplibre_core::generated::ScreenPoint,
    ) -> Result<maplibre_core::generated::LatLng> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_projection_lat_lng_for_pixel_unwrapped", native.0)?;
        let mut binding_arg_2: sys::mln_lat_lng =
            maplibre_core::generated::LatLng::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_map_projection_lat_lng_for_pixel_unwrapped(
                native,
                binding_arg_1.to_native(),
                &mut binding_arg_2,
            )
        })?;
        Ok(maplibre_core::generated::LatLng::from_native(binding_arg_2))
    }

    /// Calls `mln_map_projection_meters_per_pixel_at_latitude` using its header execution and ownership contract.
    pub fn meters_per_pixel_at_latitude(&self, binding_arg_1: f64) -> Result<f64> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check(
            "mln_map_projection_meters_per_pixel_at_latitude",
            native.0,
        )?;
        let mut binding_arg_2: f64 = Default::default();
        maplibre_core::check(unsafe {
            sys::mln_map_projection_meters_per_pixel_at_latitude(
                native,
                binding_arg_1,
                &mut binding_arg_2,
            )
        })?;
        Ok(binding_arg_2)
    }

    /// Calls `mln_map_projection_pixel_for_lat_lng` using its header execution and ownership contract.
    pub fn pixel_for_lat_lng(
        &self,
        binding_arg_1: maplibre_core::generated::LatLng,
    ) -> Result<maplibre_core::generated::ScreenPoint> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_projection_pixel_for_lat_lng", native.0)?;
        let mut binding_arg_2: sys::mln_screen_point =
            maplibre_core::generated::ScreenPoint::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_map_projection_pixel_for_lat_lng(
                native,
                binding_arg_1.to_native(),
                &mut binding_arg_2,
            )
        })?;
        Ok(maplibre_core::generated::ScreenPoint::from_native(
            binding_arg_2,
        ))
    }

    /// Calls `mln_map_projection_set_camera` using its header execution and ownership contract.
    pub fn set_camera(
        &self,
        binding_arg_1: &maplibre_core::generated::CameraOptions,
    ) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_projection_set_camera", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_camera(native, &binding_arg_1)
        })?;
        Ok(())
    }

    /// Calls `mln_map_projection_set_visible_coordinates` using its header execution and ownership contract.
    pub fn set_visible_coordinates(
        &self,
        binding_arg_1: &[maplibre_core::generated::LatLng],
        binding_arg_3: maplibre_core::generated::EdgeInsets,
    ) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_projection_set_visible_coordinates", native.0)?;
        let binding_arg_1: Vec<_> = binding_arg_1
            .iter()
            .map(|value| -> Result<_> { Ok((value).to_native()) })
            .collect::<Result<_>>()?;
        let binding_arg_2 = binding_arg_1
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_coordinates(
                native,
                binding_arg_1.as_ptr(),
                binding_arg_2,
                binding_arg_3.to_native(),
            )
        })?;
        Ok(())
    }

    /// Calls `mln_map_projection_set_visible_geometry` using its header execution and ownership contract.
    pub fn set_visible_geometry(
        &self,
        binding_arg_1: &[u8],
        binding_arg_2: maplibre_core::generated::EdgeInsets,
    ) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_projection_set_visible_geometry", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_ptr().cast(),
            size: (binding_arg_1).len(),
        };
        maplibre_core::check(unsafe {
            sys::mln_map_projection_set_visible_geometry(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
            )
        })?;
        Ok(())
    }
}
