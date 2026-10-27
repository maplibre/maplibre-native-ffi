// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A map, which holds map state independent of any render target.
    ///
    /// See `mln_map` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct MapHandle(mln_map) dispose |raw| crate::error::check(|out_diagnostic| unsafe { sys::mln_map_dispose(raw, out_diagnostic) });
    /// Returns the native handle value, which runtime events report as their source.
    fn id;
}

impl MapHandle {
    /// Adds a custom geometry source. The callback may run on any native worker
    /// thread, concurrently with runtime and map calls.
    ///
    /// See `mln_map_add_custom_geometry_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_custom_geometry_source(
        &self,
        source_id: &str,
        options: CustomGeometrySourceOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_custom_geometry_source(
                map,
                source_id,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a custom vector source. The callback may run on any native worker
    /// thread, concurrently with runtime and map calls.
    ///
    /// See `mln_map_add_custom_vector_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_custom_vector_source(
        &self,
        source_id: &str,
        options: CustomVectorSourceOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_custom_vector_source(
                map,
                source_id,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a GeoJSON source with prepared inline data.
    ///
    /// See `mln_map_add_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_geo_json_source_data(
        &self,
        source_id: &str,
        data: &GeoJsonSourceDataHandle,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let data = data.inner.native();
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_geo_json_source_data(map, source_id, data, completion, out_diagnostic)
        })
    }

    /// Adds a GeoJSON source with URL data.
    ///
    /// See `mln_map_add_geo_json_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_geo_json_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&GeoJsonSourceOptions<'_>>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_geo_json_source_url(
                map,
                source_id,
                url,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds an image source with inline image pixels.
    ///
    /// See `mln_map_add_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_image_source_image(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
        image: &PremultipliedRgba8Image<'_>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        let image = call.reference(&image)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_image_source_image(
                map,
                source_id,
                coordinates,
                coordinate_count,
                image,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds an image source that loads its image from a URL.
    ///
    /// See `mln_map_add_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_image_source_url(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
        url: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        let url = call.input(&url)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_image_source_url(
                map,
                source_id,
                coordinates,
                coordinate_count,
                url,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds one style layer from a full style-spec layer JSON object.
    ///
    /// See `mln_map_add_style_layer_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_style_layer_json(
        &self,
        layer_json: &str,
        before_layer_id: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let layer_json = call.input(&layer_json)?;
        let before_layer_id = call.input(&before_layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_style_layer_json(
                map,
                layer_json,
                before_layer_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds one style source from a style-spec source JSON object.
    ///
    /// See `mln_map_add_style_source_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_style_source_json(
        &self,
        source_id: &str,
        source_json: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let source_json = call.input(&source_json)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_style_source_json(
                map,
                source_id,
                source_json,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Submits one atomic relative camera update.
    ///
    /// See `mln_map_apply_camera_delta` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn apply_camera_delta(
        &self,
        delta: &CameraDelta,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let delta = call.reference(&delta)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_apply_camera_delta(map, delta, completion, out_diagnostic)
        })
    }

    /// Starts attachment of a texture target that renders on a graphics device.
    ///
    /// See `mln_map_attach_graphics_device_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
    pub fn attach_graphics_device_texture(
        &self,
        device: &GraphicsDeviceHandle,
        descriptor: &GraphicsDeviceTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let device = device.inner.native();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_graphics_device_texture(
                    map,
                    device,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a ring of caller-owned Metal textures.
    ///
    /// See `mln_map_attach_metal_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "ios",
        target_os = "macos",
        target_os = "tvos",
        target_os = "visionos"
    ))]
    pub unsafe fn attach_metal_borrowed_texture(
        &self,
        descriptor: &MetalBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_metal_borrowed_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a session-owned Metal texture ring.
    ///
    /// See `mln_map_attach_metal_owned_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "ios",
        target_os = "macos",
        target_os = "tvos",
        target_os = "visionos"
    ))]
    pub unsafe fn attach_metal_owned_texture(
        &self,
        descriptor: &MetalOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_metal_owned_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a Metal surface target.
    ///
    /// See `mln_map_attach_metal_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "ios",
        target_os = "macos",
        target_os = "tvos",
        target_os = "visionos"
    ))]
    pub unsafe fn attach_metal_surface(
        &self,
        descriptor: &MetalSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_metal_surface(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a ring of caller-owned OpenGL textures.
    ///
    /// See `mln_map_attach_open_gl_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn attach_open_gl_borrowed_texture(
        &self,
        descriptor: &OpenGlBorrowedTextureDescriptor<'_>,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_open_gl_borrowed_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a session-owned OpenGL texture ring.
    ///
    /// See `mln_map_attach_open_gl_owned_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn attach_open_gl_owned_texture(
        &self,
        descriptor: &OpenGlOwnedTextureDescriptor<'_>,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_open_gl_owned_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of an OpenGL surface target.
    ///
    /// See `mln_map_attach_open_gl_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn attach_open_gl_surface(
        &self,
        descriptor: &OpenGlSurfaceDescriptor<'_>,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_open_gl_surface(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a ring of caller-owned Vulkan images.
    ///
    /// See `mln_map_attach_vulkan_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "android",
        target_os = "freebsd",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub unsafe fn attach_vulkan_borrowed_texture(
        &self,
        descriptor: &VulkanBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_vulkan_borrowed_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a session-owned Vulkan texture ring.
    ///
    /// See `mln_map_attach_vulkan_owned_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "android",
        target_os = "freebsd",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub unsafe fn attach_vulkan_owned_texture(
        &self,
        descriptor: &VulkanOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_vulkan_owned_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a Vulkan surface target.
    ///
    /// See `mln_map_attach_vulkan_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "android",
        target_os = "freebsd",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub unsafe fn attach_vulkan_surface(
        &self,
        descriptor: &VulkanSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_vulkan_surface(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a ring of caller-owned WebGPU textures.
    ///
    /// See `mln_map_attach_web_gpu_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn attach_web_gpu_borrowed_texture(
        &self,
        descriptor: &WebGpuBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_web_gpu_borrowed_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a session-owned WebGPU texture ring.
    ///
    /// See `mln_map_attach_web_gpu_owned_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn attach_web_gpu_owned_texture(
        &self,
        descriptor: &WebGpuOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_web_gpu_owned_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Starts attachment of a WebGPU surface target.
    ///
    /// See `mln_map_attach_web_gpu_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn attach_web_gpu_surface(
        &self,
        descriptor: &WebGpuSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_web_gpu_surface(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Begins a command group, which holds this map's render updates until the
    /// group ends.
    ///
    /// See `mln_map_begin_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn begin_command_group(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_begin_command_group(map, completion, out_diagnostic)
        })
    }

    /// Starts an ordered query for a camera that fits a GeoJSON geometry.
    ///
    /// See `mln_map_camera_for_geometry` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_for_geometry(
        &self,
        geometry: &str,
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<NativeFuture<CameraOptions>> {
        let mut call = self.inner.call();
        let geometry = call.input(&geometry)?;
        let fit_options = call.optional_reference(fit_options.as_ref())?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_for_geometry(
                    map,
                    geometry,
                    fit_options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_camera_options, _>,
        )
    }

    /// Starts an ordered query for a camera that fits geographic bounds.
    ///
    /// See `mln_map_camera_for_lat_lng_bounds` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_for_lat_lng_bounds(
        &self,
        bounds: LatLngBounds,
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<NativeFuture<CameraOptions>> {
        let mut call = self.inner.call();
        let bounds = call.input(&bounds)?;
        let fit_options = call.optional_reference(fit_options.as_ref())?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_for_lat_lng_bounds(
                    map,
                    bounds,
                    fit_options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_camera_options, _>,
        )
    }

    /// Starts an ordered query for a camera that fits geographic coordinates.
    ///
    /// See `mln_map_camera_for_lat_lngs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
        fit_options: Option<&CameraFitOptions>,
    ) -> Result<NativeFuture<CameraOptions>> {
        let mut call = self.inner.call();
        let coordinate_count = convert::count(coordinates.len())?;
        let coordinates = call.array(coordinates)?;
        let fit_options = call.optional_reference(fit_options.as_ref())?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_for_lat_lngs(
                    map,
                    coordinates,
                    coordinate_count,
                    fit_options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_camera_options, _>,
        )
    }

    /// Cancels the camera transitions of the commands whose animation carried
    /// transition_id, and leaves every other transition running.
    ///
    /// See `mln_map_cancel_camera_transition` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_camera_transition(
        &self,
        transition_id: u64,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_cancel_camera_transition(map, transition_id, completion, out_diagnostic)
        })
    }

    /// Cancels the camera transitions running when this command commits.
    ///
    /// See `mln_map_cancel_transitions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_transitions(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_cancel_transitions(map, completion, out_diagnostic)
        })
    }

    /// Consumes a map handle without observing its asynchronous retirement.
    ///
    /// See `mln_map_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn dispose(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|map| {
            let mut call = Call::new(map);
            call.status(|map, out_diagnostic| unsafe { sys::mln_map_dispose(map, out_diagnostic) })
        })
    }

    /// Submits an ordered debug-log command.
    ///
    /// See `mln_map_dump_debug_logs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_dump_debug_logs(map, completion, out_diagnostic)
        })
    }

    /// Ends the innermost command group that `MapHandle::begin_command_group()`
    /// began.
    ///
    /// See `mln_map_end_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn end_command_group(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_end_command_group(map, completion, out_diagnostic)
        })
    }

    /// Starts an ordered camera read.
    ///
    /// See `mln_map_get_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn get_camera(&self) -> Result<NativeFuture<CameraQueryResult>> {
        let call = self.inner.call();
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_camera(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_camera_query_result, _>,
        )
    }

    /// Starts an ordered read of per-feature state from this map.
    ///
    /// See `mln_map_get_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_feature_state(
        &self,
        selector: &FeatureStateSelector<'_>,
    ) -> Result<NativeFuture<String>> {
        let mut call = self.inner.call();
        let selector = call.reference(&selector)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_feature_state(map, selector, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Queries the global-state JSON object, including style defaults.
    ///
    /// See `mln_map_get_global_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_global_state(&self) -> Result<NativeFuture<String>> {
        let call = self.inner.call();
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_global_state(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Copies image source coordinates.
    ///
    /// See `mln_map_get_image_source_coordinates` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_image_source_coordinates(
        &self,
        source_id: &str,
    ) -> Result<NativeFuture<Vec<LatLng>>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_image_source_coordinates(
                    map,
                    source_id,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_lat_lng, _>,
        )
    }

    /// Starts an ordered copy of the last successfully parsed style document.
    ///
    /// See `mln_map_get_loaded_style_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_loaded_style_json(&self) -> Result<NativeFuture<String>> {
        let call = self.inner.call();
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_loaded_style_json(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Copies the map's latest published snapshot.
    ///
    /// See `mln_map_get_snapshot` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_snapshot(&self) -> Result<MapSnapshot> {
        let mut call = self.inner.call();
        let mut out_snapshot: sys::mln_map_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_map_snapshot>() as _;
        call.status(|map, out_diagnostic| unsafe {
            sys::mln_map_get_snapshot(map, &mut out_snapshot, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_snapshot) }?)
    }

    /// Copies one complete runtime style image.
    ///
    /// See `mln_map_get_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_image(&self, image_id: &str) -> Result<NativeFuture<Option<StyleImageInfo>>> {
        let mut call = self.inner.call();
        let image_id = call.input(&image_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_image(map, image_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_image_info, _>,
        )
    }

    /// Copies the complete metadata of one style layer.
    ///
    /// See `mln_map_get_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_layer(&self, layer_id: &str) -> Result<NativeFuture<Option<StyleLayerInfo>>> {
        let mut call = self.inner.call();
        let layer_id = call.input(&layer_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_layer(map, layer_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_layer_info, _>,
        )
    }

    /// Serializes one style layer as a full style-spec layer JSON object.
    ///
    /// See `mln_map_get_style_layer_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_layer_json(&self, layer_id: &str) -> Result<NativeFuture<Option<String>>> {
        let mut call = self.inner.call();
        let layer_id = call.input(&layer_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_layer_json(map, layer_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Serializes one layer property as a style-spec JSON value.
    ///
    /// See `mln_map_get_style_layer_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
    ) -> Result<NativeFuture<Option<String>>> {
        let mut call = self.inner.call();
        let layer_id = call.input(&layer_id)?;
        let property_name = call.input(&property_name)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_layer_property(
                    map,
                    layer_id,
                    property_name,
                    completion,
                    out_diagnostic,
                )
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Serializes one style light property as a style-spec JSON value.
    ///
    /// See `mln_map_get_style_light_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_light_property(
        &self,
        property_name: &str,
    ) -> Result<NativeFuture<Option<String>>> {
        let mut call = self.inner.call();
        let property_name = call.input(&property_name)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_light_property(
                    map,
                    property_name,
                    completion,
                    out_diagnostic,
                )
            },
            completion::optional::<sys::mln_buffer_view, _>,
        )
    }

    /// Copies the complete metadata of one style source.
    ///
    /// See `mln_map_get_style_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_source(
        &self,
        source_id: &str,
    ) -> Result<NativeFuture<Option<StyleSourceInfo>>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_source(map, source_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_style_source_info, _>,
        )
    }

    /// Reads the style's global transition options.
    ///
    /// See `mln_map_get_style_transition_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_transition_options(&self) -> Result<NativeFuture<StyleTransitionOptions>> {
        let call = self.inner.call();
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_transition_options(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_style_transition_options, _>,
        )
    }

    /// Starts an ordered copy of the last requested style URL.
    ///
    /// See `mln_map_get_style_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_style_url(&self) -> Result<NativeFuture<String>> {
        let call = self.inner.call();
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_url(map, completion, out_diagnostic)
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Invalidates custom geometry source data inside one geographic region.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_geometry_source_region(
        &self,
        source_id: &str,
        bounds: LatLngBounds,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let bounds = call.input(&bounds)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                map,
                source_id,
                bounds,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Invalidates custom geometry source data for one canonical tile.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                map,
                source_id,
                tile_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Invalidates custom vector source data for one canonical tile.
    ///
    /// See `mln_map_invalidate_custom_vector_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_vector_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_invalidate_custom_vector_source_tile(
                map,
                source_id,
                tile_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Starts an ordered wrapped-bounds query for a copied camera.
    ///
    /// See `mln_map_lat_lng_bounds_for_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lng_bounds_for_camera(
        &self,
        camera: &CameraOptions,
    ) -> Result<NativeFuture<LatLngBounds>> {
        let mut call = self.inner.call();
        let camera = call.reference(&camera)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lng_bounds_for_camera(map, camera, completion, out_diagnostic)
            },
            completion::value::<sys::mln_lat_lng_bounds, _>,
        )
    }

    /// Starts an ordered unwrapped-bounds query for a copied camera.
    ///
    /// See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        camera: &CameraOptions,
    ) -> Result<NativeFuture<LatLngBounds>> {
        let mut call = self.inner.call();
        let camera = call.reference(&camera)?;
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lng_bounds_for_camera_unwrapped(
                    map,
                    camera,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_lat_lng_bounds, _>,
        )
    }

    /// Starts an ordered query of every style layer in style order.
    ///
    /// See `mln_map_list_style_layers` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn list_style_layers(&self) -> Result<NativeFuture<Vec<StyleLayerInfo>>> {
        let call = self.inner.call();
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_list_style_layers(map, completion, out_diagnostic)
            },
            completion::list::<sys::mln_style_layer_info, _>,
        )
    }

    /// Lists every style source in style order.
    ///
    /// See `mln_map_list_style_sources` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn list_style_sources(&self) -> Result<NativeFuture<Vec<StyleSourceInfo>>> {
        let call = self.inner.call();
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_list_style_sources(map, completion, out_diagnostic)
            },
            completion::list::<sys::mln_style_source_info, _>,
        )
    }

    /// Moves one style layer before another layer or to the top.
    ///
    /// See `mln_map_move_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn move_style_layer(
        &self,
        layer_id: &str,
        before_layer_id: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let layer_id = call.input(&layer_id)?;
        let before_layer_id = call.input(&before_layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_move_style_layer(
                map,
                layer_id,
                before_layer_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Releases a map asynchronously.
    ///
    /// See `mln_map_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn release(self) -> std::result::Result<NativeFuture<()>, (Self, Error)> {
        self.consume(|map| {
            let call = Call::new(map);
            call.complete(
                |map, completion, out_diagnostic| unsafe {
                    sys::mln_map_release(map, completion, out_diagnostic)
                },
                completion::unit,
            )
        })
    }

    /// Releases a map asynchronously.
    ///
    /// See `mln_map_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn release_queued<K: QueueKind>(
        self,
        queue: &DeliveryQueue<K>,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> std::result::Result<(), (Self, Error)> {
        self.consume(|map| {
            let call = Call::new(map).queue(queue);
            queue.enqueue(
                call,
                on_complete,
                |map, completion, out_diagnostic| unsafe {
                    sys::mln_map_release(map, completion, out_diagnostic)
                },
            )
        })
    }

    /// Removes per-feature state from this map.
    ///
    /// See `mln_map_remove_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn remove_feature_state(
        &self,
        selector: &FeatureStateSelector<'_>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let selector = call.reference(&selector)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_feature_state(map, selector, completion, out_diagnostic)
        })
    }

    /// Removes one runtime style image by ID.
    ///
    /// See `mln_map_remove_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_image(&self, image_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let image_id = call.input(&image_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_image(map, image_id, completion, out_diagnostic)
        })
    }

    /// Removes one style layer by ID.
    ///
    /// See `mln_map_remove_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_layer(&self, layer_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let layer_id = call.input(&layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_layer(map, layer_id, completion, out_diagnostic)
        })
    }

    /// Removes one style source by ID.
    ///
    /// See `mln_map_remove_style_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_source(&self, source_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_source(map, source_id, completion, out_diagnostic)
        })
    }

    /// Requests a repaint for a continuous map.
    ///
    /// See `mln_map_request_repaint` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn request_repaint(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_request_repaint(map, completion, out_diagnostic)
        })
    }

    /// Requests one still image for a static or tile map, which renders itself.
    ///
    /// See `mln_map_request_still_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn request_still_image(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_request_still_image(map, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Submits the sole post-creation logical extent update.
    ///
    /// See `mln_map_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn resize(&self, extent: LogicalExtent) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let extent = call.input(&extent)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_resize(map, extent, completion, out_diagnostic)
        })
    }

    /// Submits a copied camera-constraint command.
    ///
    /// See `mln_map_set_bounds` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_bounds(&self, options: &BoundOptions) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_bounds(map, options, completion, out_diagnostic)
        })
    }

    /// Sets custom geometry source data for one canonical tile.
    ///
    /// See `mln_map_set_custom_geometry_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_geometry_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let data = call.input(&data)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                map,
                source_id,
                tile_id,
                data,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Reports a custom geometry source error for one canonical tile.
    ///
    /// See `mln_map_set_custom_geometry_source_tile_error` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_geometry_source_tile_error(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        message: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let message = call.input(&message)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_geometry_source_tile_error(
                map,
                source_id,
                tile_id,
                message,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets custom vector source data for one canonical tile.
    ///
    /// See `mln_map_set_custom_vector_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_vector_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        encoding: StyleVectorTileEncoding,
        data: &[u8],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let data = call.input(&data)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_vector_source_tile_data(
                map,
                source_id,
                tile_id,
                encoding.as_raw(),
                data,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Reports a custom vector source error for one canonical tile.
    ///
    /// See `mln_map_set_custom_vector_source_tile_error` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_vector_source_tile_error(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        message: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let message = call.input(&message)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_vector_source_tile_error(
                map,
                source_id,
                tile_id,
                message,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Submits a debug-overlay command.
    ///
    /// See `mln_map_set_debug_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_debug_options(
        &self,
        options: MapDebugFlag,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_debug_options(map, options.bits(), completion, out_diagnostic)
        })
    }

    /// Selects which map-originated event types this map queues.
    ///
    /// See `mln_map_set_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_event_mask(
        &self,
        mask: RuntimeEventFlag,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_event_mask(map, mask.bits(), completion, out_diagnostic)
        })
    }

    /// Submits a copied per-feature-state command.
    ///
    /// See `mln_map_set_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_feature_state(
        &self,
        selector: &FeatureStateSelector<'_>,
        state: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let selector = call.reference(&selector)?;
        let state = call.input(&state)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_feature_state(map, selector, state, completion, out_diagnostic)
        })
    }

    /// Submits a copied free-camera command.
    ///
    /// See `mln_map_set_free_camera_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_free_camera_options(
        &self,
        options: &FreeCameraOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_free_camera_options(map, options, completion, out_diagnostic)
        })
    }

    /// Updates one GeoJSON source with prepared inline data.
    ///
    /// See `mln_map_set_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geo_json_source_data(
        &self,
        source_id: &str,
        data: &GeoJsonSourceDataHandle,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let data = data.inner.native();
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_geo_json_source_data(map, source_id, data, completion, out_diagnostic)
        })
    }

    /// Overrides one GeoJSON source's synchronous tiling at runtime.
    ///
    /// See `mln_map_set_geo_json_source_synchronous_tiling` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geo_json_source_synchronous_tiling(
        &self,
        source_id: &str,
        enabled: bool,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_geo_json_source_synchronous_tiling(
                map,
                source_id,
                enabled,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Updates one GeoJSON source to load data from a URL.
    ///
    /// See `mln_map_set_geo_json_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geo_json_source_url(
        &self,
        source_id: &str,
        url: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_geo_json_source_url(map, source_id, url, completion, out_diagnostic)
        })
    }

    /// Submits a global-state JSON value. JSON null restores the style default.
    ///
    /// See `mln_map_set_global_state_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_global_state_property(
        &self,
        property_name: &str,
        value: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let property_name = call.input(&property_name)?;
        let value = call.input(&value)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_global_state_property(
                map,
                property_name,
                value,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Updates image source coordinates.
    ///
    /// See `mln_map_set_image_source_coordinates` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_coordinates(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_image_source_coordinates(
                map,
                source_id,
                coordinates,
                coordinate_count,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Updates an image source with inline image pixels.
    ///
    /// See `mln_map_set_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_image(
        &self,
        source_id: &str,
        image: &PremultipliedRgba8Image<'_>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let image = call.reference(&image)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_image_source_image(map, source_id, image, completion, out_diagnostic)
        })
    }

    /// Updates an image source to load its image from a URL.
    ///
    /// See `mln_map_set_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_url(
        &self,
        source_id: &str,
        url: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_image_source_url(map, source_id, url, completion, out_diagnostic)
        })
    }

    /// Submits axonometric rendering options.
    ///
    /// See `mln_map_set_projection_mode` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_projection_mode(
        &self,
        mode: &ProjectionMode,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let mode = call.reference(&mode)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_projection_mode(map, mode, completion, out_diagnostic)
        })
    }

    /// Submits a rendering-stats visibility command.
    ///
    /// See `mln_map_set_rendering_stats_view_enabled` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_rendering_stats_view_enabled(
        &self,
        enabled: bool,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_rendering_stats_view_enabled(map, enabled, completion, out_diagnostic)
        })
    }

    /// Sets one runtime style image.
    ///
    /// See `mln_map_set_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_image(
        &self,
        image_id: &str,
        image: &PremultipliedRgba8Image<'_>,
        options: Option<&StyleImageOptions>,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let image_id = call.input(&image_id)?;
        let image = call.reference(&image)?;
        let options = call.optional_reference(options.as_ref())?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_image(map, image_id, image, options, completion, out_diagnostic)
        })
    }

    /// Queues an inline style JSON command. A committed command means the map
    /// applied the JSON, not that the style loaded: style loading events report
    /// the resource and parse results that follow.
    ///
    /// See `mln_map_set_style_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_style_json(&self, json: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let json = call.input(&json)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_json(map, json, completion, out_diagnostic)
        })
    }

    /// Sets one layer property using its MapLibre style-spec property name.
    ///
    /// See `mln_map_set_style_layer_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let layer_id = call.input(&layer_id)?;
        let property_name = call.input(&property_name)?;
        let value = call.input(&value)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_layer_property(
                map,
                layer_id,
                property_name,
                value,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets the style light from a style-spec light JSON object.
    ///
    /// See `mln_map_set_style_light_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_light_json(
        &self,
        light_json: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let light_json = call.input(&light_json)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_light_json(map, light_json, completion, out_diagnostic)
        })
    }

    /// Sets one style light property using its MapLibre style-spec property
    /// name.
    ///
    /// See `mln_map_set_style_light_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_light_property(
        &self,
        property_name: &str,
        value: &str,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let property_name = call.input(&property_name)?;
        let value = call.input(&value)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_light_property(
                map,
                property_name,
                value,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets whether one style source stores fetched tiles in the persistent
    /// cache.
    ///
    /// See `mln_map_set_style_source_volatile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_source_volatile(
        &self,
        source_id: &str,
        is_volatile: bool,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_source_volatile(
                map,
                source_id,
                is_volatile,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets the style's global transition options.
    ///
    /// See `mln_map_set_style_transition_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_transition_options(
        &self,
        options: &StyleTransitionOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_transition_options(map, options, completion, out_diagnostic)
        })
    }

    /// Queues a style URL command. A committed command means the map accepted
    /// the URL, not that the style loaded: style loading events report the
    /// network, decode, and parse results that follow.
    ///
    /// See `mln_map_set_style_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_style_url(&self, url: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let url = call.input(&url)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_url(map, url, completion, out_diagnostic)
        })
    }

    /// Submits a copied tile-options command.
    ///
    /// See `mln_map_set_tile_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_tile_options(
        &self,
        options: &MapTileOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_tile_options(map, options, completion, out_diagnostic)
        })
    }

    /// Submits a copied viewport-options command.
    ///
    /// See `mln_map_set_viewport_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_viewport_options(
        &self,
        options: &MapViewportOptions,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let options = call.reference(&options)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_viewport_options(map, options, completion, out_diagnostic)
        })
    }

    /// Submits one atomic camera update.
    ///
    /// See `mln_map_update_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn update_camera(&self, update: &CameraUpdate) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let update = call.reference(&update)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_update_camera(map, update, completion, out_diagnostic)
        })
    }
}

