// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A map, with its style and camera.
    ///
    /// See `mln_map` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct MapHandle(mln_map) dispose |raw| crate::error::check(|out_diagnostic| unsafe { sys::mln_map_dispose(raw, out_diagnostic) });
    /// Returns the native handle value, which runtime events report as their source.
    fn id;
}

impl MapHandle {
    /// Adds a source whose GeoJSON tiles come from callbacks.
    ///
    /// See `mln_map_add_custom_geometry_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidArgument` when the style already has a source named
    ///   `source_id`.
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

    /// Adds a source whose vector tiles come from callbacks.
    ///
    /// See `mln_map_add_custom_vector_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidArgument` when the style already has a source named
    ///   `source_id`.
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

    /// Adds a GeoJSON source with prepared data.
    ///
    /// The source uses the options that `data` was prepared with. `data` can be
    /// released once this function returns.
    ///
    /// See `mln_map_add_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidArgument` when the style already has a source named
    ///   `source_id`.
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

    /// Adds a GeoJSON source that loads its data from `url`.
    ///
    /// See `mln_map_add_geo_json_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidArgument` when the style already has a source named
    ///   `source_id`.
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

    /// Adds an image source that shows `image`.
    ///
    /// `coordinates` are the image's four corners, clockwise from the top left.
    ///
    /// See `mln_map_add_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidArgument` when the style already has a source named
    ///   `source_id`.
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

    /// Adds an image source that loads its image from `url`.
    ///
    /// `coordinates` are the image's four corners, clockwise from the top left.
    ///
    /// See `mln_map_add_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidArgument` when the style already has a source named
    ///   `source_id`.
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

    /// Adds a layer to the map's style.
    ///
    /// `layer_json` is a
    /// \[layer\](https://maplibre.org/maplibre-style-spec/layers/) object. The
    /// layer goes below the layer named `before_layer_id`, or above every layer
    /// when `before_layer_id` is empty.
    ///
    /// See `mln_map_add_style_layer_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no layer named
    ///   `before_layer_id`, or no source with the ID that the layer uses.
    /// - `Status::InvalidArgument` when the style already has a layer with the
    ///   new layer's ID.
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

    /// Adds a source to the map's style.
    ///
    /// `source_json` is a
    /// \[source\](https://maplibre.org/maplibre-style-spec/sources/) object.
    ///
    /// See `mln_map_add_style_source_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidArgument` when the style already has a source named
    ///   `source_id`.
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

    /// Moves the camera relative to where it is, as a gesture does.
    ///
    /// `delta` can pan the map's content by an offset on the screen, scale the
    /// zoom, and add to the bearing and pitch. An anchor keeps one screen point
    /// in place while the scale, bearing, or pitch changes. An anchor needs one
    /// of those changes, and combines with an offset only when the delta has no
    /// animation. The function finishes when the map applies the delta, before
    /// an animation ends.
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

    /// Creates a session that renders the map into a ring of your Metal
    /// textures.
    ///
    /// See `mln_map_attach_metal_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when `supported_render_backend_mask()` lacks
    ///   `RenderBackendFlag::METAL`, or when the target supports none of the
    ///   `producer_sync_kinds` in `options`.
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

    /// Creates a session that renders the map to a Metal layer.
    ///
    /// See `mln_map_attach_metal_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when `supported_render_backend_mask()` lacks
    ///   `RenderBackendFlag::METAL`.
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

