// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A render session draws a map into a surface or a texture.
    ///
    /// See `mln_render_session` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct RenderSessionHandle(mln_render_session) dispose |raw| crate::error::check(|out_diagnostic| unsafe { sys::mln_render_session_dispose(raw, std::ptr::null(), out_diagnostic) });
    /// Returns the native handle value.
    fn id;
}

impl RenderSessionHandle {
    /// Stops the session right away, without running its pending graphics work.
    ///
    /// Use it when the target's graphics objects are going away and a detach
    /// can't finish first. Pending work completes with `Status::TargetLost`,
    /// and pending frame demands finish with `RenderResult::TargetNotReady`.
    /// Once the call returns, the library no longer uses your graphics objects,
    /// and you can attach another session to the map. Dispose the session
    /// afterward.
    ///
    /// Only the driver thread can destroy OpenGL and WebGPU objects, so abandon
    /// keeps them until the process exits, reports how many it kept, and logs a
    /// warning. Detach the session instead to free them. A disposal that
    /// another thread starts first retires the session itself, and the abandon
    /// then fails as it does for any released session.
    ///
    /// See `mln_render_session_abandon` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn abandon(&self) -> Result<RenderAbandonResult> {
        let mut call = self.inner.call();
        let mut out_result: sys::mln_render_abandon_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_abandon_result>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_abandon(session, &mut out_result, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_result) }?)
    }

    /// Takes the newest rendered frame from the session's texture ring, if one
    /// is ready.
    ///
    /// Older frames that you haven't acquired are dropped, and the session can
    /// render over a frame before you acquire it. While you hold a frame in
    /// every texture of the ring, new frames wait until you release one. When
    /// no frame is ready, it succeeds and writes the null handle.
    ///
    /// See `mln_render_session_acquire_frame` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when the session lacks
    ///   `RenderSessionCapabilityFlag::FRAME_ACQUISITION`.
    /// - `RenderSessionSnapshot::failure_status`, `Status::TargetLost` or
    ///   `Status::NativeError`, when the session has failed. The diagnostic
    ///   carries the failure's message.
    /// - `Status::InvalidState` when you detached, abandoned, or disposed the
    ///   session.
    pub fn acquire_frame(&self) -> Result<Option<AcquiredFrameHandle>> {
        let mut call = self.inner.call();
        let mut out_frame = sys::mln_acquired_frame(0);
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_acquire_frame(session, &mut out_frame, out_diagnostic)
        })?;
        if out_frame.0 == 0 {
            return Ok(None);
        }
        Ok(Some(AcquiredFrameHandle::adopt(out_frame)?))
    }

    /// Ends the session's pending frame demands and waits for its earlier work.
    ///
    /// Frame demands that haven't started, including those that wait for a map
    /// update, finish with `RenderResult::NoUpdate`.
    ///
    /// See `mln_render_session_barrier` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session loses its target before the
    ///   barrier runs.
    pub fn barrier(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_barrier(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Clears the data that the session's renderer holds, such as loaded tiles,
    /// and draws the map again.
    ///
    /// See `mln_render_session_clear_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session loses its target before the work
    ///   runs.
    pub fn clear_data(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_clear_data(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Asks the session to render a frame.
    ///
    /// You can call it once the attach call returns. While the attach runs, the
    /// demand waits for it to complete. Every demand finishes with one frame
    /// result that carries its token, and
    /// `RenderSessionHandle::drain_frame_results()` takes it. A demand without
    /// `FrameDemandFlag::FORCE` replaces each pending demand with the same
    /// flags.
    ///
    /// See `mln_render_session_demand_frame` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `RenderSessionSnapshot::failure_status`, `Status::TargetLost` or
    ///   `Status::NativeError`, when the session has failed. The diagnostic
    ///   carries the failure's message.
    /// - `Status::InvalidState` when you detached, abandoned, or disposed the
    ///   session, or discarded a frame from every texture of its ring.
    pub fn demand_frame(&self, demand: &FrameDemand) -> Result<()> {
        let mut call = self.inner.call();
        let demand = call.reference(&demand)?;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_demand_frame(session, demand, out_diagnostic)
        })?;
        Ok(())
    }

    /// Detaches the session from its map and frees its graphics objects.
    ///
    /// Release every acquired frame first. Pending frame demands finish with
    /// `RenderResult::TargetNotReady`. Once the detach completes, the library
    /// no longer uses your graphics objects, and you can attach another session
    /// to the map. Dispose the session afterward.
    ///
    /// See `mln_render_session_detach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session loses its target before the
    ///   detach runs.
    pub fn detach(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_detach(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Releases the session in any state.
    ///
    /// An attached session detaches first, and frees its graphics objects,
    /// unless you hold one of its frames. A session driven by
    /// `RenderDriverKind::CallerGraphicsThread` detaches at your next
    /// `RenderSessionHandle::service_driver_work()`, so keep servicing it until
    /// the disposal completes. If you release or dispose its map before that
    /// service, the session is abandoned instead. Any other session is
    /// abandoned as `RenderSessionHandle::abandon()` does. Once the disposal
    /// completes, the library no longer uses your graphics objects.
    ///
    /// See `mln_render_session_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn release(
        self,
    ) -> std::result::Result<(NativeFuture<()>, RetiringRenderSession), (Self, Error)> {
        self.consume(|session| {
            let call = Call::new(session);
            call.complete(
                |session, completion, out_diagnostic| unsafe {
                    sys::mln_render_session_dispose(session, completion, out_diagnostic)
                },
                completion::unit,
            )
            .map(|value| (value, RetiringRenderSession(session)))
        })
    }

    /// Releases the session in any state.
    ///
    /// An attached session detaches first, and frees its graphics objects,
    /// unless you hold one of its frames. A session driven by
    /// `RenderDriverKind::CallerGraphicsThread` detaches at your next
    /// `RenderSessionHandle::service_driver_work()`, so keep servicing it until
    /// the disposal completes. If you release or dispose its map before that
    /// service, the session is abandoned instead. Any other session is
    /// abandoned as `RenderSessionHandle::abandon()` does. Once the disposal
    /// completes, the library no longer uses your graphics objects.
    ///
    /// See `mln_render_session_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn dispose(self) -> std::result::Result<RetiringRenderSession, (Self, Error)> {
        self.consume(|session| {
            let call = Call::new(session);
            call.discard(|session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_dispose(session, completion, out_diagnostic)
            })
            .map(|()| RetiringRenderSession(session))
        })
    }

    /// Takes the frame results that the session has queued.
    ///
    /// When no result is queued, it succeeds and writes the null handle and an
    /// empty view.
    ///
    /// See `mln_render_session_drain_frame_results` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn drain_frame_results(&self) -> Result<Vec<RenderFrameResult>> {
        let mut call = self.inner.call();
        let mut out_batch = sys::mln_render_frame_batch(0);
        let mut out_view: sys::mln_render_frame_batch_view = unsafe { std::mem::zeroed() };
        out_view.size = std::mem::size_of::<sys::mln_render_frame_batch_view>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_drain_frame_results(
                session,
                &mut out_batch,
                &mut out_view,
                out_diagnostic,
            )
        })?;
        if out_batch.0 == 0 {
            return Ok(Vec::new());
        }
        // SAFETY: native published the batch to this call alone.
        let _batch =
            unsafe { crate::handle::Drained::new(out_batch, sys::mln_render_frame_batch_release) };
        Ok(unsafe {
            convert::copy_strided(
                out_view.results,
                out_view.result_count,
                out_view.result_size,
            )
        }?)
    }

    /// Logs debugging information about the session's renderer.
    ///
    /// See `mln_render_session_dump_debug_logs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session loses its target before the work
    ///   runs.
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_dump_debug_logs(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Reads what the session supports.
    ///
    /// See `mln_render_session_get_capabilities` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn get_capabilities(&self) -> Result<RenderSessionCapabilities> {
        let mut call = self.inner.call();
        let mut out_capabilities: sys::mln_render_session_capabilities =
            unsafe { std::mem::zeroed() };
        out_capabilities.size = std::mem::size_of::<sys::mln_render_session_capabilities>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_get_capabilities(session, &mut out_capabilities, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_capabilities) }?)
    }

    /// Reads the message of the error that made the session fail.
    ///
    /// `RenderSessionSnapshot::failure_status` reports the error's status. The
    /// message is empty while the session hasn't failed, and doesn't change
    /// once it fails.
    ///
    /// See `mln_render_session_get_failure` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn get_failure(&self) -> Result<String> {
        let mut call = self.inner.call();
        let mut out_message = Vec::new();
        call.status(|session, out_diagnostic| unsafe {
            convert::caller_sized_text(
                &mut out_message,
                |out_message, message_capacity, out_message_length| {
                    sys::mln_render_session_get_failure(
                        session,
                        out_message,
                        message_capacity,
                        out_message_length,
                        out_diagnostic,
                    )
                },
            )
        })?;
        Ok(convert::utf8(out_message)?)
    }

    /// Reads the session's current state.
    ///
    /// See `mln_render_session_get_snapshot` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn get_snapshot(&self) -> Result<RenderSessionSnapshot> {
        let mut call = self.inner.call();
        let mut out_snapshot: sys::mln_render_session_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_render_session_snapshot>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_get_snapshot(session, &mut out_snapshot, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_snapshot) }?)
    }

    /// Reads a value that a source computes for one of its features.
    ///
    /// A clustered GeoJSON source answers the `supercluster` extension for a
    /// cluster feature. Its `extension_field` is `children` for the cluster's
    /// child features, `leaves` for the points in the cluster, or
    /// `expansion-zoom` for the zoom level where the cluster splits. For
    /// `leaves`, `arguments` can set `limit` and `offset`. Without `limit`, the
    /// result has up to 10 points and ignores `offset`. The result is JSON: a
    /// GeoJSON feature collection, a number, or null when the source has no
    /// answer.
    ///
    /// See `mln_render_session_query_feature_extensions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    pub fn query_feature_extensions(
        &self,
        source_id: &str,
        feature: &str,
        extension: &str,
        extension_field: &str,
        arguments: &str,
    ) -> Result<NativeFuture<String>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let feature = call.input(&feature)?;
        let extension = call.input(&extension)?;
        let extension_field = call.input(&extension_field)?;
        let arguments = call.input(&arguments)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_query_feature_extensions(
                    session,
                    source_id,
                    feature,
                    extension,
                    extension_field,
                    arguments,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Finds the features that the session drew in an area of the screen.
    ///
    /// The query reads the last frame that the session rendered. A feature that
    /// several layers draw appears once for each layer.
    ///
    /// See `mln_render_session_query_rendered_features` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    pub fn query_rendered_features(
        &self,
        geometry: &RenderedQueryGeometry,
        options: Option<&RenderedFeatureQueryOptions<'_>>,
    ) -> Result<NativeFuture<Vec<RenderedFeature>>> {
        let mut call = self.inner.call();
        let geometry = call.reference(&geometry)?;
        let options = call.optional_reference(options.as_ref())?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_query_rendered_features(
                    session,
                    geometry,
                    options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_rendered_feature, _>,
        )
    }

    /// Finds the features of the source named `source_id`.
    ///
    /// The query covers only the tiles that the source has loaded for the
    /// current view. A source loads tiles only while a visible layer uses it.
    ///
    /// See `mln_render_session_query_source_features` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    pub fn query_source_features(
        &self,
        source_id: &str,
        options: Option<&SourceFeatureQueryOptions<'_>>,
    ) -> Result<NativeFuture<Vec<SourceFeature>>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let options = call.optional_reference(options.as_ref())?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_query_source_features(
                    session,
                    source_id,
                    options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_source_feature, _>,
        )
    }

    /// Reads the pixels of the last frame that the session rendered.
    ///
    /// See `mln_render_session_read_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when the session's capabilities lack
    ///   `RenderSessionCapabilityFlag::READBACK`.
    /// - `Status::InvalidState` when the session hasn't rendered a frame since
    ///   it attached, resized, or changed its target.
    pub fn read_texture(&self) -> Result<NativeFuture<TextureReadbackResult>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_read_texture(session, completion, out_diagnostic)
            },
            completion::value::<sys::mln_texture_readback_result, _>,
        )
    }

    /// Frees memory that the session's renderer can rebuild, such as caches.
    ///
    /// See `mln_render_session_reduce_memory_use` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session loses its target before the work
    ///   runs.
    pub fn reduce_memory_use(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_reduce_memory_use(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Lets go of the session's surface or textures, and keeps the rest of the
    /// session.
    ///
    /// Use it when the platform takes a surface away, such as when your app
    /// goes to the background. Release every acquired frame first. Until you
    /// give the session a new target, frame demands finish with
    /// `RenderResult::TargetNotReady`. Once the release completes, you can
    /// destroy the old target.
    ///
    /// See `mln_render_session_release_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when the session renders offscreen.
    /// - `Status::Unsupported` when the target is a WebGL canvas.
    /// - `Status::TargetLost` when the session loses its target before the
    ///   release runs.
    pub fn release_target(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_release_target(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Resizes the session's target and its map.
    ///
    /// The session renders at the map's scale factor. Release every acquired
    /// frame first.
    ///
    /// See `mln_render_session_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when the session renders into a borrowed ring.
    ///   Give it new textures of the new size instead.
    /// - `Status::TargetLost` when the session loses its target before the
    ///   resize applies.
    pub fn resize(&self, logical_size: LogicalSize) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let logical_size = call.input(&logical_size)?;
        call.command(|session, completion, out_diagnostic| unsafe {
            sys::mln_render_session_resize(session, logical_size, completion, out_diagnostic)
        })
    }

    /// Runs the session's ready graphics work on your graphics thread.
    ///
    /// Use it with `RenderDriverKind::CallerGraphicsThread`, and call it from
    /// the same thread every time. Call it each time
    /// `RenderSessionAttachOptions::driver_work_wake` is called, even while you
    /// demand no frames, and until a detach or disposal completes. It returns
    /// once no work is ready.
    ///
    /// It accepts a disposed session until the session retires. The service
    /// after the disposal detaches the session and frees its graphics objects.
    ///
    /// See `mln_render_session_service_driver_work` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn service_driver_work(&self) -> Result<()> {
        let mut call = self.inner.call();
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_service_driver_work(session, out_diagnostic)
        })?;
        Ok(())
    }

    /// Replaces the Metal textures that a borrowed ring renders into.
    ///
    /// Use it to resize the ring, or to give the session textures again after
    /// `RenderSessionHandle::release_target()`. Release every acquired frame
    /// first. Once the replacement completes, the session no longer uses the
    /// old textures. A replacement with the textures and sizes that the ring
    /// renders into already changes nothing.
    ///
    /// See `mln_render_session_set_metal_borrowed_texture_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when the new textures have a different pixel
    ///   format than the old ones. Attach a new session to change it.
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "ios",
        target_os = "macos",
        target_os = "tvos",
        target_os = "visionos"
    ))]
    pub unsafe fn set_metal_borrowed_texture_target(
        &self,
        replacement: &MetalBorrowedTextureReplacement,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let replacement = call.reference(&replacement)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_metal_borrowed_texture_target(
                    session,
                    replacement,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Replaces the Metal layer that the session renders to.
    ///
    /// Use it to give the session a layer again after
    /// `RenderSessionHandle::release_target()`, to move it to another layer, or
    /// to resize it. Once the replacement completes, the session no longer uses
    /// the old layer. A replacement with the layer and logical size that the
    /// session renders to already changes nothing.
    ///
    /// See `mln_render_session_set_metal_surface_target` in the
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
    pub unsafe fn set_metal_surface_target(
        &self,
        replacement: &MetalSurfaceReplacement,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let replacement = call.reference(&replacement)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_metal_surface_target(
                    session,
                    replacement,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Replaces the OpenGL textures that a borrowed ring renders into.
    ///
    /// Use it to resize the ring, or to give the session textures again after
    /// `RenderSessionHandle::release_target()`. Release every acquired frame
    /// first. Once the replacement completes, the session no longer uses the
    /// old textures. A replacement with the textures and sizes that the ring
    /// renders into already changes nothing.
    ///
    /// See `mln_render_session_set_open_gl_borrowed_texture_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn set_open_gl_borrowed_texture_target(
        &self,
        replacement: &OpenGlBorrowedTextureReplacement,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let replacement = call.reference(&replacement)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_open_gl_borrowed_texture_target(
                    session,
                    replacement,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Replaces the OpenGL surface that the session presents to.
    ///
    /// Use it to give the session a surface again after
    /// `RenderSessionHandle::release_target()`, to move it to another surface,
    /// or to resize it. Once the replacement completes, the session no longer
    /// uses the old surface. A replacement with the surface and logical size
    /// that the session presents to already changes nothing.
    ///
    /// See `mln_render_session_set_open_gl_surface_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn set_open_gl_surface_target(
        &self,
        replacement: &OpenGlSurfaceReplacement,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let replacement = call.reference(&replacement)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_open_gl_surface_target(
                    session,
                    replacement,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Replaces the Vulkan images that a borrowed ring renders into.
    ///
    /// Use it to resize the ring, or to give the session images again after
    /// `RenderSessionHandle::release_target()`. Release every acquired frame
    /// first. Once the replacement completes, the session no longer uses the
    /// old images. A replacement with the images and sizes that the ring
    /// renders into already changes nothing.
    ///
    /// See `mln_render_session_set_vulkan_borrowed_texture_target` in the
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
    pub unsafe fn set_vulkan_borrowed_texture_target(
        &self,
        replacement: &VulkanBorrowedTextureReplacement,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let replacement = call.reference(&replacement)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_vulkan_borrowed_texture_target(
                    session,
                    replacement,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Replaces the Vulkan surface that the session presents to.
    ///
    /// Use it to give the session a surface again after
    /// `RenderSessionHandle::release_target()`, to move it to another surface,
    /// or to resize it. Once the replacement completes, the session no longer
    /// uses the old surface. A replacement with the surface and logical size
    /// that the session presents to already changes nothing.
    ///
    /// See `mln_render_session_set_vulkan_surface_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when the new surface needs a different color
    ///   format or pre-rotation support than the old one. Attach a new session
    ///   instead.
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
    pub unsafe fn set_vulkan_surface_target(
        &self,
        replacement: &VulkanSurfaceReplacement,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let replacement = call.reference(&replacement)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_vulkan_surface_target(
                    session,
                    replacement,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Replaces the WebGPU textures that a borrowed ring renders into.
    ///
    /// Use it to resize the ring, or to give the session textures again after
    /// `RenderSessionHandle::release_target()`. Release every acquired frame
    /// first. Once the replacement completes, the session no longer uses the
    /// old textures. A replacement with the textures and sizes that the ring
    /// renders into already changes nothing.
    ///
    /// See `mln_render_session_set_web_gpu_borrowed_texture_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn set_web_gpu_borrowed_texture_target(
        &self,
        replacement: &WebGpuBorrowedTextureReplacement,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let replacement = call.reference(&replacement)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_web_gpu_borrowed_texture_target(
                    session,
                    replacement,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Replaces the WebGPU surface that the session presents to.
    ///
    /// Use it to give the session a surface again after
    /// `RenderSessionHandle::release_target()`, to move it to another surface,
    /// or to resize it. Once the replacement completes, the session no longer
    /// uses the old surface. A replacement with the surface and logical size
    /// that the session presents to already changes nothing.
    ///
    /// See `mln_render_session_set_web_gpu_surface_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn set_web_gpu_surface_target(
        &self,
        replacement: &WebGpuSurfaceReplacement,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let replacement = call.reference(&replacement)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_web_gpu_surface_target(
                    session,
                    replacement,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }
}

/// What a release of [`RenderSessionHandle`] leaves while native retires the
/// handle. Its functions stay callable until the handle retires, and
/// fail with `Status::InvalidState` afterwards. It owns nothing, so
/// dropping it changes nothing.
#[derive(Debug)]
pub struct RetiringRenderSession(sys::mln_render_session);
impl RetiringRenderSession {
    /// Returns the native handle value.
    pub fn id(&self) -> u64 {
        self.0.0
    }

    /// Runs the session's ready graphics work on your graphics thread.
    ///
    /// Use it with `RenderDriverKind::CallerGraphicsThread`, and call it from
    /// the same thread every time. Call it each time
    /// `RenderSessionAttachOptions::driver_work_wake` is called, even while you
    /// demand no frames, and until a detach or disposal completes. It returns
    /// once no work is ready.
    ///
    /// It accepts a disposed session until the session retires. The service
    /// after the disposal detaches the session and frees its graphics objects.
    ///
    /// See `mln_render_session_service_driver_work` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn service_driver_work(&self) -> Result<()> {
        let mut call = Call::new(self.0);
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_service_driver_work(session, out_diagnostic)
        })?;
        Ok(())
    }
}

/// Submits [`RenderSessionHandle`] commands without a completion. Each call returns
/// the same acceptance status as its completing form, and native logs a
/// command that later fails as a warning through the log handler.
pub struct RenderSessionHandleDiscarding<'a>(&'a RenderSessionHandle);
impl RenderSessionHandle {
    pub fn discarding(&self) -> RenderSessionHandleDiscarding<'_> {
        RenderSessionHandleDiscarding(self)
    }
}
impl RenderSessionHandleDiscarding<'_> {
    /// Resizes the session's target and its map.
    ///
    /// The session renders at the map's scale factor. Release every acquired
    /// frame first.
    ///
    /// See `mln_render_session_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when the session renders into a borrowed ring.
    ///   Give it new textures of the new size instead.
    pub fn resize(&self, logical_size: LogicalSize) -> Result<()> {
        let mut call = self.0.inner.call();
        let logical_size = call.input(&logical_size)?;
        call.discard(|session, completion, out_diagnostic| unsafe {
            sys::mln_render_session_resize(session, logical_size, completion, out_diagnostic)
        })
    }
}