/// Makes [`MapHandle`] calls in queue mode on a [`DeliveryQueue`]. The queue
/// holds each call's completion handler, which receives the result when
/// a drain of the queue runs it, and the handlers of the registrations
/// that the call carries. A call fails with `Status::InvalidState` once
/// the queue is closed.
pub struct MapHandleQueued<'a, K: QueueKind>(&'a MapHandle, &'a DeliveryQueue<K>);
impl MapHandle {
    pub fn queued<'a, K: QueueKind>(
        &'a self,
        queue: &'a DeliveryQueue<K>,
    ) -> MapHandleQueued<'a, K> {
        MapHandleQueued(self, queue)
    }
}
impl<K: QueueKind> MapHandleQueued<'_, K> {
    /// Adds a custom geometry source. The callback may run on any native worker
    /// thread, concurrently with runtime and map calls.
    ///
    /// See `mln_map_add_custom_geometry_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_custom_geometry_source(
        &self,
        source_id: &str,
        options: CustomGeometrySourceOptions<K>,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_add_custom_geometry_source(
                    map,
                    source_id,
                    options,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Adds a custom vector source. The callback may run on any native worker
    /// thread, concurrently with runtime and map calls.
    ///
    /// See `mln_map_add_custom_vector_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_custom_vector_source(
        &self,
        source_id: &str,
        options: CustomVectorSourceOptions<K>,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_add_custom_vector_source(
                    map,
                    source_id,
                    options,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Adds a GeoJSON source with prepared inline data.
    ///
    /// See `mln_map_add_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_geo_json_source_data(
        &self,
        source_id: &str,
        data: &GeoJsonSourceDataHandle,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let data = data.inner.native();
        let source_id = call.input(&source_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_add_geo_json_source_data(
                    map,
                    source_id,
                    data,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Adds a GeoJSON source with URL data.
    ///
    /// See `mln_map_add_geo_json_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_geo_json_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&GeoJsonSourceOptions<'_>>,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        let options = call.optional_reference(options.as_ref())?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_add_geo_json_source_url(
                    map,
                    source_id,
                    url,
                    options,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Adds an image source with inline image pixels.
    ///
    /// See `mln_map_add_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_image_source_image(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
        image: &PremultipliedRgba8Image<'_>,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        let image = call.reference(&image)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_add_image_source_image(
                    map,
                    source_id,
                    coordinates,
                    coordinate_count,
                    image,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Adds an image source that loads its image from a URL.
    ///
    /// See `mln_map_add_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_image_source_url(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
        url: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        let url = call.input(&url)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_add_image_source_url(
                    map,
                    source_id,
                    coordinates,
                    coordinate_count,
                    url,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Adds one style layer from a full style-spec layer JSON object.
    ///
    /// See `mln_map_add_style_layer_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_style_layer_json(
        &self,
        layer_json: &str,
        before_layer_id: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let layer_json = call.input(&layer_json)?;
        let before_layer_id = call.input(&before_layer_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_add_style_layer_json(
                    map,
                    layer_json,
                    before_layer_id,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Adds one style source from a style-spec source JSON object.
    ///
    /// See `mln_map_add_style_source_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_style_source_json(
        &self,
        source_id: &str,
        source_json: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let source_json = call.input(&source_json)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_add_style_source_json(
                    map,
                    source_id,
                    source_json,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Submits one atomic relative camera update.
    ///
    /// See `mln_map_apply_camera_delta` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn apply_camera_delta(
        &self,
        delta: &CameraDelta<K>,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let delta = call.reference(&delta)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_apply_camera_delta(map, delta, completion, out_diagnostic)
            },
        )
    }

    /// Starts attachment of a texture target that renders on a graphics device.
    ///
    /// See `mln_map_attach_graphics_device_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
    pub fn attach_graphics_device_texture(
        &self,
        device: &GraphicsDeviceHandle,
        descriptor: &GraphicsDeviceTextureDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let device = device.inner.native();
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_graphics_device_texture(
                    map,
                    device,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a ring of caller-owned Metal textures.
    ///
    /// See `mln_map_attach_metal_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "ios",
        target_os = "macos",
        target_os = "tvos",
        target_os = "visionos"
    ))]
    pub unsafe fn attach_metal_borrowed_texture(
        &self,
        descriptor: &MetalBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_metal_borrowed_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a session-owned Metal texture ring.
    ///
    /// See `mln_map_attach_metal_owned_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "ios",
        target_os = "macos",
        target_os = "tvos",
        target_os = "visionos"
    ))]
    pub unsafe fn attach_metal_owned_texture(
        &self,
        descriptor: &MetalOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_metal_owned_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a Metal surface target.
    ///
    /// See `mln_map_attach_metal_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "ios",
        target_os = "macos",
        target_os = "tvos",
        target_os = "visionos"
    ))]
    pub unsafe fn attach_metal_surface(
        &self,
        descriptor: &MetalSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_metal_surface(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a ring of caller-owned OpenGL textures.
    ///
    /// See `mln_map_attach_open_gl_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn attach_open_gl_borrowed_texture(
        &self,
        descriptor: &OpenGlBorrowedTextureDescriptor<'_>,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_open_gl_borrowed_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a session-owned OpenGL texture ring.
    ///
    /// See `mln_map_attach_open_gl_owned_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn attach_open_gl_owned_texture(
        &self,
        descriptor: &OpenGlOwnedTextureDescriptor<'_>,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_open_gl_owned_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of an OpenGL surface target.
    ///
    /// See `mln_map_attach_open_gl_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn attach_open_gl_surface(
        &self,
        descriptor: &OpenGlSurfaceDescriptor<'_>,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_open_gl_surface(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a ring of caller-owned Vulkan images.
    ///
    /// See `mln_map_attach_vulkan_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "android",
        target_os = "freebsd",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub unsafe fn attach_vulkan_borrowed_texture(
        &self,
        descriptor: &VulkanBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_vulkan_borrowed_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a session-owned Vulkan texture ring.
    ///
    /// See `mln_map_attach_vulkan_owned_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "android",
        target_os = "freebsd",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub unsafe fn attach_vulkan_owned_texture(
        &self,
        descriptor: &VulkanOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_vulkan_owned_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a Vulkan surface target.
    ///
    /// See `mln_map_attach_vulkan_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "android",
        target_os = "freebsd",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub unsafe fn attach_vulkan_surface(
        &self,
        descriptor: &VulkanSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_vulkan_surface(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a ring of caller-owned WebGPU textures.
    ///
    /// See `mln_map_attach_web_gpu_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn attach_web_gpu_borrowed_texture(
        &self,
        descriptor: &WebGpuBorrowedTextureDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_web_gpu_borrowed_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a session-owned WebGPU texture ring.
    ///
    /// See `mln_map_attach_web_gpu_owned_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn attach_web_gpu_owned_texture(
        &self,
        descriptor: &WebGpuOwnedTextureDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_web_gpu_owned_texture(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Starts attachment of a WebGPU surface target.
    ///
    /// See `mln_map_attach_web_gpu_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn attach_web_gpu_surface(
        &self,
        descriptor: &WebGpuSurfaceDescriptor,
        options: &RenderSessionAttachOptions,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<RenderSessionHandle> {
        let mut call = self.0.inner.call().queue(self.1);
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_web_gpu_surface(
                    map,
                    descriptor,
                    options,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
        )?;
        Ok(RenderSessionHandle::adopt(out_session)?)
    }

    /// Begins a command group, which holds this map's render updates until the
    /// group ends.
    ///
    /// See `mln_map_begin_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn begin_command_group(
        &self,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_begin_command_group(map, completion, out_diagnostic)
            },
        )
    }

    /// Starts an ordered query for a camera that fits a GeoJSON geometry.
    ///
    /// See `mln_map_camera_for_geometry` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_for_geometry(
        &self,
        geometry: &str,
        fit_options: Option<&CameraFitOptions>,
        on_complete: impl CompletionHandler<K, delivery::Copied<sys::mln_camera_options, CameraOptions>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let geometry = call.input(&geometry)?;
        let fit_options = call.optional_reference(fit_options.as_ref())?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_for_geometry(
                    map,
                    geometry,
                    fit_options,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Starts an ordered query for a camera that fits geographic bounds.
    ///
    /// See `mln_map_camera_for_lat_lng_bounds` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_for_lat_lng_bounds(
        &self,
        bounds: LatLngBounds,
        fit_options: Option<&CameraFitOptions>,
        on_complete: impl CompletionHandler<K, delivery::Copied<sys::mln_camera_options, CameraOptions>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let bounds = call.input(&bounds)?;
        let fit_options = call.optional_reference(fit_options.as_ref())?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_for_lat_lng_bounds(
                    map,
                    bounds,
                    fit_options,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Starts an ordered query for a camera that fits geographic coordinates.
    ///
    /// See `mln_map_camera_for_lat_lngs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn camera_for_lat_lngs(
        &self,
        coordinates: &[LatLng],
        fit_options: Option<&CameraFitOptions>,
        on_complete: impl CompletionHandler<K, delivery::Copied<sys::mln_camera_options, CameraOptions>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let coordinate_count = convert::count(coordinates.len())?;
        let coordinates = call.array(coordinates)?;
        let fit_options = call.optional_reference(fit_options.as_ref())?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_camera_for_lat_lngs(
                    map,
                    coordinates,
                    coordinate_count,
                    fit_options,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Cancels the camera transitions of the commands whose animation carried
    /// transition_id, and leaves every other transition running.
    ///
    /// See `mln_map_cancel_camera_transition` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_camera_transition(
        &self,
        transition_id: u64,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_cancel_camera_transition(
                    map,
                    transition_id,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Cancels the camera transitions running when this command commits.
    ///
    /// See `mln_map_cancel_transitions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_transitions(
        &self,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_cancel_transitions(map, completion, out_diagnostic)
            },
        )
    }

    /// Submits an ordered debug-log command.
    ///
    /// See `mln_map_dump_debug_logs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn dump_debug_logs(
        &self,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_dump_debug_logs(map, completion, out_diagnostic)
            },
        )
    }

    /// Ends the innermost command group that `MapHandle::begin_command_group()`
    /// began.
    ///
    /// See `mln_map_end_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn end_command_group(
        &self,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_end_command_group(map, completion, out_diagnostic)
            },
        )
    }

    /// Starts an ordered camera read.
    ///
    /// See `mln_map_get_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn get_camera(
        &self,
        on_complete: impl CompletionHandler<
            K,
            delivery::Copied<sys::mln_camera_query_result, CameraQueryResult>,
        >,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_camera(map, completion, out_diagnostic)
            },
        )
    }

    /// Starts an ordered read of per-feature state from this map.
    ///
    /// See `mln_map_get_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_feature_state(
        &self,
        selector: &FeatureStateSelector<'_>,
        on_complete: impl CompletionHandler<K, delivery::Copied<sys::mln_buffer_view, String>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let selector = call.reference(&selector)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_feature_state(map, selector, completion, out_diagnostic)
            },
        )
    }

    /// Queries the global-state JSON object, including style defaults.
    ///
    /// See `mln_map_get_global_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_global_state(
        &self,
        on_complete: impl CompletionHandler<K, delivery::Copied<sys::mln_buffer_view, String>>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_global_state(map, completion, out_diagnostic)
            },
        )
    }

    /// Copies image source coordinates.
    ///
    /// See `mln_map_get_image_source_coordinates` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_image_source_coordinates(
        &self,
        source_id: &str,
        on_complete: impl CompletionHandler<K, delivery::CopiedList<sys::mln_lat_lng, LatLng>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_image_source_coordinates(
                    map,
                    source_id,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Starts an ordered copy of the last successfully parsed style document.
    ///
    /// See `mln_map_get_loaded_style_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_loaded_style_json(
        &self,
        on_complete: impl CompletionHandler<K, delivery::Copied<sys::mln_buffer_view, String>>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_loaded_style_json(map, completion, out_diagnostic)
            },
        )
    }

    /// Copies one complete runtime style image.
    ///
    /// See `mln_map_get_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_image(
        &self,
        image_id: &str,
        on_complete: impl CompletionHandler<
            K,
            delivery::CopiedOptional<sys::mln_style_image_info, StyleImageInfo>,
        >,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let image_id = call.input(&image_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_image(map, image_id, completion, out_diagnostic)
            },
        )
    }

    /// Copies the complete metadata of one style layer.
    ///
    /// See `mln_map_get_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_layer(
        &self,
        layer_id: &str,
        on_complete: impl CompletionHandler<
            K,
            delivery::CopiedOptional<sys::mln_style_layer_info, StyleLayerInfo>,
        >,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let layer_id = call.input(&layer_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_layer(map, layer_id, completion, out_diagnostic)
            },
        )
    }

    /// Serializes one style layer as a full style-spec layer JSON object.
    ///
    /// See `mln_map_get_style_layer_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_layer_json(
        &self,
        layer_id: &str,
        on_complete: impl CompletionHandler<K, delivery::CopiedOptional<sys::mln_buffer_view, String>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let layer_id = call.input(&layer_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_layer_json(map, layer_id, completion, out_diagnostic)
            },
        )
    }

    /// Serializes one layer property as a style-spec JSON value.
    ///
    /// See `mln_map_get_style_layer_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        on_complete: impl CompletionHandler<K, delivery::CopiedOptional<sys::mln_buffer_view, String>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let layer_id = call.input(&layer_id)?;
        let property_name = call.input(&property_name)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_layer_property(
                    map,
                    layer_id,
                    property_name,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Serializes one style light property as a style-spec JSON value.
    ///
    /// See `mln_map_get_style_light_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_light_property(
        &self,
        property_name: &str,
        on_complete: impl CompletionHandler<K, delivery::CopiedOptional<sys::mln_buffer_view, String>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let property_name = call.input(&property_name)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_light_property(
                    map,
                    property_name,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Copies the complete metadata of one style source.
    ///
    /// See `mln_map_get_style_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_source(
        &self,
        source_id: &str,
        on_complete: impl CompletionHandler<
            K,
            delivery::CopiedOptional<sys::mln_style_source_info, StyleSourceInfo>,
        >,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_source(map, source_id, completion, out_diagnostic)
            },
        )
    }

    /// Reads the style's global transition options.
    ///
    /// See `mln_map_get_style_transition_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn get_style_transition_options(
        &self,
        on_complete: impl CompletionHandler<
            K,
            delivery::Copied<sys::mln_style_transition_options, StyleTransitionOptions>,
        >,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_transition_options(map, completion, out_diagnostic)
            },
        )
    }

    /// Starts an ordered copy of the last requested style URL.
    ///
    /// See `mln_map_get_style_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_style_url(
        &self,
        on_complete: impl CompletionHandler<K, delivery::Copied<sys::mln_buffer_view, String>>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_get_style_url(map, completion, out_diagnostic)
            },
        )
    }

    /// Invalidates custom geometry source data inside one geographic region.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_geometry_source_region(
        &self,
        source_id: &str,
        bounds: LatLngBounds,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let bounds = call.input(&bounds)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_invalidate_custom_geometry_source_region(
                    map,
                    source_id,
                    bounds,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Invalidates custom geometry source data for one canonical tile.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_invalidate_custom_geometry_source_tile(
                    map,
                    source_id,
                    tile_id,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Invalidates custom vector source data for one canonical tile.
    ///
    /// See `mln_map_invalidate_custom_vector_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_vector_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_invalidate_custom_vector_source_tile(
                    map,
                    source_id,
                    tile_id,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Starts an ordered wrapped-bounds query for a copied camera.
    ///
    /// See `mln_map_lat_lng_bounds_for_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lng_bounds_for_camera(
        &self,
        camera: &CameraOptions,
        on_complete: impl CompletionHandler<K, delivery::Copied<sys::mln_lat_lng_bounds, LatLngBounds>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let camera = call.reference(&camera)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lng_bounds_for_camera(map, camera, completion, out_diagnostic)
            },
        )
    }

    /// Starts an ordered unwrapped-bounds query for a copied camera.
    ///
    /// See `mln_map_lat_lng_bounds_for_camera_unwrapped` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn lat_lng_bounds_for_camera_unwrapped(
        &self,
        camera: &CameraOptions,
        on_complete: impl CompletionHandler<K, delivery::Copied<sys::mln_lat_lng_bounds, LatLngBounds>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let camera = call.reference(&camera)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_lat_lng_bounds_for_camera_unwrapped(
                    map,
                    camera,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Starts an ordered query of every style layer in style order.
    ///
    /// See `mln_map_list_style_layers` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn list_style_layers(
        &self,
        on_complete: impl CompletionHandler<
            K,
            delivery::CopiedList<sys::mln_style_layer_info, StyleLayerInfo>,
        >,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_list_style_layers(map, completion, out_diagnostic)
            },
        )
    }

    /// Lists every style source in style order.
    ///
    /// See `mln_map_list_style_sources` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn list_style_sources(
        &self,
        on_complete: impl CompletionHandler<
            K,
            delivery::CopiedList<sys::mln_style_source_info, StyleSourceInfo>,
        >,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_list_style_sources(map, completion, out_diagnostic)
            },
        )
    }

    /// Moves one style layer before another layer or to the top.
    ///
    /// See `mln_map_move_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn move_style_layer(
        &self,
        layer_id: &str,
        before_layer_id: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let layer_id = call.input(&layer_id)?;
        let before_layer_id = call.input(&before_layer_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_move_style_layer(
                    map,
                    layer_id,
                    before_layer_id,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Removes per-feature state from this map.
    ///
    /// See `mln_map_remove_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn remove_feature_state(
        &self,
        selector: &FeatureStateSelector<'_>,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let selector = call.reference(&selector)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_remove_feature_state(map, selector, completion, out_diagnostic)
            },
        )
    }

    /// Removes one runtime style image by ID.
    ///
    /// See `mln_map_remove_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_image(
        &self,
        image_id: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let image_id = call.input(&image_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_remove_style_image(map, image_id, completion, out_diagnostic)
            },
        )
    }

    /// Removes one style layer by ID.
    ///
    /// See `mln_map_remove_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_layer(
        &self,
        layer_id: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let layer_id = call.input(&layer_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_remove_style_layer(map, layer_id, completion, out_diagnostic)
            },
        )
    }

    /// Removes one style source by ID.
    ///
    /// See `mln_map_remove_style_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_source(
        &self,
        source_id: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_remove_style_source(map, source_id, completion, out_diagnostic)
            },
        )
    }

    /// Requests a repaint for a continuous map.
    ///
    /// See `mln_map_request_repaint` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn request_repaint(
        &self,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_request_repaint(map, completion, out_diagnostic)
            },
        )
    }

    /// Requests one still image for a static or tile map, which renders itself.
    ///
    /// See `mln_map_request_still_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn request_still_image(
        &self,
        on_complete: impl CompletionHandler<K, delivery::Unit>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_request_still_image(map, completion, out_diagnostic)
            },
        )
    }

    /// Submits the sole post-creation logical extent update.
    ///
    /// See `mln_map_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn resize(
        &self,
        extent: LogicalExtent,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let extent = call.input(&extent)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_resize(map, extent, completion, out_diagnostic)
            },
        )
    }

    /// Submits a copied camera-constraint command.
    ///
    /// See `mln_map_set_bounds` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_bounds(
        &self,
        options: &BoundOptions,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_bounds(map, options, completion, out_diagnostic)
            },
        )
    }

    /// Sets custom geometry source data for one canonical tile.
    ///
    /// See `mln_map_set_custom_geometry_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_geometry_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let data = call.input(&data)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_custom_geometry_source_tile_data(
                    map,
                    source_id,
                    tile_id,
                    data,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Reports a custom geometry source error for one canonical tile.
    ///
    /// See `mln_map_set_custom_geometry_source_tile_error` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_geometry_source_tile_error(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        message: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let message = call.input(&message)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_custom_geometry_source_tile_error(
                    map,
                    source_id,
                    tile_id,
                    message,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Sets custom vector source data for one canonical tile.
    ///
    /// See `mln_map_set_custom_vector_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_vector_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        encoding: StyleVectorTileEncoding,
        data: &[u8],
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let data = call.input(&data)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_custom_vector_source_tile_data(
                    map,
                    source_id,
                    tile_id,
                    encoding.as_raw(),
                    data,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Reports a custom vector source error for one canonical tile.
    ///
    /// See `mln_map_set_custom_vector_source_tile_error` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_vector_source_tile_error(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        message: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let message = call.input(&message)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_custom_vector_source_tile_error(
                    map,
                    source_id,
                    tile_id,
                    message,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Submits a debug-overlay command.
    ///
    /// See `mln_map_set_debug_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_debug_options(
        &self,
        options: MapDebugFlag,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_debug_options(map, options.bits(), completion, out_diagnostic)
            },
        )
    }

    /// Selects which map-originated event types this map queues.
    ///
    /// See `mln_map_set_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_event_mask(
        &self,
        mask: RuntimeEventFlag,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_event_mask(map, mask.bits(), completion, out_diagnostic)
            },
        )
    }

    /// Submits a copied per-feature-state command.
    ///
    /// See `mln_map_set_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_feature_state(
        &self,
        selector: &FeatureStateSelector<'_>,
        state: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let selector = call.reference(&selector)?;
        let state = call.input(&state)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_feature_state(map, selector, state, completion, out_diagnostic)
            },
        )
    }

    /// Submits a copied free-camera command.
    ///
    /// See `mln_map_set_free_camera_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_free_camera_options(
        &self,
        options: &FreeCameraOptions,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_free_camera_options(map, options, completion, out_diagnostic)
            },
        )
    }

    /// Updates one GeoJSON source with prepared inline data.
    ///
    /// See `mln_map_set_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geo_json_source_data(
        &self,
        source_id: &str,
        data: &GeoJsonSourceDataHandle,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let data = data.inner.native();
        let source_id = call.input(&source_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_geo_json_source_data(
                    map,
                    source_id,
                    data,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Overrides one GeoJSON source's synchronous tiling at runtime.
    ///
    /// See `mln_map_set_geo_json_source_synchronous_tiling` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geo_json_source_synchronous_tiling(
        &self,
        source_id: &str,
        enabled: bool,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_geo_json_source_synchronous_tiling(
                    map,
                    source_id,
                    enabled,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Updates one GeoJSON source to load data from a URL.
    ///
    /// See `mln_map_set_geo_json_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geo_json_source_url(
        &self,
        source_id: &str,
        url: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_geo_json_source_url(
                    map,
                    source_id,
                    url,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Submits a global-state JSON value. JSON null restores the style default.
    ///
    /// See `mln_map_set_global_state_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_global_state_property(
        &self,
        property_name: &str,
        value: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let property_name = call.input(&property_name)?;
        let value = call.input(&value)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_global_state_property(
                    map,
                    property_name,
                    value,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Updates image source coordinates.
    ///
    /// See `mln_map_set_image_source_coordinates` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_coordinates(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_image_source_coordinates(
                    map,
                    source_id,
                    coordinates,
                    coordinate_count,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Updates an image source with inline image pixels.
    ///
    /// See `mln_map_set_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_image(
        &self,
        source_id: &str,
        image: &PremultipliedRgba8Image<'_>,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let image = call.reference(&image)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_image_source_image(
                    map,
                    source_id,
                    image,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Updates an image source to load its image from a URL.
    ///
    /// See `mln_map_set_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_url(
        &self,
        source_id: &str,
        url: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_image_source_url(map, source_id, url, completion, out_diagnostic)
            },
        )
    }

    /// Submits axonometric rendering options.
    ///
    /// See `mln_map_set_projection_mode` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_projection_mode(
        &self,
        mode: &ProjectionMode,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let mode = call.reference(&mode)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_projection_mode(map, mode, completion, out_diagnostic)
            },
        )
    }

    /// Submits a rendering-stats visibility command.
    ///
    /// See `mln_map_set_rendering_stats_view_enabled` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_rendering_stats_view_enabled(
        &self,
        enabled: bool,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let call = self.0.inner.call().queue(self.1);
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_rendering_stats_view_enabled(
                    map,
                    enabled,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Sets one runtime style image.
    ///
    /// See `mln_map_set_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_image(
        &self,
        image_id: &str,
        image: &PremultipliedRgba8Image<'_>,
        options: Option<&StyleImageOptions>,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let image_id = call.input(&image_id)?;
        let image = call.reference(&image)?;
        let options = call.optional_reference(options.as_ref())?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_style_image(
                    map,
                    image_id,
                    image,
                    options,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Queues an inline style JSON command. A committed command means the map
    /// applied the JSON, not that the style loaded: style loading events report
    /// the resource and parse results that follow.
    ///
    /// See `mln_map_set_style_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_style_json(
        &self,
        json: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let json = call.input(&json)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_style_json(map, json, completion, out_diagnostic)
            },
        )
    }

    /// Sets one layer property using its MapLibre style-spec property name.
    ///
    /// See `mln_map_set_style_layer_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let layer_id = call.input(&layer_id)?;
        let property_name = call.input(&property_name)?;
        let value = call.input(&value)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_style_layer_property(
                    map,
                    layer_id,
                    property_name,
                    value,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Sets the style light from a style-spec light JSON object.
    ///
    /// See `mln_map_set_style_light_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_light_json(
        &self,
        light_json: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let light_json = call.input(&light_json)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_style_light_json(map, light_json, completion, out_diagnostic)
            },
        )
    }

    /// Sets one style light property using its MapLibre style-spec property
    /// name.
    ///
    /// See `mln_map_set_style_light_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_light_property(
        &self,
        property_name: &str,
        value: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let property_name = call.input(&property_name)?;
        let value = call.input(&value)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_style_light_property(
                    map,
                    property_name,
                    value,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Sets whether one style source stores fetched tiles in the persistent
    /// cache.
    ///
    /// See `mln_map_set_style_source_volatile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_source_volatile(
        &self,
        source_id: &str,
        is_volatile: bool,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let source_id = call.input(&source_id)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_style_source_volatile(
                    map,
                    source_id,
                    is_volatile,
                    completion,
                    out_diagnostic,
                )
            },
        )
    }

    /// Sets the style's global transition options.
    ///
    /// See `mln_map_set_style_transition_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_transition_options(
        &self,
        options: &StyleTransitionOptions,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_style_transition_options(map, options, completion, out_diagnostic)
            },
        )
    }

    /// Queues a style URL command. A committed command means the map accepted
    /// the URL, not that the style loaded: style loading events report the
    /// network, decode, and parse results that follow.
    ///
    /// See `mln_map_set_style_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_style_url(
        &self,
        url: &str,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let url = call.input(&url)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_style_url(map, url, completion, out_diagnostic)
            },
        )
    }

    /// Submits a copied tile-options command.
    ///
    /// See `mln_map_set_tile_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_tile_options(
        &self,
        options: &MapTileOptions,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_tile_options(map, options, completion, out_diagnostic)
            },
        )
    }

    /// Submits a copied viewport-options command.
    ///
    /// See `mln_map_set_viewport_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_viewport_options(
        &self,
        options: &MapViewportOptions,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let options = call.reference(&options)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_set_viewport_options(map, options, completion, out_diagnostic)
            },
        )
    }

    /// Submits one atomic camera update.
    ///
    /// See `mln_map_update_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn update_camera(
        &self,
        update: &CameraUpdate<K>,
        on_complete: impl CompletionHandler<K, delivery::Command>,
    ) -> Result<()> {
        let mut call = self.0.inner.call().queue(self.1);
        let update = call.reference(&update)?;
        self.1.enqueue(
            call,
            on_complete,
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_update_camera(map, update, completion, out_diagnostic)
            },
        )
    }
}