    /// Attaches a render session that renders the map offscreen.
    ///
    /// Read each image's pixels with `RenderSessionHandle::read_texture()`.
    ///
    /// The session renders on `device`. When `device` is null, the session
    /// creates a device with the default options, and releases it when the
    /// session ends.
    ///
    /// See `mln_map_attach_offscreen` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidState` when the map already has a render session.
    /// - `Status::NativeError` when `device` is null and the platform can't
    ///   create a device.
    pub fn attach_offscreen(
        &self,
        descriptor: &OffscreenDescriptor,
        options: &RenderSessionAttachOptions,
        device: Option<&OffscreenDeviceHandle>,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let device = device.map_or(sys::mln_offscreen_device(0), |device| device.inner.native());
        let mut out_session = sys::mln_render_session(0);
        let descriptor = call.reference(&descriptor)?;
        let options = call.reference(&options)?;
        let future = call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_attach_offscreen(
                    map,
                    descriptor,
                    options,
                    device,
                    &mut out_session,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((RenderSessionHandle::adopt(out_session)?, future))
    }

    /// Creates a session that renders the map into a ring of your OpenGL
    /// textures.
    ///
    /// See `mln_map_attach_open_gl_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when `open_gl_supported_context_provider_mask()`
    ///   lacks the context's platform, or when the target supports none of the
    ///   `producer_sync_kinds` in `options`.
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

    /// Creates a session that renders the map to an OpenGL surface.
    ///
    /// See `mln_map_attach_open_gl_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when `open_gl_supported_context_provider_mask()`
    ///   lacks the context's platform.
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

    /// Creates a session that renders the map into a ring of your Vulkan
    /// images.
    ///
    /// See `mln_map_attach_vulkan_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when `supported_render_backend_mask()` lacks
    ///   `RenderBackendFlag::VULKAN`, or when the target supports none of the
    ///   `producer_sync_kinds` in `options`.
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

    /// Creates a session that renders the map to a Vulkan surface.
    ///
    /// See `mln_map_attach_vulkan_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when `supported_render_backend_mask()` lacks
    ///   `RenderBackendFlag::VULKAN`.
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

    /// Creates a session that renders the map into a ring of your WebGPU
    /// textures.
    ///
    /// See `mln_map_attach_web_gpu_borrowed_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when `supported_render_backend_mask()` lacks
    ///   `RenderBackendFlag::WEB_GPU`, or when the target supports none of the
    ///   `producer_sync_kinds` in `options`.
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

    /// Creates a session that renders the map to a WebGPU surface.
    ///
    /// See `mln_map_attach_web_gpu_surface` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when `supported_render_backend_mask()` lacks
    ///   `RenderBackendFlag::WEB_GPU`.
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

    /// Holds back the map's changes from rendering until
    /// `MapHandle::end_command_group()`, so they show in the same frame.
    ///
    /// Groups nest, and the changes render when the outermost group ends.
    ///
    /// See `mln_map_begin_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn begin_command_group(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_begin_command_group(map, completion, out_diagnostic)
        })
    }

    /// Computes a camera that fits a GeoJSON geometry in the map's view.
    ///
    /// `geometry` is a GeoJSON geometry object with at least one coordinate,
    /// not a feature. The camera keeps the map's bearing and pitch unless
    /// `fit_options` sets them, and its zoom stays within the map's zoom
    /// limits.
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

    /// Computes a camera that fits `bounds` in the map's view.
    ///
    /// The camera keeps the map's bearing and pitch unless `fit_options` sets
    /// them, and its zoom stays within the map's zoom limits.
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

    /// Computes a camera that fits `coordinates` in the map's view.
    ///
    /// `coordinates` must not be empty. The camera keeps the map's bearing and
    /// pitch unless `fit_options` sets them, and its zoom stays within the
    /// map's zoom limits.
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

    /// Stops the running camera animations whose animation options set
    /// `transition_id`.
    ///
    /// Other animations keep running. The end handler of each stopped animation
    /// reports `CameraTransitionOutcome::Cancelled`.
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

    /// Stops every running camera animation where it is.
    ///
    /// The end handler of each stopped animation reports
    /// `CameraTransitionOutcome::Cancelled`.
    ///
    /// See `mln_map_cancel_transitions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_transitions(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_cancel_transitions(map, completion, out_diagnostic)
        })
    }

    /// Releases the map in any state, and finishes in the background.
    ///
    /// Use it from destructors and finalizers. Otherwise, use
    /// `MapHandle::release()`.
    ///
    /// See `mln_map_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn dispose(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|map| {
            let mut call = Call::new(map);
            call.status(|map, out_diagnostic| unsafe { sys::mln_map_dispose(map, out_diagnostic) })
        })
    }

    /// Logs debugging information about the map's style and sources.
    ///
    /// See `mln_map_dump_debug_logs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_dump_debug_logs(map, completion, out_diagnostic)
        })
    }

    /// Ends the group that `MapHandle::begin_command_group()` began.
    ///
    /// See `mln_map_end_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn end_command_group(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_end_command_group(map, completion, out_diagnostic)
        })
    }

    /// Reads the map's camera.
    ///
    /// The query reads the camera after the map applies the commands sent
    /// before it.
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

    /// Reads the state of a feature, as a JSON object.
    ///
    /// The object is empty when the feature has no state.
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

    /// Reads the global state of the map's style as a JSON object.
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

    /// Reads the corners of an image source's image, clockwise from the top
    /// left.
    ///
    /// See `mln_map_get_image_source_coordinates` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't an
    ///   image source.
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

    /// Reads the JSON of the style that the map loaded.
    ///
    /// The JSON doesn't include the changes made to the style since it loaded.
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

    /// Copies the map's state.
    ///
    /// The copy doesn't wait for pending commands. To read the camera after
    /// they apply, use `MapHandle::get_camera()`.
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

    /// Reads an image of the map's style.
    ///
    /// Completes with no value when the style has no image named `image_id`.
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

    /// Reads a layer of the map's style.
    ///
    /// Completes with no value when the style has no layer named `layer_id`.
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

    /// Reads a layer of the map's style as a
    /// \[layer\](https://maplibre.org/maplibre-style-spec/layers/) object.
    ///
    /// Completes with no value when the style has no layer named `layer_id`.
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

    /// Reads a property of a layer in the map's style.
    ///
    /// Completes with no value when the layer doesn't set the property.
    ///
    /// See `mln_map_get_style_layer_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no layer named `layer_id`.
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

    /// Reads a property of the light of the map's style.
    ///
    /// Completes with no value when the light doesn't set the property.
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

    /// Reads a source of the map's style.
    ///
    /// Completes with no value when the style has no source named `source_id`.
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

    /// Reads the default transition of the map's style.
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

    /// Reads the URL of the map's style.
    ///
    /// The URL is empty when the style came from `MapHandle::set_style_json()`.
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

    /// Makes a custom geometry source ask again for the tiles in `bounds` that
    /// the map still needs.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   custom geometry source.
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

    /// Makes a custom geometry source ask for a tile again when the map needs
    /// it.
    ///
    /// See `mln_map_invalidate_custom_geometry_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   custom geometry source.
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

    /// Makes a custom vector source ask for a tile again when the map needs it.
    ///
    /// See `mln_map_invalidate_custom_vector_source_tile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   custom vector source.
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

    /// Computes the area that the map would show with `camera`.
    ///
    /// Camera fields that `camera` doesn't set come from the map's camera. The
    /// longitudes stay between -180 and 180.
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

    /// Computes the area that the map would show with `camera`, without
    /// wrapping longitudes.
    ///
    /// Camera fields that `camera` doesn't set come from the map's camera. The
    /// longitudes can fall outside -180 to 180, so a view across the
    /// antimeridian gives one continuous range.
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

    /// Lists the layers of the map's style, from bottom to top.
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

    /// Lists the sources of the map's style.
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

    /// Moves a layer of the map's style below the layer named
    /// `before_layer_id`, or above every layer when `before_layer_id` is empty.
    ///
    /// See `mln_map_move_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no layer named `layer_id` or
    ///   `before_layer_id`.
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

    /// Releases the map.
    ///
    /// Detach the map's render session first.
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

    /// Removes the feature state that `selector` selects.
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

    /// Removes an image from the map's style.
    ///
    /// See `mln_map_remove_style_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no image named `image_id`.
    pub fn remove_style_image(&self, image_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let image_id = call.input(&image_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_image(map, image_id, completion, out_diagnostic)
        })
    }

    /// Removes a layer from the map's style.
    ///
    /// See `mln_map_remove_style_layer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no layer named `layer_id`.
    pub fn remove_style_layer(&self, layer_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let layer_id = call.input(&layer_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_layer(map, layer_id, completion, out_diagnostic)
        })
    }

    /// Removes a source from the map's style.
    ///
    /// See `mln_map_remove_style_source` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidState` when a layer still uses the source.
    pub fn remove_style_source(&self, source_id: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_remove_style_source(map, source_id, completion, out_diagnostic)
        })
    }

    /// Renders a new frame of a continuous map, even when nothing changed.
    ///
    /// See `mln_map_request_repaint` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn request_repaint(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_request_repaint(map, completion, out_diagnostic)
        })
    }

    /// Renders a still image of a static or tile map.
    ///
    /// The map's render session draws the image once every resource that the
    /// image needs has loaded.
    ///
    /// See `mln_map_request_still_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Cancelled` when the map is released before the image is done.
    /// - `Status::NativeError` when the style or a resource that the image
    ///   needs fails to load.
    pub fn request_still_image(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |map, completion, out_diagnostic| unsafe {
                sys::mln_map_request_still_image(map, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Sets the map's size.
    ///
    /// A map with a render session takes its size from the session, so resize
    /// the session with `RenderSessionHandle::resize()` instead.
    ///
    /// See `mln_map_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidState` when the map has a render session.
    /// - `Status::InvalidState` when a render session attached to the map
    ///   before the resize applied.
    pub fn resize(&self, logical_size: LogicalSize) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let logical_size = call.input(&logical_size)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_resize(map, logical_size, completion, out_diagnostic)
        })
    }

    /// Limits the camera's center to an area, and its zoom and pitch to ranges.
    ///
    /// Limits that `options` doesn't set keep their current values. When the
    /// camera is outside the new limits, it moves inside them.
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

    /// Sets the data of a tile in a custom geometry source.
    ///
    /// `data` is a GeoJSON feature collection. The source ignores data for a
    /// tile that it hasn't asked for, or has since cancelled.
    ///
    /// See `mln_map_set_custom_geometry_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   custom geometry source.
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

    /// Reports that a tile of a custom geometry source failed to load.
    ///
    /// See `mln_map_set_custom_geometry_source_tile_error` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   custom geometry source.
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

    /// Sets the data of a tile in a custom vector source.
    ///
    /// `data` is the tile, encoded as `encoding`. The source ignores data for a
    /// tile that it hasn't asked for, or has since cancelled.
    ///
    /// See `mln_map_set_custom_vector_source_tile_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   custom vector source.
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

    /// Reports that a tile of a custom vector source failed to load.
    ///
    /// See `mln_map_set_custom_vector_source_tile_error` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   custom vector source.
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

    /// Draws debugging overlays, such as tile borders, on the map.
    ///
    /// `flags` replaces the overlays set before.
    ///
    /// See `mln_map_set_debug_flags` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_debug_flags(&self, flags: MapDebugFlag) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_debug_flags(map, flags.bits(), completion, out_diagnostic)
        })
    }

    /// Selects the events that the map reports.
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

    /// Merges `state` into the state of a feature.
    ///
    /// `state` is a JSON object. Its keys replace the same keys in the
    /// feature's state, and the other keys stay.
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

    /// Places the camera at a position and orientation in space.
    ///
    /// The map adjusts them to a camera that it can show, and stops running
    /// camera animations.
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

    /// Replaces the data of a GeoJSON source.
    ///
    /// `data` must have been prepared with the same options as the source. It
    /// can be released once this function returns.
    ///
    /// See `mln_map_set_geo_json_source_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   GeoJSON source, or `data` was prepared with other options.
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

    /// Sets whether a GeoJSON source builds its tiles synchronously.
    ///
    /// This works like the synchronous tiling option of `GeoJsonSourceOptions`,
    /// but `enabled` can't turn off synchronous tiling that the source's
    /// options turned on.
    ///
    /// See `mln_map_set_geo_json_source_synchronous_tiling` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   GeoJSON source.
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

    /// Makes a GeoJSON source load its data from `url`.
    ///
    /// See `mln_map_set_geo_json_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't a
    ///   GeoJSON source.
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

    /// Sets a value that the style's `global-state`
    /// \[expressions\](https://maplibre.org/maplibre-style-spec/expressions/)
    /// read.
    ///
    /// See `mln_map_set_global_state_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidState` until the map has parsed its style, which
    ///   `RuntimeEventType::MapStyleLoaded` reports, and again while it loads a
    ///   new style URL.
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

    /// Moves the image of an image source.
    ///
    /// `coordinates` are the image's four corners, clockwise from the top left.
    ///
    /// See `mln_map_set_image_source_coordinates` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't an
    ///   image source.
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

    /// Replaces the image of an image source.
    ///
    /// See `mln_map_set_image_source_image` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't an
    ///   image source.
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

    /// Makes an image source load its image from `url`.
    ///
    /// See `mln_map_set_image_source_url` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
    /// - `Status::InvalidArgument` when the source named `source_id` isn't an
    ///   image source.
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

    /// Sets whether the map draws in perspective or axonometric projection, and
    /// how much it skews the view.
    ///
    /// Settings that `mode` doesn't set keep their current values.
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

    /// Shows or hides an overlay of rendering statistics on the map.
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

    /// Adds an image to the map's style, or replaces the image named
    /// `image_id`.
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

    /// Loads the map's style from `json`.
    ///
    /// The style's sources, sprites, and glyphs load in the background.
    ///
    /// See `mln_map_set_style_json` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NativeError` when `json` isn't a valid style.
    pub fn set_style_json(&self, json: &str) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let json = call.input(&json)?;
        call.command(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_style_json(map, json, completion, out_diagnostic)
        })
    }

    /// Sets a property of a layer in the map's style.
    ///
    /// `property_name` is one of the layer's layout or paint
    /// \[properties\](https://maplibre.org/maplibre-style-spec/layers/), or
    /// `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null
    /// `value` resets any of these but `source` and `source-layer` to its
    /// default.
    ///
    /// See `mln_map_set_style_layer_property` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no layer named `layer_id`.
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

    /// Replaces the \[light\](https://maplibre.org/maplibre-style-spec/light/)
    /// of the map's style.
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

    /// Sets a property of the light of the map's style.
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

    /// Sets whether a source is volatile.
    ///
    /// The map doesn't store the tiles of a volatile source in the persistent
    /// cache.
    ///
    /// See `mln_map_set_style_source_volatile` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the style has no source named `source_id`.
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

    /// Sets the default transition of the map's style.
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

    /// Loads the map's style from `url`.
    ///
    /// The style loads in the background, and
    /// `RuntimeEventType::MapStyleLoaded` or
    /// `RuntimeEventType::MapLoadingFailed` reports how the load ends. Sources,
    /// layers, and images added before the style loads are lost when it loads.
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

    /// Sets how the map chooses which tiles to load, and at what detail.
    ///
    /// Settings that `options` doesn't set keep their current values.
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

    /// Sets how the map orients and constrains its view.
    ///
    /// Settings that `options` doesn't set keep their current values.
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

    /// Moves the camera, at once or with an animation.
    ///
    /// Camera fields that `update` doesn't set keep their values, and the
    /// update stops any running animation of the fields it sets. The function
    /// finishes when the map applies the update, before an animation ends. To
    /// learn when an animation ends, set an `CameraTransitionHandler` in its
    /// animation options.
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
    /// Adds a source whose GeoJSON tiles come from callbacks.
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

    /// Adds a source whose vector tiles come from callbacks.
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

    /// Adds a GeoJSON source with prepared data.
    ///
    /// The source uses the options that `data` was prepared with. `data` can be
    /// released once this function returns.
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

    /// Adds a GeoJSON source that loads its data from `url`.
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

    /// Adds an image source that shows `image`.
    ///
    /// `coordinates` are the image's four corners, clockwise from the top left.
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

    /// Adds an image source that loads its image from `url`.
    ///
    /// `coordinates` are the image's four corners, clockwise from the top left.
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

    /// Adds a layer to the map's style.
    ///
    /// `layer_json` is a
    /// \[layer\](https://maplibre.org/maplibre-style-spec/layers/) object. The
    /// layer goes below the layer named `before_layer_id`, or above every layer
    /// when `before_layer_id` is empty.
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

    /// Adds a source to the map's style.
    ///
    /// `source_json` is a
    /// \[source\](https://maplibre.org/maplibre-style-spec/sources/) object.
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

    /// Moves the camera relative to where it is, as a gesture does.
    ///
    /// `delta` can pan the map's content by an offset on the screen, scale the
    /// zoom, and add to the bearing and pitch. An anchor keeps one screen point
    /// in place while the scale, bearing, or pitch changes. An anchor needs one
    /// of those changes, and combines with an offset only when the delta has no
    /// animation. The function finishes when the map applies the delta, before
    /// an animation ends.
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

    /// Holds back the map's changes from rendering until
    /// `MapHandle::end_command_group()`, so they show in the same frame.
    ///
    /// Groups nest, and the changes render when the outermost group ends.
    ///
    /// See `mln_map_begin_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn begin_command_group(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_begin_command_group(map, completion, out_diagnostic)
        })
    }

    /// Stops the running camera animations whose animation options set
    /// `transition_id`.
    ///
    /// Other animations keep running. The end handler of each stopped animation
    /// reports `CameraTransitionOutcome::Cancelled`.
    ///
    /// See `mln_map_cancel_camera_transition` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_camera_transition(&self, transition_id: u64) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_cancel_camera_transition(map, transition_id, completion, out_diagnostic)
        })
    }

    /// Stops every running camera animation where it is.
    ///
    /// The end handler of each stopped animation reports
    /// `CameraTransitionOutcome::Cancelled`.
    ///
    /// See `mln_map_cancel_transitions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn cancel_transitions(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_cancel_transitions(map, completion, out_diagnostic)
        })
    }

    /// Logs debugging information about the map's style and sources.
    ///
    /// See `mln_map_dump_debug_logs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn dump_debug_logs(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_dump_debug_logs(map, completion, out_diagnostic)
        })
    }

    /// Ends the group that `MapHandle::begin_command_group()` began.
    ///
    /// See `mln_map_end_command_group` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn end_command_group(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_end_command_group(map, completion, out_diagnostic)
        })
    }

    /// Makes a custom geometry source ask again for the tiles in `bounds` that
    /// the map still needs.
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

    /// Makes a custom geometry source ask for a tile again when the map needs
    /// it.
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

    /// Makes a custom vector source ask for a tile again when the map needs it.
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

    /// Moves a layer of the map's style below the layer named
    /// `before_layer_id`, or above every layer when `before_layer_id` is empty.
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

    /// Removes the feature state that `selector` selects.
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

    /// Removes an image from the map's style.
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

    /// Removes a layer from the map's style.
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

    /// Removes a source from the map's style.
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

    /// Renders a new frame of a continuous map, even when nothing changed.
    ///
    /// See `mln_map_request_repaint` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn request_repaint(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_request_repaint(map, completion, out_diagnostic)
        })
    }

    /// Sets the map's size.
    ///
    /// A map with a render session takes its size from the session, so resize
    /// the session with `RenderSessionHandle::resize()` instead.
    ///
    /// See `mln_map_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidState` when the map has a render session.
    pub fn resize(&self, logical_size: LogicalSize) -> Result<()> {
        let mut call = self.0.inner.call();
        let logical_size = call.input(&logical_size)?;
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_resize(map, logical_size, completion, out_diagnostic)
        })
    }

    /// Limits the camera's center to an area, and its zoom and pitch to ranges.
    ///
    /// Limits that `options` doesn't set keep their current values. When the
    /// camera is outside the new limits, it moves inside them.
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

    /// Sets the data of a tile in a custom geometry source.
    ///
    /// `data` is a GeoJSON feature collection. The source ignores data for a
    /// tile that it hasn't asked for, or has since cancelled.
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

    /// Reports that a tile of a custom geometry source failed to load.
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

    /// Sets the data of a tile in a custom vector source.
    ///
    /// `data` is the tile, encoded as `encoding`. The source ignores data for a
    /// tile that it hasn't asked for, or has since cancelled.
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

    /// Reports that a tile of a custom vector source failed to load.
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

    /// Draws debugging overlays, such as tile borders, on the map.
    ///
    /// `flags` replaces the overlays set before.
    ///
    /// See `mln_map_set_debug_flags` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_debug_flags(&self, flags: MapDebugFlag) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_debug_flags(map, flags.bits(), completion, out_diagnostic)
        })
    }

    /// Selects the events that the map reports.
    ///
    /// See `mln_map_set_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_event_mask(&self, mask: RuntimeEventFlag) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_event_mask(map, mask.bits(), completion, out_diagnostic)
        })
    }

    /// Merges `state` into the state of a feature.
    ///
    /// `state` is a JSON object. Its keys replace the same keys in the
    /// feature's state, and the other keys stay.
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

    /// Places the camera at a position and orientation in space.
    ///
    /// The map adjusts them to a camera that it can show, and stops running
    /// camera animations.
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

    /// Replaces the data of a GeoJSON source.
    ///
    /// `data` must have been prepared with the same options as the source. It
    /// can be released once this function returns.
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

    /// Sets whether a GeoJSON source builds its tiles synchronously.
    ///
    /// This works like the synchronous tiling option of `GeoJsonSourceOptions`,
    /// but `enabled` can't turn off synchronous tiling that the source's
    /// options turned on.
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

    /// Makes a GeoJSON source load its data from `url`.
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

    /// Sets a value that the style's `global-state`
    /// \[expressions\](https://maplibre.org/maplibre-style-spec/expressions/)
    /// read.
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

    /// Moves the image of an image source.
    ///
    /// `coordinates` are the image's four corners, clockwise from the top left.
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

    /// Replaces the image of an image source.
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

    /// Makes an image source load its image from `url`.
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

    /// Sets whether the map draws in perspective or axonometric projection, and
    /// how much it skews the view.
    ///
    /// Settings that `mode` doesn't set keep their current values.
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

    /// Shows or hides an overlay of rendering statistics on the map.
    ///
    /// See `mln_map_set_rendering_stats_view_enabled` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
    pub fn set_rendering_stats_view_enabled(&self, enabled: bool) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|map, completion, out_diagnostic| unsafe {
            sys::mln_map_set_rendering_stats_view_enabled(map, enabled, completion, out_diagnostic)
        })
    }

    /// Adds an image to the map's style, or replaces the image named
    /// `image_id`.
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

    /// Loads the map's style from `json`.
    ///
    /// The style's sources, sprites, and glyphs load in the background.
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

    /// Sets a property of a layer in the map's style.
    ///
    /// `property_name` is one of the layer's layout or paint
    /// \[properties\](https://maplibre.org/maplibre-style-spec/layers/), or
    /// `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null
    /// `value` resets any of these but `source` and `source-layer` to its
    /// default.
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

    /// Replaces the \[light\](https://maplibre.org/maplibre-style-spec/light/)
    /// of the map's style.
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

    /// Sets a property of the light of the map's style.
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

    /// Sets whether a source is volatile.
    ///
    /// The map doesn't store the tiles of a volatile source in the persistent
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

    /// Sets the default transition of the map's style.
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

    /// Loads the map's style from `url`.
    ///
    /// The style loads in the background, and
    /// `RuntimeEventType::MapStyleLoaded` or
    /// `RuntimeEventType::MapLoadingFailed` reports how the load ends. Sources,
    /// layers, and images added before the style loads are lost when it loads.
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

    /// Sets how the map chooses which tiles to load, and at what detail.
    ///
    /// Settings that `options` doesn't set keep their current values.
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

    /// Sets how the map orients and constrains its view.
    ///
    /// Settings that `options` doesn't set keep their current values.
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

    /// Moves the camera, at once or with an animation.
    ///
    /// Camera fields that `update` doesn't set keep their values, and the
    /// update stops any running animation of the fields it sets. The function
    /// finishes when the map applies the update, before an animation ends. To
    /// learn when an animation ends, set an `CameraTransitionHandler` in its
    /// animation options.
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
