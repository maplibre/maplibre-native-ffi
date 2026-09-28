// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

#[derive(Debug)]
pub(crate) struct MapHandleState {
    pub(crate) handle: crate::handle::ConcurrentNativeHandle<sys::mln_map>,
    id: u64,
    _parent: std::sync::Arc<crate::RuntimeHandleState>,
}
impl MapHandleState {
    pub(crate) fn native(&self) -> Result<sys::mln_map> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("MapHandle"))
    }
}
impl Drop for MapHandleState {
    fn drop(&mut self) {
        self.handle
            .finalize_with(|raw| unsafe { maplibre_core::generated::map_dispose(raw) });
    }
}
/// Owns one `mln_map` native handle.
pub struct MapHandle {
    pub(crate) inner: std::sync::Arc<MapHandleState>,
}
impl std::fmt::Debug for MapHandle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("MapHandle")
            .field("closed", &self.is_closed())
            .finish()
    }
}
impl MapHandle {
    pub(crate) fn from_native(
        raw: sys::mln_map,
        parent: std::sync::Arc<crate::RuntimeHandleState>,
    ) -> Result<Self> {
        // SAFETY: raw came from an accepted ownership transfer of this handle type.
        let handle = unsafe { crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_map") }?;
        Ok(Self {
            inner: std::sync::Arc::new(MapHandleState {
                handle,
                id: raw.0,
                _parent: parent,
            }),
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

impl MapHandle {
    /// Calls `mln_map_add_color_relief_layer` using its header execution and ownership contract.
    pub fn add_color_relief_layer(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
        binding_arg_3: Option<&str>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_color_relief_layer", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        let binding_arg_3 = match (binding_arg_3).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_color_relief_layer(
                native,
                binding_arg_1,
                binding_arg_2,
                binding_arg_3,
                completion,
            )
        })
    }

    /// Calls `mln_map_add_custom_geometry_source` using its header execution and ownership contract.
    pub fn add_custom_geometry_source(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::CustomGeometrySourceOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_custom_geometry_source", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let submitted = crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_custom_geometry_source(
                native,
                binding_arg_1,
                &binding_arg_2,
                completion,
            )
        });
        if submitted.is_ok() {
            arena.accept_registrations();
        }
        submitted
    }

    /// Calls `mln_map_add_custom_mvt_vector_source` using its header execution and ownership contract.
    pub fn add_custom_mvt_vector_source(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::CustomMvtVectorSourceOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_custom_mvt_vector_source", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let submitted = crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_custom_mvt_vector_source(
                native,
                binding_arg_1,
                &binding_arg_2,
                completion,
            )
        });
        if submitted.is_ok() {
            arena.accept_registrations();
        }
        submitted
    }

    /// Calls `mln_map_add_geojson_source_data` using its header execution and ownership contract.
    pub fn add_geojson_source_data(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &crate::GeojsonSourceDataHandle,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_geojson_source_data", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2_native = binding_arg_2.inner.native()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_geojson_source_data(
                native,
                binding_arg_1,
                binding_arg_2_native,
                completion,
            )
        })
    }

    /// Calls `mln_map_add_geojson_source_url` using its header execution and ownership contract.
    pub fn add_geojson_source_url(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
        binding_arg_3: Option<&maplibre_core::generated::GeojsonSourceOptions>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_geojson_source_url", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        let binding_arg_3 = binding_arg_3
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_geojson_source_url(
                native,
                binding_arg_1,
                binding_arg_2,
                binding_arg_3
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                completion,
            )
        })
    }

    /// Calls `mln_map_add_hillshade_layer` using its header execution and ownership contract.
    pub fn add_hillshade_layer(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
        binding_arg_3: Option<&str>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_hillshade_layer", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        let binding_arg_3 = match (binding_arg_3).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_hillshade_layer(
                native,
                binding_arg_1,
                binding_arg_2,
                binding_arg_3,
                completion,
            )
        })
    }

    /// Calls `mln_map_add_image_source_image` using its header execution and ownership contract.
    pub fn add_image_source_image(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[maplibre_core::generated::LatLng],
        binding_arg_4: &maplibre_core::generated::PremultipliedRgba8Image,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_image_source_image", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2: Vec<_> = binding_arg_2
            .iter()
            .map(|value| -> Result<_> { Ok((value).to_native()) })
            .collect::<Result<_>>()?;
        let binding_arg_3 = binding_arg_2
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        let binding_arg_4 = binding_arg_4.to_native(&mut arena)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_image_source_image(
                native,
                binding_arg_1,
                binding_arg_2.as_ptr(),
                binding_arg_3,
                &binding_arg_4,
                completion,
            )
        })
    }

    /// Calls `mln_map_add_image_source_url` using its header execution and ownership contract.
    pub fn add_image_source_url(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[maplibre_core::generated::LatLng],
        binding_arg_4: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_image_source_url", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2: Vec<_> = binding_arg_2
            .iter()
            .map(|value| -> Result<_> { Ok((value).to_native()) })
            .collect::<Result<_>>()?;
        let binding_arg_3 = binding_arg_2
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        let binding_arg_4 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_4).as_bytes().as_ptr().cast(),
            size: (binding_arg_4).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_image_source_url(
                native,
                binding_arg_1,
                binding_arg_2.as_ptr(),
                binding_arg_3,
                binding_arg_4,
                completion,
            )
        })
    }

    /// Calls `mln_map_add_location_indicator_layer` using its header execution and ownership contract.
    pub fn add_location_indicator_layer(
        &self,
        binding_arg_1: &str,
        binding_arg_2: Option<&str>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_location_indicator_layer", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = match (binding_arg_2).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_location_indicator_layer(
                native,
                binding_arg_1,
                binding_arg_2,
                completion,
            )
        })
    }

    /// Calls `mln_map_add_raster_dem_source_tiles` using its header execution and ownership contract.
    pub fn add_raster_dem_source_tiles(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[&str],
        binding_arg_4: Option<&maplibre_core::generated::StyleTileSourceOptions>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_raster_dem_source_tiles", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2: Vec<_> = binding_arg_2
            .iter()
            .map(|value| -> Result<_> {
                Ok(maplibre_native_ffi_sys::mln_buffer_view {
                    data: (value).as_bytes().as_ptr().cast(),
                    size: (value).as_bytes().len(),
                })
            })
            .collect::<Result<_>>()?;
        let binding_arg_3 = binding_arg_2
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        let binding_arg_4 = binding_arg_4
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_raster_dem_source_tiles(
                native,
                binding_arg_1,
                binding_arg_2.as_ptr(),
                binding_arg_3,
                binding_arg_4
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                completion,
            )
        })
    }

    /// Calls `mln_map_add_raster_dem_source_url` using its header execution and ownership contract.
    pub fn add_raster_dem_source_url(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
        binding_arg_3: Option<&maplibre_core::generated::StyleTileSourceOptions>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_raster_dem_source_url", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        let binding_arg_3 = binding_arg_3
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_raster_dem_source_url(
                native,
                binding_arg_1,
                binding_arg_2,
                binding_arg_3
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                completion,
            )
        })
    }

    /// Calls `mln_map_add_raster_source_tiles` using its header execution and ownership contract.
    pub fn add_raster_source_tiles(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[&str],
        binding_arg_4: Option<&maplibre_core::generated::StyleTileSourceOptions>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_raster_source_tiles", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2: Vec<_> = binding_arg_2
            .iter()
            .map(|value| -> Result<_> {
                Ok(maplibre_native_ffi_sys::mln_buffer_view {
                    data: (value).as_bytes().as_ptr().cast(),
                    size: (value).as_bytes().len(),
                })
            })
            .collect::<Result<_>>()?;
        let binding_arg_3 = binding_arg_2
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        let binding_arg_4 = binding_arg_4
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_raster_source_tiles(
                native,
                binding_arg_1,
                binding_arg_2.as_ptr(),
                binding_arg_3,
                binding_arg_4
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                completion,
            )
        })
    }

    /// Calls `mln_map_add_raster_source_url` using its header execution and ownership contract.
    pub fn add_raster_source_url(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
        binding_arg_3: Option<&maplibre_core::generated::StyleTileSourceOptions>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_raster_source_url", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        let binding_arg_3 = binding_arg_3
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_raster_source_url(
                native,
                binding_arg_1,
                binding_arg_2,
                binding_arg_3
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                completion,
            )
        })
    }

    /// Calls `mln_map_add_style_layer_json` using its header execution and ownership contract.
    pub fn add_style_layer_json(
        &self,
        binding_arg_1: &[u8],
        binding_arg_2: Option<&str>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_style_layer_json", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_ptr().cast(),
            size: (binding_arg_1).len(),
        };
        let binding_arg_2 = match (binding_arg_2).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_style_layer_json(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_add_style_source_json` using its header execution and ownership contract.
    pub fn add_style_source_json(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[u8],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_style_source_json", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_ptr().cast(),
            size: (binding_arg_2).len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_style_source_json(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_add_vector_source_tiles` using its header execution and ownership contract.
    pub fn add_vector_source_tiles(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[&str],
        binding_arg_4: Option<&maplibre_core::generated::StyleTileSourceOptions>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_vector_source_tiles", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2: Vec<_> = binding_arg_2
            .iter()
            .map(|value| -> Result<_> {
                Ok(maplibre_native_ffi_sys::mln_buffer_view {
                    data: (value).as_bytes().as_ptr().cast(),
                    size: (value).as_bytes().len(),
                })
            })
            .collect::<Result<_>>()?;
        let binding_arg_3 = binding_arg_2
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        let binding_arg_4 = binding_arg_4
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_vector_source_tiles(
                native,
                binding_arg_1,
                binding_arg_2.as_ptr(),
                binding_arg_3,
                binding_arg_4
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                completion,
            )
        })
    }

    /// Calls `mln_map_add_vector_source_url` using its header execution and ownership contract.
    pub fn add_vector_source_url(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
        binding_arg_3: Option<&maplibre_core::generated::StyleTileSourceOptions>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_add_vector_source_url", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        let binding_arg_3 = binding_arg_3
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_add_vector_source_url(
                native,
                binding_arg_1,
                binding_arg_2,
                binding_arg_3
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                completion,
            )
        })
    }

    /// Calls `mln_map_apply_camera_delta` using its header execution and ownership contract.
    pub fn apply_camera_delta(
        &self,
        binding_arg_1: &maplibre_core::generated::CameraDelta,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_apply_camera_delta", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_apply_camera_delta(native, &binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_camera_for_geometry` using its header execution and ownership contract.
    pub fn camera_for_geometry(
        &self,
        binding_arg_1: &[u8],
        binding_arg_2: Option<&maplibre_core::generated::CameraFitOptions>,
    ) -> Result<NativeFuture<maplibre_core::generated::CameraOptions>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_camera_for_geometry", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_ptr().cast(),
            size: (binding_arg_1).len(),
        };
        let binding_arg_2 = binding_arg_2.map(|value| value.to_native());
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_camera_for_geometry(
                    native,
                    binding_arg_1,
                    binding_arg_2
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_camera_options>(result)?;
                Ok(maplibre_core::generated::CameraOptions::from_native(value))
            },
        )
    }

    /// Calls `mln_map_camera_for_lat_lng_bounds` using its header execution and ownership contract.
    pub fn camera_for_lat_lng_bounds(
        &self,
        binding_arg_1: maplibre_core::generated::LatLngBounds,
        binding_arg_2: Option<&maplibre_core::generated::CameraFitOptions>,
    ) -> Result<NativeFuture<maplibre_core::generated::CameraOptions>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_camera_for_lat_lng_bounds", native.0)?;
        let binding_arg_2 = binding_arg_2.map(|value| value.to_native());
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_camera_for_lat_lng_bounds(
                    native,
                    binding_arg_1.to_native(),
                    binding_arg_2
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_camera_options>(result)?;
                Ok(maplibre_core::generated::CameraOptions::from_native(value))
            },
        )
    }

    /// Calls `mln_map_camera_for_lat_lngs` using its header execution and ownership contract.
    pub fn camera_for_lat_lngs(
        &self,
        binding_arg_1: &[maplibre_core::generated::LatLng],
        binding_arg_3: Option<&maplibre_core::generated::CameraFitOptions>,
    ) -> Result<NativeFuture<maplibre_core::generated::CameraOptions>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_camera_for_lat_lngs", native.0)?;
        let binding_arg_1: Vec<_> = binding_arg_1
            .iter()
            .map(|value| -> Result<_> { Ok((value).to_native()) })
            .collect::<Result<_>>()?;
        let binding_arg_2 = binding_arg_1
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        let binding_arg_3 = binding_arg_3.map(|value| value.to_native());
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_camera_for_lat_lngs(
                    native,
                    binding_arg_1.as_ptr(),
                    binding_arg_2,
                    binding_arg_3
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                )
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_camera_options>(result)?;
                Ok(maplibre_core::generated::CameraOptions::from_native(value))
            },
        )
    }

    /// Calls `mln_map_camera_query` using its header execution and ownership contract.
    pub fn camera_query(
        &self,
    ) -> Result<NativeFuture<maplibre_core::generated::CameraQueryResult>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_camera_query", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_camera_query(native, completion) },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_camera_query_result>(result)?;
                Ok(maplibre_core::generated::CameraQueryResult::from_native(
                    value,
                ))
            },
        )
    }

    /// Calls `mln_map_camera_snapshot_get` using its header execution and ownership contract.
    pub fn camera_snapshot_get(&self) -> Result<(maplibre_core::generated::CameraOptions, u64)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_camera_snapshot_get", native.0)?;
        let mut binding_arg_1: sys::mln_camera_options =
            maplibre_core::generated::CameraOptions::default().to_native();
        let mut binding_arg_2: u64 = Default::default();
        maplibre_core::check(unsafe {
            sys::mln_map_camera_snapshot_get(native, &mut binding_arg_1, &mut binding_arg_2)
        })?;
        Ok((
            maplibre_core::generated::CameraOptions::from_native(binding_arg_1),
            binding_arg_2,
        ))
    }

    /// Calls `mln_map_cancel_transitions` using its header execution and ownership contract.
    pub fn cancel_transitions(&self) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_cancel_transitions", native.0)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_cancel_transitions(native, completion)
        })
    }

    /// Calls `mln_map_copy_layer_source_id` using its header execution and ownership contract.
    pub fn copy_layer_source_id(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<String>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_copy_layer_source_id", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_copy_layer_source_id(native, binding_arg_1, completion)
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.size == 0 {
                    None
                } else {
                    Some(unsafe { maplibre_core::string::copy_string_view(value) }?)
                })
            },
        )
    }

    /// Calls `mln_map_copy_layer_source_layer` using its header execution and ownership contract.
    pub fn copy_layer_source_layer(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<String>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_copy_layer_source_layer", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_copy_layer_source_layer(native, binding_arg_1, completion)
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_buffer_view>(result)?;
                Ok(if value.size == 0 {
                    None
                } else {
                    Some(unsafe { maplibre_core::string::copy_string_view(value) }?)
                })
            },
        )
    }

    /// Calls `mln_map_copy_style_image_premultiplied_rgba8` using its header execution and ownership contract.
    pub fn copy_style_image_premultiplied_rgba8(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_copy_style_image_premultiplied_rgba8", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_copy_style_image_premultiplied_rgba8(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_buffer_view>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe { maplibre_core::string::copy_string_view_bytes(value) }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_copy_style_image_stretches` using its header execution and ownership contract.
    pub fn copy_style_image_stretches(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<maplibre_core::generated::StyleImageStretchesResult>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_copy_style_image_stretches", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_copy_style_image_stretches(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_style_image_stretches_result>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe {
                            maplibre_core::generated::StyleImageStretchesResult::from_native(value)
                        }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_copy_style_source_attribution` using its header execution and ownership contract.
    pub fn copy_style_source_attribution(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<String>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_copy_style_source_attribution", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_copy_style_source_attribution(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_buffer_view>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe { maplibre_core::string::copy_string_view(value) }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_copy_style_source_url` using its header execution and ownership contract.
    pub fn copy_style_source_url(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<String>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_copy_style_source_url", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_copy_style_source_url(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_buffer_view>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe { maplibre_core::string::copy_string_view(value) }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_dispose` using its header execution and ownership contract.
    pub fn dispose(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            maplibre_core::check(unsafe { sys::mln_map_dispose(native) })?;
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }

    /// Calls `mln_map_dump_debug_logs` using its header execution and ownership contract.
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_dump_debug_logs", native.0)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_dump_debug_logs(native, completion)
        })
    }

    /// Calls `mln_map_get_feature_state` using its header execution and ownership contract.
    pub fn get_feature_state(
        &self,
        binding_arg_1: &maplibre_core::generated::FeatureStateSelector,
    ) -> Result<NativeFuture<Vec<u8>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_feature_state", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_feature_state(native, &binding_arg_1, completion)
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_buffer_view>(result)?;
                Ok(unsafe { maplibre_core::string::copy_string_view_bytes(value) }?)
            },
        )
    }

    /// Calls `mln_map_get_global_state` using its header execution and ownership contract.
    pub fn get_global_state(&self) -> Result<NativeFuture<Vec<u8>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_global_state", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_get_global_state(native, completion) },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_buffer_view>(result)?;
                Ok(unsafe { maplibre_core::string::copy_string_view_bytes(value) }?)
            },
        )
    }

    /// Calls `mln_map_get_image_source_coordinates` using its header execution and ownership contract.
    pub fn get_image_source_coordinates(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<Vec<maplibre_core::generated::LatLng>>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_image_source_coordinates", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_image_source_coordinates(native, binding_arg_1, completion)
            },
            |result| {
                if result.value.is_null() {
                    return Ok(None);
                }
                (|result| {
                    crate::completion::copy_slice::<sys::mln_lat_lng>(result)?
                        .into_iter()
                        .map(|value| -> Result<_> {
                            Ok(maplibre_core::generated::LatLng::from_native(value))
                        })
                        .collect::<Result<Vec<_>>>()
                })(result)
                .map(Some)
            },
        )
    }

    /// Calls `mln_map_get_layer_filter` using its header execution and ownership contract.
    pub fn get_layer_filter(&self, binding_arg_1: &str) -> Result<NativeFuture<Option<Vec<u8>>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_layer_filter", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_layer_filter(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_buffer_view>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe { maplibre_core::string::copy_string_view_bytes(value) }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_get_layer_property` using its header execution and ownership contract.
    pub fn get_layer_property(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_layer_property", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_layer_property(native, binding_arg_1, binding_arg_2, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_buffer_view>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe { maplibre_core::string::copy_string_view_bytes(value) }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_get_style_image_info` using its header execution and ownership contract.
    pub fn get_style_image_info(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<maplibre_core::generated::StyleImageResult>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_style_image_info", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_style_image_info(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_style_image_result>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe {
                            maplibre_core::generated::StyleImageResult::from_native(value)
                        }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_get_style_layer_info` using its header execution and ownership contract.
    pub fn get_style_layer_info(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<maplibre_core::generated::StyleLayerResult>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_style_layer_info", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_style_layer_info(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_style_layer_result>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe {
                            maplibre_core::generated::StyleLayerResult::from_native(value)
                        }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_get_style_layer_json` using its header execution and ownership contract.
    pub fn get_style_layer_json(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_style_layer_json", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_style_layer_json(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_buffer_view>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe { maplibre_core::string::copy_string_view_bytes(value) }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_get_style_light_property` using its header execution and ownership contract.
    pub fn get_style_light_property(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<Vec<u8>>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_style_light_property", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_style_light_property(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_buffer_view>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe { maplibre_core::string::copy_string_view_bytes(value) }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_get_style_source_info` using its header execution and ownership contract.
    pub fn get_style_source_info(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<maplibre_core::generated::StyleSourceResult>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_style_source_info", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_style_source_info(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_style_source_result>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe {
                            maplibre_core::generated::StyleSourceResult::from_native(value)
                        }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_get_style_source_tile_urls` using its header execution and ownership contract.
    pub fn get_style_source_tile_urls(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Option<maplibre_core::generated::StyleSourceTileUrlsResult>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_style_source_tile_urls", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_get_style_source_tile_urls(native, binding_arg_1, completion)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_style_source_tile_urls_result>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe {
                            maplibre_core::generated::StyleSourceTileUrlsResult::from_native(value)
                        }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_map_get_style_transition_options` using its header execution and ownership contract.
    pub fn get_style_transition_options(
        &self,
    ) -> Result<NativeFuture<maplibre_core::generated::StyleTransitionOptions>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_get_style_transition_options", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_get_style_transition_options(native, completion) },
            |result| {
                let value =
                    crate::completion::copy_value::<sys::mln_style_transition_options>(result)?;
                Ok(maplibre_core::generated::StyleTransitionOptions::from_native(value))
            },
        )
    }

    /// Calls `mln_map_invalidate_custom_geometry_source_region` using its header execution and ownership contract.
    pub fn invalidate_custom_geometry_source_region(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::LatLngBounds,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check(
            "mln_map_invalidate_custom_geometry_source_region",
            native.0,
        )?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
                completion,
            )
        })
    }

    /// Calls `mln_map_invalidate_custom_geometry_source_tile` using its header execution and ownership contract.
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::CanonicalTileId,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_invalidate_custom_geometry_source_tile", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
                completion,
            )
        })
    }

    /// Calls `mln_map_invalidate_custom_mvt_vector_source_tile` using its header execution and ownership contract.
    pub fn invalidate_custom_mvt_vector_source_tile(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::CanonicalTileId,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check(
            "mln_map_invalidate_custom_mvt_vector_source_tile",
            native.0,
        )?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_invalidate_custom_mvt_vector_source_tile(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
                completion,
            )
        })
    }

    /// Calls `mln_map_lat_lng_bounds_for_camera` using its header execution and ownership contract.
    pub fn lat_lng_bounds_for_camera(
        &self,
        binding_arg_1: &maplibre_core::generated::CameraOptions,
    ) -> Result<NativeFuture<maplibre_core::generated::LatLngBounds>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_lat_lng_bounds_for_camera", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_lat_lng_bounds_for_camera(native, &binding_arg_1, completion)
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_lat_lng_bounds>(result)?;
                Ok(maplibre_core::generated::LatLngBounds::from_native(value))
            },
        )
    }

    /// Calls `mln_map_lat_lng_bounds_for_camera_unwrapped` using its header execution and ownership contract.
    pub fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        binding_arg_1: &maplibre_core::generated::CameraOptions,
    ) -> Result<NativeFuture<maplibre_core::generated::LatLngBounds>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_lat_lng_bounds_for_camera_unwrapped", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_lat_lng_bounds_for_camera_unwrapped(native, &binding_arg_1, completion)
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_lat_lng_bounds>(result)?;
                Ok(maplibre_core::generated::LatLngBounds::from_native(value))
            },
        )
    }

    /// Calls `mln_map_lat_lng_for_pixel` using its header execution and ownership contract.
    pub fn lat_lng_for_pixel(
        &self,
        binding_arg_1: maplibre_core::generated::ScreenPoint,
    ) -> Result<NativeFuture<maplibre_core::generated::LatLng>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_lat_lng_for_pixel", native.0)?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_lat_lng_for_pixel(native, binding_arg_1.to_native(), completion)
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_lat_lng>(result)?;
                Ok(maplibre_core::generated::LatLng::from_native(value))
            },
        )
    }

    /// Calls `mln_map_lat_lng_for_pixel_unwrapped` using its header execution and ownership contract.
    pub fn lat_lng_for_pixel_unwrapped(
        &self,
        binding_arg_1: maplibre_core::generated::ScreenPoint,
    ) -> Result<NativeFuture<maplibre_core::generated::LatLng>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_lat_lng_for_pixel_unwrapped", native.0)?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_lat_lng_for_pixel_unwrapped(
                    native,
                    binding_arg_1.to_native(),
                    completion,
                )
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_lat_lng>(result)?;
                Ok(maplibre_core::generated::LatLng::from_native(value))
            },
        )
    }

    /// Calls `mln_map_lat_lngs_for_pixels` using its header execution and ownership contract.
    pub fn lat_lngs_for_pixels(
        &self,
        binding_arg_1: &[maplibre_core::generated::ScreenPoint],
    ) -> Result<NativeFuture<Vec<maplibre_core::generated::LatLng>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_lat_lngs_for_pixels", native.0)?;
        let binding_arg_1: Vec<_> = binding_arg_1
            .iter()
            .map(|value| -> Result<_> { Ok((value).to_native()) })
            .collect::<Result<_>>()?;
        let binding_arg_2 = binding_arg_1
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_lat_lngs_for_pixels(
                    native,
                    binding_arg_1.as_ptr(),
                    binding_arg_2,
                    completion,
                )
            },
            |result| {
                crate::completion::copy_slice::<sys::mln_lat_lng>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(maplibre_core::generated::LatLng::from_native(value))
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_map_lat_lngs_for_pixels_unwrapped` using its header execution and ownership contract.
    pub fn lat_lngs_for_pixels_unwrapped(
        &self,
        binding_arg_1: &[maplibre_core::generated::ScreenPoint],
    ) -> Result<NativeFuture<Vec<maplibre_core::generated::LatLng>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_lat_lngs_for_pixels_unwrapped", native.0)?;
        let binding_arg_1: Vec<_> = binding_arg_1
            .iter()
            .map(|value| -> Result<_> { Ok((value).to_native()) })
            .collect::<Result<_>>()?;
        let binding_arg_2 = binding_arg_1
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_lat_lngs_for_pixels_unwrapped(
                    native,
                    binding_arg_1.as_ptr(),
                    binding_arg_2,
                    completion,
                )
            },
            |result| {
                crate::completion::copy_slice::<sys::mln_lat_lng>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(maplibre_core::generated::LatLng::from_native(value))
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_map_list_style_layer_ids` using its header execution and ownership contract.
    pub fn list_style_layer_ids(&self) -> Result<NativeFuture<Vec<String>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_list_style_layer_ids", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_list_style_layer_ids(native, completion) },
            |result| {
                crate::completion::copy_slice::<sys::mln_buffer_view>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(unsafe { maplibre_core::string::copy_string_view(value) }?)
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_map_list_style_layers` using its header execution and ownership contract.
    pub fn list_style_layers(
        &self,
    ) -> Result<NativeFuture<Vec<maplibre_core::generated::StyleLayerEntry>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_list_style_layers", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_list_style_layers(native, completion) },
            |result| {
                crate::completion::copy_slice::<sys::mln_style_layer_entry>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(unsafe {
                            maplibre_core::generated::StyleLayerEntry::from_native(value)
                        }?)
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_map_list_style_source_ids` using its header execution and ownership contract.
    pub fn list_style_source_ids(&self) -> Result<NativeFuture<Vec<String>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_list_style_source_ids", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_list_style_source_ids(native, completion) },
            |result| {
                crate::completion::copy_slice::<sys::mln_buffer_view>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(unsafe { maplibre_core::string::copy_string_view(value) }?)
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_map_loaded_style_json` using its header execution and ownership contract.
    pub fn loaded_style_json(&self) -> Result<NativeFuture<Vec<u8>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_loaded_style_json", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_loaded_style_json(native, completion) },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_buffer_view>(result)?;
                Ok(unsafe { maplibre_core::string::copy_string_view_bytes(value) }?)
            },
        )
    }

    /// Calls `mln_map_meters_per_pixel_at_latitude` using its header execution and ownership contract.
    pub fn meters_per_pixel_at_latitude(&self, binding_arg_1: f64) -> Result<NativeFuture<f64>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_meters_per_pixel_at_latitude", native.0)?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_meters_per_pixel_at_latitude(native, binding_arg_1, completion)
            },
            |result| {
                let value = crate::completion::copy_value::<f64>(result)?;
                Ok(value)
            },
        )
    }

    /// Calls `mln_map_move_style_layer` using its header execution and ownership contract.
    pub fn move_style_layer(
        &self,
        binding_arg_1: &str,
        binding_arg_2: Option<&str>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_move_style_layer", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = match (binding_arg_2).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_move_style_layer(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_pixel_for_lat_lng` using its header execution and ownership contract.
    pub fn pixel_for_lat_lng(
        &self,
        binding_arg_1: maplibre_core::generated::LatLng,
    ) -> Result<NativeFuture<maplibre_core::generated::ScreenPoint>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_pixel_for_lat_lng", native.0)?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_pixel_for_lat_lng(native, binding_arg_1.to_native(), completion)
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_screen_point>(result)?;
                Ok(maplibre_core::generated::ScreenPoint::from_native(value))
            },
        )
    }

    /// Calls `mln_map_pixels_for_lat_lngs` using its header execution and ownership contract.
    pub fn pixels_for_lat_lngs(
        &self,
        binding_arg_1: &[maplibre_core::generated::LatLng],
    ) -> Result<NativeFuture<Vec<maplibre_core::generated::ScreenPoint>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_pixels_for_lat_lngs", native.0)?;
        let binding_arg_1: Vec<_> = binding_arg_1
            .iter()
            .map(|value| -> Result<_> { Ok((value).to_native()) })
            .collect::<Result<_>>()?;
        let binding_arg_2 = binding_arg_1
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_map_pixels_for_lat_lngs(
                    native,
                    binding_arg_1.as_ptr(),
                    binding_arg_2,
                    completion,
                )
            },
            |result| {
                crate::completion::copy_slice::<sys::mln_screen_point>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(maplibre_core::generated::ScreenPoint::from_native(value))
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_map_projection_create` using its header execution and ownership contract.
    pub fn projection_create(&self) -> Result<NativeFuture<crate::MapProjectionHandle>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_projection_create", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_projection_create(native, completion) },
            move |result| {
                let value = crate::completion::copy_value::<sys::mln_map_projection>(result)?;
                crate::MapProjectionHandle::from_native(value)
            },
        )
    }

    /// Calls `mln_map_release` using its header execution and ownership contract.
    pub fn release(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            crate::completion::submit(
                |completion| unsafe { sys::mln_map_release(native, completion) },
                crate::completion::unit,
            )
        })?;
        Ok(result.unwrap_or_else(|| crate::completion::ready(())))
    }

    /// Calls `mln_map_remove_feature_state` using its header execution and ownership contract.
    pub fn remove_feature_state(
        &self,
        binding_arg_1: &maplibre_core::generated::FeatureStateSelector,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_remove_feature_state", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_remove_feature_state(native, &binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_remove_style_image` using its header execution and ownership contract.
    pub fn remove_style_image(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_remove_style_image", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_remove_style_image(native, binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_remove_style_layer` using its header execution and ownership contract.
    pub fn remove_style_layer(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_remove_style_layer", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_remove_style_layer(native, binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_remove_style_source` using its header execution and ownership contract.
    pub fn remove_style_source(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_remove_style_source", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_remove_style_source(native, binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_request_repaint` using its header execution and ownership contract.
    pub fn request_repaint(&self) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_request_repaint", native.0)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_request_repaint(native, completion)
        })
    }

    /// Calls `mln_map_request_still_image` using its header execution and ownership contract.
    pub fn request_still_image(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_request_still_image", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_request_still_image(native, completion) },
            crate::completion::unit,
        )
    }

    /// Calls `mln_map_resize` using its header execution and ownership contract.
    pub fn resize(
        &self,
        binding_arg_1: maplibre_core::generated::LogicalExtent,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_resize", native.0)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_resize(native, binding_arg_1.to_native(), completion)
        })
    }

    /// Calls `mln_map_set_bounds` using its header execution and ownership contract.
    pub fn set_bounds(
        &self,
        binding_arg_1: &maplibre_core::generated::BoundOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_bounds", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_bounds(native, &binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_set_custom_geometry_source_tile_data` using its header execution and ownership contract.
    pub fn set_custom_geometry_source_tile_data(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::CanonicalTileId,
        binding_arg_3: &[u8],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_custom_geometry_source_tile_data", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_3 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_3).as_ptr().cast(),
            size: (binding_arg_3).len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
                binding_arg_3,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_custom_mvt_vector_source_tile_data` using its header execution and ownership contract.
    pub fn set_custom_mvt_vector_source_tile_data(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::CanonicalTileId,
        binding_arg_3: &[u8],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_custom_mvt_vector_source_tile_data", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_3 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_3).as_ptr().cast(),
            size: (binding_arg_3).len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_custom_mvt_vector_source_tile_data(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
                binding_arg_3,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_custom_mvt_vector_source_tile_error` using its header execution and ownership contract.
    pub fn set_custom_mvt_vector_source_tile_error(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::CanonicalTileId,
        binding_arg_3: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check(
            "mln_map_set_custom_mvt_vector_source_tile_error",
            native.0,
        )?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_3 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_3).as_bytes().as_ptr().cast(),
            size: (binding_arg_3).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_custom_mvt_vector_source_tile_error(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
                binding_arg_3,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_debug_options` using its header execution and ownership contract.
    pub fn set_debug_options(
        &self,
        binding_arg_1: maplibre_core::generated::MapDebugOption,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_debug_options", native.0)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_debug_options(native, binding_arg_1.to_native(), completion)
        })
    }

    /// Calls `mln_map_set_event_mask` using its header execution and ownership contract.
    pub fn set_event_mask(
        &self,
        binding_arg_1: maplibre_core::generated::RuntimeEventMask,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_event_mask", native.0)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_event_mask(native, binding_arg_1.to_native(), completion)
        })
    }

    /// Calls `mln_map_set_feature_state` using its header execution and ownership contract.
    pub fn set_feature_state(
        &self,
        binding_arg_1: &maplibre_core::generated::FeatureStateSelector,
        binding_arg_2: &[u8],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_feature_state", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_ptr().cast(),
            size: (binding_arg_2).len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_feature_state(native, &binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_free_camera_options` using its header execution and ownership contract.
    pub fn set_free_camera_options(
        &self,
        binding_arg_1: &maplibre_core::generated::FreeCameraOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_free_camera_options", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_free_camera_options(native, &binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_set_geojson_source_data` using its header execution and ownership contract.
    pub fn set_geojson_source_data(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &crate::GeojsonSourceDataHandle,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_geojson_source_data", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2_native = binding_arg_2.inner.native()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_geojson_source_data(
                native,
                binding_arg_1,
                binding_arg_2_native,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_geojson_source_synchronous_tiling` using its header execution and ownership contract.
    pub fn set_geojson_source_synchronous_tiling(
        &self,
        binding_arg_1: &str,
        binding_arg_2: bool,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_geojson_source_synchronous_tiling", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_geojson_source_synchronous_tiling(
                native,
                binding_arg_1,
                binding_arg_2,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_geojson_source_url` using its header execution and ownership contract.
    pub fn set_geojson_source_url(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_geojson_source_url", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_geojson_source_url(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_global_state_property` using its header execution and ownership contract.
    pub fn set_global_state_property(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[u8],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_global_state_property", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_ptr().cast(),
            size: (binding_arg_2).len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_global_state_property(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_image_source_coordinates` using its header execution and ownership contract.
    pub fn set_image_source_coordinates(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[maplibre_core::generated::LatLng],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_image_source_coordinates", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2: Vec<_> = binding_arg_2
            .iter()
            .map(|value| -> Result<_> { Ok((value).to_native()) })
            .collect::<Result<_>>()?;
        let binding_arg_3 = binding_arg_2
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_image_source_coordinates(
                native,
                binding_arg_1,
                binding_arg_2.as_ptr(),
                binding_arg_3,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_image_source_image` using its header execution and ownership contract.
    pub fn set_image_source_image(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &maplibre_core::generated::PremultipliedRgba8Image,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_image_source_image", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_image_source_image(native, binding_arg_1, &binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_image_source_url` using its header execution and ownership contract.
    pub fn set_image_source_url(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_image_source_url", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_image_source_url(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_layer_filter` using its header execution and ownership contract.
    pub fn set_layer_filter(
        &self,
        binding_arg_1: &str,
        binding_arg_2: Option<&[u8]>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_layer_filter", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = binding_arg_2
            .map(|value| -> Result<_> {
                Ok(maplibre_native_ffi_sys::mln_buffer_view {
                    data: (value).as_ptr().cast(),
                    size: (value).len(),
                })
            })
            .transpose()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_layer_filter(
                native,
                binding_arg_1,
                binding_arg_2
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                completion,
            )
        })
    }

    /// Calls `mln_map_set_layer_max_zoom` using its header execution and ownership contract.
    pub fn set_layer_max_zoom(
        &self,
        binding_arg_1: &str,
        binding_arg_2: f64,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_layer_max_zoom", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_layer_max_zoom(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_layer_min_zoom` using its header execution and ownership contract.
    pub fn set_layer_min_zoom(
        &self,
        binding_arg_1: &str,
        binding_arg_2: f64,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_layer_min_zoom", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_layer_min_zoom(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_layer_property` using its header execution and ownership contract.
    pub fn set_layer_property(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
        binding_arg_3: &[u8],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_layer_property", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        let binding_arg_3 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_3).as_ptr().cast(),
            size: (binding_arg_3).len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_layer_property(
                native,
                binding_arg_1,
                binding_arg_2,
                binding_arg_3,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_layer_source_id` using its header execution and ownership contract.
    pub fn set_layer_source_id(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_layer_source_id", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_bytes().as_ptr().cast(),
            size: (binding_arg_2).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_layer_source_id(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_layer_source_layer` using its header execution and ownership contract.
    pub fn set_layer_source_layer(
        &self,
        binding_arg_1: &str,
        binding_arg_2: Option<&str>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_layer_source_layer", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = match (binding_arg_2).as_ref() {
            Some(item) => maplibre_native_ffi_sys::mln_buffer_view {
                data: (item).as_bytes().as_ptr().cast(),
                size: (item).as_bytes().len(),
            },
            None => maplibre_native_ffi_sys::mln_buffer_view {
                data: std::ptr::null(),
                size: 0,
            },
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_layer_source_layer(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_layer_visibility` using its header execution and ownership contract.
    pub fn set_layer_visibility(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::StyleLayerVisibility,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_layer_visibility", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_layer_visibility(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
                completion,
            )
        })
    }

    /// Calls `mln_map_set_location_indicator_accuracy_radius` using its header execution and ownership contract.
    pub fn set_location_indicator_accuracy_radius(
        &self,
        binding_arg_1: &str,
        binding_arg_2: f64,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_location_indicator_accuracy_radius", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_location_indicator_accuracy_radius(
                native,
                binding_arg_1,
                binding_arg_2,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_location_indicator_bearing` using its header execution and ownership contract.
    pub fn set_location_indicator_bearing(
        &self,
        binding_arg_1: &str,
        binding_arg_2: f64,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_location_indicator_bearing", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_location_indicator_bearing(
                native,
                binding_arg_1,
                binding_arg_2,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_location_indicator_image_name` using its header execution and ownership contract.
    pub fn set_location_indicator_image_name(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::LocationIndicatorImageKind,
        binding_arg_3: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_location_indicator_image_name", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_3 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_3).as_bytes().as_ptr().cast(),
            size: (binding_arg_3).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_location_indicator_image_name(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
                binding_arg_3,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_location_indicator_location` using its header execution and ownership contract.
    pub fn set_location_indicator_location(
        &self,
        binding_arg_1: &str,
        binding_arg_2: maplibre_core::generated::LatLng,
        binding_arg_3: f64,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_location_indicator_location", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_location_indicator_location(
                native,
                binding_arg_1,
                binding_arg_2.to_native(),
                binding_arg_3,
                completion,
            )
        })
    }

    /// Calls `mln_map_set_projection_mode` using its header execution and ownership contract.
    pub fn set_projection_mode(
        &self,
        binding_arg_1: &maplibre_core::generated::ProjectionMode,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_projection_mode", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_projection_mode(native, &binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_set_rendering_stats_view_enabled` using its header execution and ownership contract.
    pub fn set_rendering_stats_view_enabled(
        &self,
        binding_arg_1: bool,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_rendering_stats_view_enabled", native.0)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_rendering_stats_view_enabled(native, binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_set_style_image` using its header execution and ownership contract.
    pub fn set_style_image(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &maplibre_core::generated::PremultipliedRgba8Image,
        binding_arg_3: Option<&maplibre_core::generated::StyleImageOptions>,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_style_image", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let binding_arg_3 = binding_arg_3
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_style_image(
                native,
                binding_arg_1,
                &binding_arg_2,
                binding_arg_3
                    .as_ref()
                    .map_or(std::ptr::null(), |value| value),
                completion,
            )
        })
    }

    /// Calls `mln_map_set_style_json` using its header execution and ownership contract.
    pub fn set_style_json(
        &self,
        binding_arg_1: &[u8],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_style_json", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_ptr().cast(),
            size: (binding_arg_1).len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_style_json(native, binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_set_style_light_json` using its header execution and ownership contract.
    pub fn set_style_light_json(
        &self,
        binding_arg_1: &[u8],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_style_light_json", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_ptr().cast(),
            size: (binding_arg_1).len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_style_light_json(native, binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_set_style_light_property` using its header execution and ownership contract.
    pub fn set_style_light_property(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[u8],
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_style_light_property", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_ptr().cast(),
            size: (binding_arg_2).len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_style_light_property(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_style_source_volatile` using its header execution and ownership contract.
    pub fn set_style_source_volatile(
        &self,
        binding_arg_1: &str,
        binding_arg_2: bool,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_style_source_volatile", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_style_source_volatile(native, binding_arg_1, binding_arg_2, completion)
        })
    }

    /// Calls `mln_map_set_style_transition_options` using its header execution and ownership contract.
    pub fn set_style_transition_options(
        &self,
        binding_arg_1: &maplibre_core::generated::StyleTransitionOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_style_transition_options", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_style_transition_options(native, &binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_set_style_url` using its header execution and ownership contract.
    pub fn set_style_url(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_style_url", native.0)?;
        let binding_arg_1 = maplibre_core::string::c_string(binding_arg_1)?;
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_style_url(native, binding_arg_1.as_ptr(), completion)
        })
    }

    /// Calls `mln_map_set_tile_options` using its header execution and ownership contract.
    pub fn set_tile_options(
        &self,
        binding_arg_1: &maplibre_core::generated::MapTileOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_tile_options", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_tile_options(native, &binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_set_viewport_options` using its header execution and ownership contract.
    pub fn set_viewport_options(
        &self,
        binding_arg_1: &maplibre_core::generated::MapViewportOptions,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_set_viewport_options", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_set_viewport_options(native, &binding_arg_1, completion)
        })
    }

    /// Calls `mln_map_snapshot_get` using its header execution and ownership contract.
    pub fn snapshot_get(&self) -> Result<maplibre_core::generated::MapSnapshot> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_snapshot_get", native.0)?;
        let mut binding_arg_1: sys::mln_map_snapshot =
            maplibre_core::generated::MapSnapshot::default().to_native();
        maplibre_core::check(unsafe { sys::mln_map_snapshot_get(native, &mut binding_arg_1) })?;
        Ok(maplibre_core::generated::MapSnapshot::from_native(
            binding_arg_1,
        ))
    }

    /// Calls `mln_map_style_url` using its header execution and ownership contract.
    pub fn style_url(&self) -> Result<NativeFuture<String>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_style_url", native.0)?;
        crate::completion::submit(
            |completion| unsafe { sys::mln_map_style_url(native, completion) },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_buffer_view>(result)?;
                Ok(unsafe { maplibre_core::string::copy_string_view(value) }?)
            },
        )
    }

    /// Calls `mln_map_update_camera` using its header execution and ownership contract.
    pub fn update_camera(
        &self,
        binding_arg_1: &maplibre_core::generated::CameraUpdate,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_update_camera", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit_command(|completion| unsafe {
            sys::mln_map_update_camera(native, &binding_arg_1, completion)
        })
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_metal_borrowed_texture_attach` using its header execution and ownership contract.
    pub unsafe fn metal_borrowed_texture_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::MetalBorrowedTextureDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_metal_borrowed_texture_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_metal_borrowed_texture_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_metal_owned_texture_attach` using its header execution and ownership contract.
    pub unsafe fn metal_owned_texture_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::MetalOwnedTextureDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_metal_owned_texture_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_metal_owned_texture_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_metal_surface_attach` using its header execution and ownership contract.
    pub unsafe fn metal_surface_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::MetalSurfaceDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_metal_surface_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_metal_surface_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_opengl_borrowed_texture_attach` using its header execution and ownership contract.
    pub unsafe fn opengl_borrowed_texture_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::OpenglBorrowedTextureDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_opengl_borrowed_texture_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_opengl_borrowed_texture_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_opengl_owned_texture_attach` using its header execution and ownership contract.
    pub unsafe fn opengl_owned_texture_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::OpenglOwnedTextureDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_opengl_owned_texture_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_opengl_owned_texture_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_opengl_surface_attach` using its header execution and ownership contract.
    pub unsafe fn opengl_surface_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::OpenglSurfaceDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_opengl_surface_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_opengl_surface_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_vulkan_borrowed_texture_attach` using its header execution and ownership contract.
    pub unsafe fn vulkan_borrowed_texture_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::VulkanBorrowedTextureDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_vulkan_borrowed_texture_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_vulkan_borrowed_texture_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_vulkan_owned_texture_attach` using its header execution and ownership contract.
    pub unsafe fn vulkan_owned_texture_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::VulkanOwnedTextureDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_vulkan_owned_texture_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_vulkan_owned_texture_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_vulkan_surface_attach` using its header execution and ownership contract.
    pub unsafe fn vulkan_surface_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::VulkanSurfaceDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_vulkan_surface_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_vulkan_surface_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_webgpu_borrowed_texture_attach` using its header execution and ownership contract.
    pub unsafe fn webgpu_borrowed_texture_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::WebgpuBorrowedTextureDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_webgpu_borrowed_texture_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_webgpu_borrowed_texture_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_webgpu_owned_texture_attach` using its header execution and ownership contract.
    pub unsafe fn webgpu_owned_texture_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::WebgpuOwnedTextureDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_webgpu_owned_texture_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_webgpu_owned_texture_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_webgpu_surface_attach` using its header execution and ownership contract.
    pub unsafe fn webgpu_surface_attach(
        &self,
        binding_arg_1: &maplibre_core::generated::WebgpuSurfaceDescriptor,
        binding_arg_2: &maplibre_core::generated::RenderSessionAttachOptions,
    ) -> Result<(crate::RenderSessionHandle, NativeFuture<()>)> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_webgpu_surface_attach", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        let binding_arg_2 = binding_arg_2.to_native(&mut arena)?;
        let mut binding_arg_3 = sys::mln_render_session(0);
        let submitted = crate::completion::submit(
            |completion| unsafe {
                sys::mln_webgpu_surface_attach(
                    native,
                    &binding_arg_1,
                    &binding_arg_2,
                    &mut binding_arg_3,
                    completion,
                )
            },
            crate::completion::unit,
        )?;
        arena.accept_registrations();
        Ok((
            crate::RenderSessionHandle::from_native(binding_arg_3, binding_parent)?,
            submitted,
        ))
    }
}