/// Submits [`MapHandle`] commands without a completion. Each call returns
/// the same acceptance status as its completing form, and native logs a
/// command that later fails as a warning through the log handler.
pub struct MapHandleDiscarding<'a>(&'a MapHandle);
impl MapHandle {
    pub fn discarding(&self) -> MapHandleDiscarding<'_> {
        MapHandleDiscarding(self)
    }
}
impl MapHandleDiscarding<'_> {
    /// Adds a custom geometry source. The callback may run on any native worker
    /// thread, concurrently with runtime and map calls.
    ///
    /// See `mln_map_add_custom_geometry_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_custom_geometry_source(
        &self,
        source_id: &str,
        options: CustomGeometrySourceOptions,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let options = call.reference(&options)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_custom_geometry_source(
                map,
                source_id,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a custom vector source. The callback may run on any native worker
    /// thread, concurrently with runtime and map calls.
    ///
    /// See `mln_map_add_custom_vector_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_custom_vector_source(
        &self,
        source_id: &str,
        options: CustomVectorSourceOptions,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let options = call.reference(&options)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_custom_vector_source(
                map,
                source_id,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds a GeoJSON source with prepared inline data.
    ///
    /// See `mln_map_add_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_geo_json_source_data(
        &self,
        source_id: &str,
        data: &GeoJsonSourceDataHandle,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let data = data.inner.native();
        let source_id = call.input(&source_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_geo_json_source_data(map, source_id, data, completion, out_diagnostic)
        })
    }

    /// Adds a GeoJSON source with URL data.
    ///
    /// See `mln_map_add_geo_json_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_geo_json_source_url(
        &self,
        source_id: &str,
        url: &str,
        options: Option<&GeoJsonSourceOptions<'_>>,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        let options = call.optional_reference(options.as_ref())?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_geo_json_source_url(
                map,
                source_id,
                url,
                options,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds an image source with inline image pixels.
    ///
    /// See `mln_map_add_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_image_source_image(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
        image: &PremultipliedRgba8Image<'_>,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        let image = call.reference(&image)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_image_source_image(
                map,
                source_id,
                coordinates,
                coordinate_count,
                image,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds an image source that loads its image from a URL.
    ///
    /// See `mln_map_add_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_image_source_url(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
        url: &str,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        let url = call.input(&url)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_image_source_url(
                map,
                source_id,
                coordinates,
                coordinate_count,
                url,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds one style layer from a full style-spec layer JSON object.
    ///
    /// See `mln_map_add_style_layer_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_style_layer_json(&self, layer_json: &str, before_layer_id: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let layer_json = call.input(&layer_json)?;
        let before_layer_id = call.input(&before_layer_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_style_layer_json(
                map,
                layer_json,
                before_layer_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Adds one style source from a style-spec source JSON object.
    ///
    /// See `mln_map_add_style_source_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn add_style_source_json(&self, source_id: &str, source_json: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let source_json = call.input(&source_json)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_add_style_source_json(
                map,
                source_id,
                source_json,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Submits one atomic relative camera update.
    ///
    /// See `mln_map_apply_camera_delta` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn apply_camera_delta(&self, delta: &CameraDelta) -> Result<()> {
        let mut call = self.0.inner.call();
        let delta = call.reference(&delta)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_apply_camera_delta(map, delta, completion, out_diagnostic)
        })
    }

    /// Begins a command group, which holds this map's render updates until the
    /// group ends.
    ///
    /// See `mln_map_begin_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn begin_command_group(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_begin_command_group(map, completion, out_diagnostic)
        })
    }

    /// Cancels the camera transitions of the commands whose animation carried
    /// transition_id, and leaves every other transition running.
    ///
    /// See `mln_map_cancel_camera_transition` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_camera_transition(&self, transition_id: u64) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_cancel_camera_transition(map, transition_id, completion, out_diagnostic)
        })
    }

    /// Cancels the camera transitions running when this command commits.
    ///
    /// See `mln_map_cancel_transitions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_transitions(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_cancel_transitions(map, completion, out_diagnostic)
        })
    }

    /// Submits an ordered debug-log command.
    ///
    /// See `mln_map_dump_debug_logs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn dump_debug_logs(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_dump_debug_logs(map, completion, out_diagnostic)
        })
    }

    /// Ends the innermost command group that `MapHandle::begin_command_group()`
    /// began.
    ///
    /// See `mln_map_end_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn end_command_group(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_end_command_group(map, completion, out_diagnostic)
        })
    }

    /// Invalidates custom geometry source data inside one geographic region.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_geometry_source_region(
        &self,
        source_id: &str,
        bounds: LatLngBounds,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let bounds = call.input(&bounds)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_region(
                map,
                source_id,
                bounds,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Invalidates custom geometry source data for one canonical tile.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_geometry_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_invalidate_custom_geometry_source_tile(
                map,
                source_id,
                tile_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Invalidates custom vector source data for one canonical tile.
    ///
    /// See `mln_map_invalidate_custom_vector_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn invalidate_custom_vector_source_tile(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_invalidate_custom_vector_source_tile(
                map,
                source_id,
                tile_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Moves one style layer before another layer or to the top.
    ///
    /// See `mln_map_move_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn move_style_layer(&self, layer_id: &str, before_layer_id: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let layer_id = call.input(&layer_id)?;
        let before_layer_id = call.input(&before_layer_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_move_style_layer(
                map,
                layer_id,
                before_layer_id,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Removes per-feature state from this map.
    ///
    /// See `mln_map_remove_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn remove_feature_state(&self, selector: &FeatureStateSelector<'_>) -> Result<()> {
        let mut call = self.0.inner.call();
        let selector = call.reference(&selector)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_feature_state(map, selector, completion, out_diagnostic)
        })
    }

    /// Removes one runtime style image by ID.
    ///
    /// See `mln_map_remove_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_image(&self, image_id: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let image_id = call.input(&image_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_image(map, image_id, completion, out_diagnostic)
        })
    }

    /// Removes one style layer by ID.
    ///
    /// See `mln_map_remove_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_layer(&self, layer_id: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let layer_id = call.input(&layer_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_layer(map, layer_id, completion, out_diagnostic)
        })
    }

    /// Removes one style source by ID.
    ///
    /// See `mln_map_remove_style_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn remove_style_source(&self, source_id: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_source(map, source_id, completion, out_diagnostic)
        })
    }

    /// Requests a repaint for a continuous map.
    ///
    /// See `mln_map_request_repaint` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn request_repaint(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_request_repaint(map, completion, out_diagnostic)
        })
    }

    /// Submits the sole post-creation logical extent update.
    ///
    /// See `mln_map_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn resize(&self, extent: LogicalExtent) -> Result<()> {
        let mut call = self.0.inner.call();
        let extent = call.input(&extent)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_resize(map, extent, completion, out_diagnostic)
        })
    }

    /// Submits a copied camera-constraint command.
    ///
    /// See `mln_map_set_bounds` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_bounds(&self, options: &BoundOptions) -> Result<()> {
        let mut call = self.0.inner.call();
        let options = call.reference(&options)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_bounds(map, options, completion, out_diagnostic)
        })
    }

    /// Sets custom geometry source data for one canonical tile.
    ///
    /// See `mln_map_set_custom_geometry_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_geometry_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        data: &str,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let data = call.input(&data)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_geometry_source_tile_data(
                map,
                source_id,
                tile_id,
                data,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Reports a custom geometry source error for one canonical tile.
    ///
    /// See `mln_map_set_custom_geometry_source_tile_error` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_geometry_source_tile_error(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        message: &str,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let message = call.input(&message)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_geometry_source_tile_error(
                map,
                source_id,
                tile_id,
                message,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets custom vector source data for one canonical tile.
    ///
    /// See `mln_map_set_custom_vector_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_vector_source_tile_data(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        encoding: StyleVectorTileEncoding,
        data: &[u8],
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let data = call.input(&data)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_vector_source_tile_data(
                map,
                source_id,
                tile_id,
                encoding.as_raw(),
                data,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Reports a custom vector source error for one canonical tile.
    ///
    /// See `mln_map_set_custom_vector_source_tile_error` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_custom_vector_source_tile_error(
        &self,
        source_id: &str,
        tile_id: CanonicalTileId,
        message: &str,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let tile_id = call.input(&tile_id)?;
        let message = call.input(&message)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_custom_vector_source_tile_error(
                map,
                source_id,
                tile_id,
                message,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Submits a debug-overlay command.
    ///
    /// See `mln_map_set_debug_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_debug_options(&self, options: MapDebugFlag) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_debug_options(map, options.bits(), completion, out_diagnostic)
        })
    }

    /// Selects which map-originated event types this map queues.
    ///
    /// See `mln_map_set_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_event_mask(&self, mask: RuntimeEventFlag) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_event_mask(map, mask.bits(), completion, out_diagnostic)
        })
    }

    /// Submits a copied per-feature-state command.
    ///
    /// See `mln_map_set_feature_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_feature_state(
        &self,
        selector: &FeatureStateSelector<'_>,
        state: &str,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let selector = call.reference(&selector)?;
        let state = call.input(&state)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_feature_state(map, selector, state, completion, out_diagnostic)
        })
    }

    /// Submits a copied free-camera command.
    ///
    /// See `mln_map_set_free_camera_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_free_camera_options(&self, options: &FreeCameraOptions) -> Result<()> {
        let mut call = self.0.inner.call();
        let options = call.reference(&options)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_free_camera_options(map, options, completion, out_diagnostic)
        })
    }

    /// Updates one GeoJSON source with prepared inline data.
    ///
    /// See `mln_map_set_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geo_json_source_data(
        &self,
        source_id: &str,
        data: &GeoJsonSourceDataHandle,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let data = data.inner.native();
        let source_id = call.input(&source_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_geo_json_source_data(map, source_id, data, completion, out_diagnostic)
        })
    }

    /// Overrides one GeoJSON source's synchronous tiling at runtime.
    ///
    /// See `mln_map_set_geo_json_source_synchronous_tiling` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geo_json_source_synchronous_tiling(
        &self,
        source_id: &str,
        enabled: bool,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_geo_json_source_synchronous_tiling(
                map,
                source_id,
                enabled,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Updates one GeoJSON source to load data from a URL.
    ///
    /// See `mln_map_set_geo_json_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_geo_json_source_url(&self, source_id: &str, url: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_geo_json_source_url(map, source_id, url, completion, out_diagnostic)
        })
    }

    /// Submits a global-state JSON value. JSON null restores the style default.
    ///
    /// See `mln_map_set_global_state_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_global_state_property(&self, property_name: &str, value: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let property_name = call.input(&property_name)?;
        let value = call.input(&value)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_global_state_property(
                map,
                property_name,
                value,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Updates image source coordinates.
    ///
    /// See `mln_map_set_image_source_coordinates` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_coordinates(
        &self,
        source_id: &str,
        coordinates: &[LatLng],
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let coordinate_count = convert::count(coordinates.len())?;
        let source_id = call.input(&source_id)?;
        let coordinates = call.array(coordinates)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_image_source_coordinates(
                map,
                source_id,
                coordinates,
                coordinate_count,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Updates an image source with inline image pixels.
    ///
    /// See `mln_map_set_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_image(
        &self,
        source_id: &str,
        image: &PremultipliedRgba8Image<'_>,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let image = call.reference(&image)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_image_source_image(map, source_id, image, completion, out_diagnostic)
        })
    }

    /// Updates an image source to load its image from a URL.
    ///
    /// See `mln_map_set_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_image_source_url(&self, source_id: &str, url: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        let url = call.input(&url)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_image_source_url(map, source_id, url, completion, out_diagnostic)
        })
    }

    /// Submits axonometric rendering options.
    ///
    /// See `mln_map_set_projection_mode` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_projection_mode(&self, mode: &ProjectionMode) -> Result<()> {
        let mut call = self.0.inner.call();
        let mode = call.reference(&mode)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_projection_mode(map, mode, completion, out_diagnostic)
        })
    }

    /// Submits a rendering-stats visibility command.
    ///
    /// See `mln_map_set_rendering_stats_view_enabled` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_rendering_stats_view_enabled(&self, enabled: bool) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_rendering_stats_view_enabled(map, enabled, completion, out_diagnostic)
        })
    }

    /// Sets one runtime style image.
    ///
    /// See `mln_map_set_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_image(
        &self,
        image_id: &str,
        image: &PremultipliedRgba8Image<'_>,
        options: Option<&StyleImageOptions>,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let image_id = call.input(&image_id)?;
        let image = call.reference(&image)?;
        let options = call.optional_reference(options.as_ref())?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_image(map, image_id, image, options, completion, out_diagnostic)
        })
    }

    /// Queues an inline style JSON command. A committed command means the map
    /// applied the JSON, not that the style loaded: style loading events report
    /// the resource and parse results that follow.
    ///
    /// See `mln_map_set_style_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_style_json(&self, json: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let json = call.input(&json)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_json(map, json, completion, out_diagnostic)
        })
    }

    /// Sets one layer property using its MapLibre style-spec property name.
    ///
    /// See `mln_map_set_style_layer_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_layer_property(
        &self,
        layer_id: &str,
        property_name: &str,
        value: &str,
    ) -> Result<()> {
        let mut call = self.0.inner.call();
        let layer_id = call.input(&layer_id)?;
        let property_name = call.input(&property_name)?;
        let value = call.input(&value)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_layer_property(
                map,
                layer_id,
                property_name,
                value,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets the style light from a style-spec light JSON object.
    ///
    /// See `mln_map_set_style_light_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_light_json(&self, light_json: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let light_json = call.input(&light_json)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_light_json(map, light_json, completion, out_diagnostic)
        })
    }

    /// Sets one style light property using its MapLibre style-spec property
    /// name.
    ///
    /// See `mln_map_set_style_light_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_light_property(&self, property_name: &str, value: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let property_name = call.input(&property_name)?;
        let value = call.input(&value)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_light_property(
                map,
                property_name,
                value,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets whether one style source stores fetched tiles in the persistent
    /// cache.
    ///
    /// See `mln_map_set_style_source_volatile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_source_volatile(&self, source_id: &str, is_volatile: bool) -> Result<()> {
        let mut call = self.0.inner.call();
        let source_id = call.input(&source_id)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_source_volatile(
                map,
                source_id,
                is_volatile,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Sets the style's global transition options.
    ///
    /// See `mln_map_set_style_transition_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    pub fn set_style_transition_options(&self, options: &StyleTransitionOptions) -> Result<()> {
        let mut call = self.0.inner.call();
        let options = call.reference(&options)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_transition_options(map, options, completion, out_diagnostic)
        })
    }

    /// Queues a style URL command. A committed command means the map accepted
    /// the URL, not that the style loaded: style loading events report the
    /// network, decode, and parse results that follow.
    ///
    /// See `mln_map_set_style_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_style_url(&self, url: &str) -> Result<()> {
        let mut call = self.0.inner.call();
        let url = call.input(&url)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_url(map, url, completion, out_diagnostic)
        })
    }

    /// Submits a copied tile-options command.
    ///
    /// See `mln_map_set_tile_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_tile_options(&self, options: &MapTileOptions) -> Result<()> {
        let mut call = self.0.inner.call();
        let options = call.reference(&options)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_tile_options(map, options, completion, out_diagnostic)
        })
    }

    /// Submits a copied viewport-options command.
    ///
    /// See `mln_map_set_viewport_options` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_viewport_options(&self, options: &MapViewportOptions) -> Result<()> {
        let mut call = self.0.inner.call();
        let options = call.reference(&options)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_viewport_options(map, options, completion, out_diagnostic)
        })
    }

    /// Submits one atomic camera update.
    ///
    /// See `mln_map_update_camera` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn update_camera(&self, update: &CameraUpdate) -> Result<()> {
        let mut call = self.0.inner.call();
        let update = call.reference(&update)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_update_camera(map, update, completion, out_diagnostic)
        })
    }
}
