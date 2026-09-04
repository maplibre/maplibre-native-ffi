part of 'runtime.dart';

final class _TextureFrameLease {
  _TextureFrameLease(Object owner, this.release)
    : owner = WeakReference<Object>(owner);

  _TextureFrameLease.orphaned(this.release) : owner = null;

  final WeakReference<Object>? owner;
  final void Function() release;
}

/// Any-thread standalone projection helper snapshot from a map transform.
final class MapProjectionHandle {
  MapProjectionHandle._(NativeMapProjection handle)
    : _state = NativeHandleState(
        handle,
        'MapProjectionHandle',
        threadAffine: false,
      );

  final NativeHandleState<NativeMapProjection> _state;

  /// Whether this projection helper has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  NativeMapProjection get _handle => _state.handle;

  /// Copies the current projection camera options.
  CameraOptions camera() {
    return withNativeArena((arena) {
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      _check(raw.mln_map_projection_get_camera(_handle.raw, outCamera));
      return native_struct.cameraOptionsFromNative(outCamera.ref);
    });
  }

  /// Applies camera fields to the projection helper.
  void setCamera(CameraOptions camera) {
    withNativeArena((arena) {
      final nativeCamera = arena<raw.mln_camera_options>();
      nativeCamera.ref = native_struct.cameraOptionsToNative(
        camera,
        raw.mln_camera_options_default(),
      );
      _check(raw.mln_map_projection_set_camera(_handle.raw, nativeCamera));
    });
  }

  /// Updates the camera so coordinates are visible within [padding].
  void setVisibleCoordinates(
    List<LatLng> coordinates, {
    EdgeInsets padding = const EdgeInsets(),
  }) {
    withNativeArena((arena) {
      final nativeCoordinates = coordinates.isEmpty
          ? nullptr.cast<raw.mln_lat_lng>()
          : arena<raw.mln_lat_lng>(coordinates.length);
      for (var index = 0; index < coordinates.length; index += 1) {
        nativeCoordinates[index] = native_struct.latLngToNative(
          coordinates[index],
        );
      }
      _check(
        raw.mln_map_projection_set_visible_coordinates(
          _handle.raw,
          nativeCoordinates,
          coordinates.length,
          native_struct.edgeInsetsToNative(padding),
        ),
      );
    });
  }

  /// Updates the camera so geometry coordinates are visible within [padding].
  void setVisibleGeometry(
    Uint8List geometry, {
    EdgeInsets padding = const EdgeInsets(),
  }) {
    withNativeArena((arena) {
      final nativeGeometry = nativeBufferView(geometry, arena);
      _check(
        raw.mln_map_projection_set_visible_geometry(
          _handle.raw,
          nativeGeometry,
          native_struct.edgeInsetsToNative(padding),
        ),
      );
    });
  }

  /// Converts a geographic world coordinate to a screen point.
  ScreenPoint pixelForLatLng(LatLng coordinate) {
    return withNativeArena((arena) {
      final outPoint = arena<raw.mln_screen_point>();
      _check(
        raw.mln_map_projection_pixel_for_lat_lng(
          _handle.raw,
          native_struct.latLngToNative(coordinate),
          outPoint,
        ),
      );
      return native_struct.screenPointFromNative(outPoint.ref);
    });
  }

  /// Converts a screen point to a geographic coordinate with longitude wrapped
  /// to the range from -180 to 180 degrees.
  LatLng latLngForPixel(ScreenPoint point) {
    return withNativeArena((arena) {
      final outCoordinate = arena<raw.mln_lat_lng>();
      _check(
        raw.mln_map_projection_lat_lng_for_pixel(
          _handle.raw,
          native_struct.screenPointToNative(point),
          outCoordinate,
        ),
      );
      return native_struct.latLngFromNative(outCoordinate.ref);
    });
  }

  /// Converts a screen point to an unwrapped geographic coordinate.
  ///
  /// The longitude preserves the visible world copy and may fall outside
  /// -180 to 180.
  LatLng latLngForPixelUnwrapped(ScreenPoint point) {
    return withNativeArena((arena) {
      final outCoordinate = arena<raw.mln_lat_lng>();
      _check(
        raw.mln_map_projection_lat_lng_for_pixel_unwrapped(
          _handle.raw,
          native_struct.screenPointToNative(point),
          outCoordinate,
        ),
      );
      return native_struct.latLngFromNative(outCoordinate.ref);
    });
  }

  /// Explicitly destroys this projection helper.
  void close() {
    _state.close(
      (handle) => raw.mln_map_projection_destroy(handle.raw),
      _c.threadLastErrorMessage,
    );
  }
}

/// Outcome reported by [RenderSessionHandle.renderUpdate].
final class RenderResult {
  const RenderResult._(this.rawValue, this.name);

  /// The render target holds a new frame.
  static const rendered = RenderResult._(0, 'rendered');

  /// The call produced no frame.
  static const noUpdate = RenderResult._(1, 'noUpdate');

  /// The map has not applied the session's current size yet.
  static const sizePending = RenderResult._(2, 'sizePending');

  /// The render target had no frame to draw into.
  static const targetNotReady = RenderResult._(3, 'targetNotReady');

  /// Creates a render result while preserving unknown native values.
  factory RenderResult.fromRawValue(int rawValue) => switch (rawValue) {
    0 => rendered,
    1 => noUpdate,
    2 => sizePending,
    3 => targetNotReady,
    _ => RenderResult._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is RenderResult && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Outcome of [RenderSessionHandle.renderUpdate]: the render result plus the
/// repaint request the map raised while rendering.
final class RenderUpdate {
  const RenderUpdate._({required this.result, required this.needsRepaint});

  /// What the call produced; names the wake to wait for before calling again.
  final RenderResult result;

  /// Whether the map asked for another frame while it rendered this one, as
  /// during an ongoing camera transition.
  ///
  /// This is the same signal that
  /// [RuntimeEventType.mapRenderFrameFinished] carries in its
  /// [RuntimeEventRenderFrame.needsRepaint] field, delivered here without the
  /// event round trip, so a host can re-arm its frame loop before it drains
  /// events. It is true only when [result] is [RenderResult.rendered] and
  /// reads false for every other outcome.
  final bool needsRepaint;
}

/// Owner-thread render session handle attached to a retained map.
final class RenderSessionHandle {
  RenderSessionHandle._(NativeRenderSession handle)
    : _state = NativeHandleState(handle, 'RenderSessionHandle');

  final NativeHandleState<NativeRenderSession> _state;
  _TextureFrameLease? _activeTextureFrame;

  /// Whether this render session has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  /// The session belongs to the isolate that attached it, which need not be the
  /// map's. It holds no Dart reference to the map; native reports an
  /// invalid-state status for destroying a map that still has a session.
  NativeRenderSession get _handle => _state.handle;

  /// Resizes an attached render session.
  ///
  /// Surface and session-owned texture targets resize in place. A caller-owned
  /// texture is rejected here: allocate one at the new size and hand it over
  /// with [setMetalBorrowedTextureTarget] or its Vulkan or OpenGL counterpart.
  ///
  /// The session keeps its renderer across a resize, along with the tile
  /// pyramid, glyph and image atlases, and symbol placement. A scale factor
  /// that differs from the session's current one retires the renderer instead,
  /// because a renderer compiles its shaders for one pixel ratio; the same
  /// applies to every `setTarget` method. Map-owned feature state survives
  /// either way.
  void resize(int width, int height, {double scaleFactor = 1}) {
    _checkNoActiveTextureFrame('resize render session');
    _check(
      raw.mln_render_session_resize(_handle.raw, width, height, scaleFactor),
    );
  }

  /// Presents this attached surface session through a new Metal surface.
  ///
  /// Replacing the surface in place keeps this session's renderer along with
  /// the tile pyramid, atlases, and symbol placement. Map-owned feature state
  /// is unchanged.
  ///
  /// [descriptor] names the same graphics context this session attached with,
  /// and its extent applies as [resize] applies one. A descriptor whose context
  /// device is neither null nor this session's device throws an
  /// [InvalidArgumentException] and leaves this session rendering into the
  /// surface it has. The session assigns the layer its own device and pixel
  /// format.
  void setMetalSurfaceTarget(MetalSurfaceDescriptor descriptor) {
    _checkNoActiveTextureFrame('set Metal surface target');
    withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_metal_surface_descriptor>();
      nativeDescriptor.ref = _metalSurfaceDescriptorToNative(descriptor);
      _check(raw.mln_metal_surface_set_target(_handle.raw, nativeDescriptor));
    });
  }

  /// Presents this attached surface session through a new Vulkan surface.
  ///
  /// See [setMetalSurfaceTarget] for what replacing a surface preserves. The
  /// outgoing `VkSurfaceKHR` must still be valid: this session holds a
  /// swapchain built from it, and Vulkan destroys every swapchain before its
  /// surface.
  void setVulkanSurfaceTarget(VulkanSurfaceDescriptor descriptor) {
    _checkNoActiveTextureFrame('set Vulkan surface target');
    withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_vulkan_surface_descriptor>();
      nativeDescriptor.ref = _vulkanSurfaceDescriptorToNative(descriptor);
      _check(raw.mln_vulkan_surface_set_target(_handle.raw, nativeDescriptor));
    });
  }

  /// Presents this attached surface session through a new OpenGL surface.
  ///
  /// See [setMetalSurfaceTarget] for what replacing a surface preserves. The
  /// new surface is made current on the next render, so a host may hand over a
  /// replacement for one it has already destroyed, and an unusable surface is
  /// reported by the next [renderUpdate] rather than by this call.
  void setOpenGLSurfaceTarget(OpenGLSurfaceDescriptor descriptor) {
    _checkNoActiveTextureFrame('set OpenGL surface target');
    withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_opengl_surface_descriptor>();
      nativeDescriptor.ref = _openglSurfaceDescriptorToNative(descriptor);
      _check(raw.mln_opengl_surface_set_target(_handle.raw, nativeDescriptor));
    });
  }

  /// Renders this attached texture session into a new caller-owned Metal
  /// texture.
  ///
  /// Handing a reallocated texture over here keeps this session's renderer,
  /// where [resize] rejects a caller-owned texture outright.
  ///
  /// The replacement belongs to the device this session attached with, which
  /// throws an [InvalidArgumentException] otherwise, and carries the pixel
  /// format it attached with, which throws an [UnsupportedFeatureException]
  /// otherwise; both leave this session rendering into the texture it has. The
  /// caller owns the replacement and keeps it valid until the next replacement,
  /// [detach], or [close]. The outgoing texture is neither read nor released
  /// here, so a host that already released it may still hand over a
  /// replacement.
  void setMetalBorrowedTextureTarget(
    MetalBorrowedTextureDescriptor descriptor,
  ) {
    _checkNoActiveTextureFrame('set Metal borrowed texture target');
    withNativeArena((arena) {
      final nativeDescriptor =
          arena<raw.mln_metal_borrowed_texture_descriptor>();
      nativeDescriptor.ref = _metalBorrowedTextureDescriptorToNative(
        descriptor,
      );
      _check(
        raw.mln_metal_borrowed_texture_set_target(
          _handle.raw,
          nativeDescriptor,
        ),
      );
    });
  }

  /// Renders this attached texture session into a new caller-owned Vulkan
  /// image.
  ///
  /// See [setMetalBorrowedTextureTarget] for what replacing a target preserves.
  /// The replacement carries the format and both layouts this session attached
  /// with, since its render pass was built around them.
  void setVulkanBorrowedTextureTarget(
    VulkanBorrowedTextureDescriptor descriptor,
  ) {
    _checkNoActiveTextureFrame('set Vulkan borrowed texture target');
    withNativeArena((arena) {
      final nativeDescriptor =
          arena<raw.mln_vulkan_borrowed_texture_descriptor>();
      nativeDescriptor.ref = _vulkanBorrowedTextureDescriptorToNative(
        descriptor,
      );
      _check(
        raw.mln_vulkan_borrowed_texture_set_target(
          _handle.raw,
          nativeDescriptor,
        ),
      );
    });
  }

  /// Renders this attached texture session into a new caller-owned OpenGL
  /// texture.
  ///
  /// See [setMetalBorrowedTextureTarget] for what replacing a target preserves.
  /// The replacement belongs to the context this session attached with, or one
  /// in its share group, and that context must be current on this isolate's
  /// thread.
  void setOpenGLBorrowedTextureTarget(
    OpenGLBorrowedTextureDescriptor descriptor,
  ) {
    _checkNoActiveTextureFrame('set OpenGL borrowed texture target');
    withNativeArena((arena) {
      final nativeDescriptor =
          arena<raw.mln_opengl_borrowed_texture_descriptor>();
      nativeDescriptor.ref = _openglBorrowedTextureDescriptorToNative(
        descriptor,
      );
      _check(
        raw.mln_opengl_borrowed_texture_set_target(
          _handle.raw,
          nativeDescriptor,
        ),
      );
    });
  }

  /// Renders the latest map update into this session's render target.
  ///
  /// [RenderUpdate.result] names the wake to wait for before calling again:
  ///
  /// - [RenderResult.rendered]: the target holds a new frame. The map retains
  ///   its latest update, so a host redraws on demand after a resize or a
  ///   surface expose, and gates a frame loop on
  ///   [RuntimeEventType.mapRenderUpdateAvailable].
  /// - [RenderResult.noUpdate]: the call produced no frame. The map either has
  ///   no update yet, or the Metal backend has not created an owned texture
  ///   because content is not ready. Wait for
  ///   [RuntimeEventType.mapRenderUpdateAvailable].
  /// - [RenderResult.sizePending]: this session resized and the map, which
  ///   applies its size on its own thread, is still behind. The map publishes
  ///   an update for the new size on its own, so wait for the next
  ///   [RuntimeEventType.mapRenderUpdateAvailable].
  /// - [RenderResult.targetNotReady]: the render target had no frame
  ///   available, such as a Metal surface whose next drawable is nil. Wait for
  ///   a host event that changes the target, or back off and retry.
  ///
  /// [RenderUpdate.needsRepaint] reports whether the map asked for another
  /// frame while it rendered this one, so a host can re-arm its frame loop
  /// before it drains events.
  RenderUpdate renderUpdate() {
    _checkNoActiveTextureFrame('render update');
    return withNativeArena((arena) {
      final result = arena<Uint32>();
      final needsRepaint = arena<Bool>();
      _check(
        raw.mln_render_session_render_update(_handle.raw, result, needsRepaint),
      );
      return RenderUpdate._(
        result: RenderResult.fromRawValue(result.value),
        needsRepaint: needsRepaint.value,
      );
    });
  }

  /// Detaches backend-bound render resources while keeping the handle live.
  void detach() {
    _checkNoActiveTextureFrame('detach render session');
    _check(raw.mln_render_session_detach(_handle.raw));
  }

  /// Asks the session renderer to release cached resources where possible.
  void reduceMemoryUse() {
    _check(raw.mln_render_session_reduce_memory_use(_handle.raw));
  }

  /// Clears renderer data for the session.
  void clearData() {
    _check(raw.mln_render_session_clear_data(_handle.raw));
  }

  /// Dumps renderer debug logs through MapLibre Native logging.
  void dumpDebugLogs() {
    _check(raw.mln_render_session_dump_debug_logs(_handle.raw));
  }

  /// Queries rendered features from the latest render session state.
  List<QueriedFeature> queryRenderedFeatures(
    RenderedQueryGeometry geometry, {
    RenderedFeatureQueryOptions? options,
  }) {
    final resolvedOptions = options ?? RenderedFeatureQueryOptions();
    return withNativeArena((arena) {
      final nativeGeometry = arena<raw.mln_rendered_query_geometry>();
      nativeGeometry.ref = _renderedQueryGeometryToNative(geometry, arena);
      final nativeOptions = _renderedFeatureQueryOptionsToNative(
        resolvedOptions,
        arena,
      );
      final outResult = arena<Uint64>();
      outResult.value = 0;
      _check(
        raw.mln_render_session_query_rendered_features(
          _handle.raw,
          nativeGeometry,
          nativeOptions,
          outResult,
        ),
      );
      return _copyQueriedFeatureList(NativeQueriedFeatureList(outResult.value));
    });
  }

  /// Queries source features from the latest render session state.
  List<QueriedFeature> querySourceFeatures(
    String sourceId, {
    SourceFeatureQueryOptions? options,
  }) {
    final resolvedOptions = options ?? SourceFeatureQueryOptions();
    return withNativeArena((arena) {
      final nativeSourceId = nativeStringView(sourceId, arena);
      final nativeOptions = _sourceFeatureQueryOptionsToNative(
        resolvedOptions,
        arena,
      );
      final outResult = arena<Uint64>();
      outResult.value = 0;
      _check(
        raw.mln_render_session_query_source_features(
          _handle.raw,
          nativeSourceId.value,
          nativeOptions,
          outResult,
        ),
      );
      return _copyQueriedFeatureList(NativeQueriedFeatureList(outResult.value));
    });
  }

  /// Queries a feature extension from the latest render session state.
  Uint8List queryFeatureExtensions({
    required String sourceId,
    required Uint8List feature,
    required String extension,
    required String extensionField,
    Uint8List? arguments,
  }) {
    return withNativeArena((arena) {
      final nativeSourceId = nativeStringView(sourceId, arena);
      final nativeFeature = nativeBufferView(feature, arena);
      final nativeExtension = nativeStringView(extension, arena);
      final nativeExtensionField = nativeStringView(extensionField, arena);
      final nativeArguments = arguments == null
          ? nullptr.cast<raw.mln_buffer_view>()
          : (arena<raw.mln_buffer_view>()
              ..ref = nativeBufferView(arguments, arena));
      final outResult = arena<Uint64>();
      outResult.value = 0;
      _check(
        raw.mln_render_session_query_feature_extensions(
          _handle.raw,
          nativeSourceId.value,
          nativeFeature,
          nativeExtension.value,
          nativeExtensionField.value,
          nativeArguments,
          outResult,
        ),
      );
      return copyOwnedBuffer(NativeOwnedBufferHandle(outResult.value));
    });
  }

  /// Reads the latest rendered session-owned texture as premultiplied RGBA8.
  TextureImageInfo textureImageInfo() {
    _checkNoActiveTextureFrame('read texture image info');
    return withNativeArena((arena) {
      final info = arena<raw.mln_texture_image_info>();
      info.ref = raw.mln_texture_image_info_default();
      // A null buffer with zero capacity is a size probe that reports the
      // required byte length.
      _check(
        raw.mln_texture_read_premultiplied_rgba8(
          _handle.raw,
          nullptr.cast<Uint8>(),
          0,
          info,
        ),
      );
      return TextureImageInfo._fromNative(info.ref);
    });
  }

  /// Reads the latest texture into caller-owned native [buffer].
  TextureImageInfo readPremultipliedRgba8Into(NativeBuffer buffer) {
    _checkNoActiveTextureFrame('read texture image');
    final expectedInfo = textureImageInfo();
    if (buffer.byteLength < expectedInfo.byteLength) {
      throwInvalidArgument(
        'native buffer length ${buffer.byteLength} is smaller than required '
        '${expectedInfo.byteLength}',
      );
    }
    return withNativeArena((arena) {
      final info = arena<raw.mln_texture_image_info>();
      info.ref = raw.mln_texture_image_info_default();
      _check(
        raw.mln_texture_read_premultiplied_rgba8(
          _handle.raw,
          Pointer<Uint8>.fromAddress(buffer.unsafePointer.address),
          buffer.byteLength,
          info,
        ),
      );
      return TextureImageInfo._fromNative(info.ref);
    });
  }

  /// Reads the latest rendered session-owned texture as premultiplied RGBA8.
  TextureImage readPremultipliedRgba8() {
    _checkNoActiveTextureFrame('read texture image');
    final info = textureImageInfo();
    return withNativeArena((arena) {
      final data = arena<Uint8>(info.byteLength);
      final readInfo = arena<raw.mln_texture_image_info>();
      readInfo.ref = raw.mln_texture_image_info_default();
      _check(
        raw.mln_texture_read_premultiplied_rgba8(
          _handle.raw,
          data,
          info.byteLength,
          readInfo,
        ),
      );
      final copiedInfo = TextureImageInfo._fromNative(readInfo.ref);
      return TextureImage(
        info: copiedInfo,
        bytes: Uint8List.fromList(data.asTypedList(copiedInfo.byteLength)),
      );
    });
  }

  /// Acquires the latest Metal texture frame until [MetalOwnedTextureFrame.close].
  MetalOwnedTextureFrame acquireMetalTextureFrame() {
    _checkNoActiveTextureFrame('acquire Metal texture frame');
    final outFrame = calloc<raw.mln_metal_owned_texture_frame>();
    try {
      outFrame.ref.size = sizeOf<raw.mln_metal_owned_texture_frame>();
      _check(raw.mln_metal_owned_texture_acquire_frame(_handle.raw, outFrame));
    } catch (_) {
      calloc.free(outFrame);
      rethrow;
    }
    return _constructAcquiredTextureFrame(
      outFrame,
      () => raw.mln_metal_owned_texture_release_frame(_handle.raw, outFrame),
      () => MetalOwnedTextureFrame._(this, outFrame),
    );
  }

  /// Acquires the latest Vulkan texture frame until [VulkanOwnedTextureFrame.close].
  VulkanOwnedTextureFrame acquireVulkanTextureFrame() {
    _checkNoActiveTextureFrame('acquire Vulkan texture frame');
    final outFrame = calloc<raw.mln_vulkan_owned_texture_frame>();
    try {
      outFrame.ref.size = sizeOf<raw.mln_vulkan_owned_texture_frame>();
      _check(raw.mln_vulkan_owned_texture_acquire_frame(_handle.raw, outFrame));
    } catch (_) {
      calloc.free(outFrame);
      rethrow;
    }
    return _constructAcquiredTextureFrame(
      outFrame,
      () => raw.mln_vulkan_owned_texture_release_frame(_handle.raw, outFrame),
      () => VulkanOwnedTextureFrame._(this, outFrame),
    );
  }

  /// Acquires the latest OpenGL texture frame until [OpenGLOwnedTextureFrame.close].
  OpenGLOwnedTextureFrame acquireOpenGLTextureFrame() {
    _checkNoActiveTextureFrame('acquire OpenGL texture frame');
    final outFrame = calloc<raw.mln_opengl_owned_texture_frame>();
    try {
      outFrame.ref.size = sizeOf<raw.mln_opengl_owned_texture_frame>();
      _check(raw.mln_opengl_owned_texture_acquire_frame(_handle.raw, outFrame));
    } catch (_) {
      calloc.free(outFrame);
      rethrow;
    }
    return _constructAcquiredTextureFrame(
      outFrame,
      () => raw.mln_opengl_owned_texture_release_frame(_handle.raw, outFrame),
      () => OpenGLOwnedTextureFrame._(this, outFrame),
    );
  }

  /// Explicitly destroys this render session.
  void close() {
    _checkNoActiveTextureFrame('close render session');
    _state.close(
      (handle) => raw.mln_render_session_destroy(handle.raw),
      _c.threadLastErrorMessage,
    );
  }

  void _registerTextureFrame(Object frame, void Function() release) {
    _checkNoActiveTextureFrame('acquire texture frame');
    _activeTextureFrame = _TextureFrameLease(frame, release);
  }

  T _constructAcquiredTextureFrame<T, F extends NativeType>(
    Pointer<F> descriptor,
    int Function() release,
    T Function() construct,
  ) {
    try {
      return construct();
    } catch (error, stackTrace) {
      cleanupFailedFrameConstruction(
        release: release,
        releaseSucceeded: () => calloc.free(descriptor),
        releaseFailed: () {
          _activeTextureFrame = _TextureFrameLease.orphaned(() {
            _check(release());
            calloc.free(descriptor);
          });
        },
      );
      Error.throwWithStackTrace(error, stackTrace);
    }
  }

  void _releaseTextureFrame(Object frame) {
    final lease = _activeTextureFrame;
    if (lease == null || !identical(lease.owner?.target, frame)) {
      return;
    }
    lease.release();
    _activeTextureFrame = null;
  }

  void _checkNoActiveTextureFrame(String operation) {
    final lease = _activeTextureFrame;
    if (lease == null) {
      return;
    }
    if (lease.owner?.target != null) {
      throwInvalidState(
        '$operation requires releasing the active texture frame',
      );
    }
    lease.release();
    _activeTextureFrame = null;
  }
}

/// Releasable handle for a resource request owned by a Dart provider.
///
/// This carries only the request's handle id, so it may be sent to another
/// isolate and completed there. Completion and release are process-wide
/// one-shot operations even when copies of this handle race across isolates.
extension type const ResourceRequestHandle._(NativeResourceRequest _handle) {
  /// The handle, with the ABI validated first: this type is made to be used
  /// from an isolate where the memoized check has not run yet.
  NativeResourceRequest get _checked {
    ensureAbiVersion();
    return _handle;
  }

  /// Reports whether MapLibre has cancelled this provider request.
  bool get isCancelled => cancelled();

  /// Reports whether MapLibre has cancelled this provider request.
  bool cancelled() {
    return withNativeArena((arena) {
      final outCancelled = arena<Bool>();
      _check(raw.mln_resource_request_cancelled(_checked.raw, outCancelled));
      return outCancelled.value;
    });
  }

  /// Completes this request with [response] and releases it. Completion is
  /// one-shot: the C API rejects a second attempt from any isolate.
  void complete(ResourceResponse response) {
    _checkResourceResponseNativeStrings(response);
    withNativeArena((arena) {
      final nativeResponse = arena<raw.mln_resource_response>();
      nativeResponse.ref = _resourceResponseToNative(response, arena);
      try {
        _check(raw.mln_resource_request_complete(_checked.raw, nativeResponse));
      } finally {
        raw.mln_resource_request_release(_checked.raw);
        _retireResourceRequestCancelState(_handle.raw);
      }
    });
  }

  /// Registers [callback] to run once when MapLibre cancels this request, or
  /// clears the registration when [callback] is null.
  ///
  /// Each call replaces the previous registration. The callback runs on the
  /// isolate that registered it, after MapLibre discards a request that this
  /// provider has left open. A request that is already cancelled schedules the
  /// callback immediately. A request the provider completed is never reported.
  ///
  /// The callback may use this handle, including [complete], which reports
  /// [InvalidStateException] for a cancelled request, and [close]. An exception
  /// the callback throws is contained rather than delivered to the isolate.
  ///
  /// Register, replace, clear, and retire one request on the same isolate.
  void setCancelCallback(ResourceRequestCancelCallback? callback) {
    final requestId = _checked.raw;
    final previous = _resourceRequestCancelStates[requestId];
    if (callback == null) {
      try {
        _check(
          raw.mln_resource_request_set_cancel_callback(
            requestId,
            nullptr,
            nullptr,
          ),
        );
      } finally {
        _resourceRequestCancelStates.remove(requestId);
        previous?.close();
      }
      return;
    }
    final state = _ResourceRequestCancelState(requestId, callback);
    try {
      _check(
        raw.mln_resource_request_set_cancel_callback(
          requestId,
          state.listener.nativeFunction,
          nullptr,
        ),
      );
    } catch (_) {
      state.close();
      rethrow;
    }
    _resourceRequestCancelStates[requestId] = state;
    previous?.close();
  }

  /// Releases the provider reference without completing it.
  ///
  /// Releasing an already-retired request is a no-op in the C API, so this is
  /// safe to call from any isolate holding a copy.
  void close() {
    raw.mln_resource_request_release(_checked.raw);
    _retireResourceRequestCancelState(_handle.raw);
  }

  /// Blocks until this request is completed or released, wherever that happens.
  void waitUntilRetired() {
    _check(raw.mln_resource_request_wait_until_retired(_checked.raw));
  }
}

/// Exposes an attach reference's map id for tests that must reach the C API
/// with a raw id.
int mapAttachRefIdForTesting(MapAttachRef ref) => ref._mapId;

/// Exposes a runtime's handle id for tests that must reach the C API with a raw
/// id.
int runtimeHandleIdForTesting(RuntimeHandle runtime) => runtime._state.handleId;

/// CPU image readback metadata for a texture session frame.
final class TextureImageInfo {
  const TextureImageInfo._({
    required this.width,
    required this.height,
    required this.stride,
    required this.byteLength,
  });

  factory TextureImageInfo._fromNative(raw.mln_texture_image_info value) =>
      TextureImageInfo._(
        width: value.width,
        height: value.height,
        stride: value.stride,
        byteLength: value.byte_length,
      );

  /// Physical image width in device pixels.
  final int width;

  /// Physical image height in device pixels.
  final int height;

  /// Bytes per image row.
  final int stride;

  /// Required output buffer byte length.
  final int byteLength;
}

/// Dart-owned premultiplied RGBA8 texture readback bytes.
final class TextureImage {
  TextureImage({required this.info, required Uint8List bytes})
    : bytes = Uint8List.fromList(bytes).asUnmodifiableView();

  /// Image metadata.
  final TextureImageInfo info;

  /// Copied premultiplied RGBA8 bytes.
  final Uint8List bytes;
}

/// Scoped Metal texture frame borrowed from a session-owned texture target.
final class MetalOwnedTextureFrame implements Finalizable {
  MetalOwnedTextureFrame._(this._session, this._frame) {
    final frame = _frame.ref;
    _generation = uint64FromNative(frame.generation);
    _width = frame.width;
    _height = frame.height;
    _scaleFactor = frame.scale_factor;
    _frameId = uint64FromNative(frame.frame_id);
    _pixelFormat = uint64FromNative(frame.pixel_format);
    _textureAddress = frame.texture.address;
    _deviceAddress = frame.device.address;
    _leakReporter = NativeLeakReporter(
      this,
      'MetalOwnedTextureFrame',
      NativeHandle(_frame.address),
    );
    try {
      final session = _session;
      final descriptor = _frame;
      _session._registerTextureFrame(this, () {
        _check(
          raw.mln_metal_owned_texture_release_frame(
            session._handle.raw,
            descriptor,
          ),
        );
        calloc.free(descriptor);
      });
    } catch (_) {
      _leakReporter.close();
      rethrow;
    }
  }

  final RenderSessionHandle _session;
  final Pointer<raw.mln_metal_owned_texture_frame> _frame;
  late final BigInt _generation;
  late final int _width;
  late final int _height;
  late final double _scaleFactor;
  late final BigInt _frameId;
  late final BigInt _pixelFormat;
  late final int _textureAddress;
  late final int _deviceAddress;
  late final NativeLeakReporter _leakReporter;
  var _closed = false;

  /// Session-owned target generation that produced this frame.
  BigInt get generation {
    _checkOpen();
    return _generation;
  }

  /// Physical texture width in device pixels.
  int get width {
    _checkOpen();
    return _width;
  }

  /// Physical texture height in device pixels.
  int get height {
    _checkOpen();
    return _height;
  }

  /// UI-to-device pixel scale used for this frame.
  double get scaleFactor {
    _checkOpen();
    return _scaleFactor;
  }

  /// Backend frame identity within [generation].
  BigInt get frameId {
    _checkOpen();
    return _frameId;
  }

  /// Backend-native Metal pixel format value.
  BigInt get pixelFormat {
    _checkOpen();
    return _pixelFormat;
  }

  /// Unsafe borrowed `id<MTLTexture>` / `MTL::Texture*` pointer.
  ///
  /// The pointer is valid only until [close] releases this frame.
  ScopedNativePointer get unsafeTexture =>
      _borrowedPointer(_textureAddress, 'Metal texture');

  /// Unsafe borrowed `id<MTLDevice>` / `MTL::Device*` pointer.
  ///
  /// The pointer is valid only until [close] releases this frame.
  ScopedNativePointer get unsafeDevice =>
      _borrowedPointer(_deviceAddress, 'Metal device');

  /// Releases this frame. The unsafe backend pointers become invalid.
  void close() {
    if (_closed) {
      return;
    }
    _session._releaseTextureFrame(this);
    _leakReporter.close();
    _closed = true;
  }

  ScopedNativePointer _borrowedPointer(int address, String name) {
    _checkOpen();
    return ScopedNativePointer(
      address,
      checkValid: _checkOpen,
      debugName: name,
    );
  }

  void _checkOpen() {
    if (_closed) {
      throwInvalidArgument('Metal texture frame has already been released');
    }
    final _ = _session._handle.raw;
  }
}

/// Scoped Vulkan texture frame borrowed from a session-owned texture target.
final class VulkanOwnedTextureFrame implements Finalizable {
  VulkanOwnedTextureFrame._(this._session, this._frame) {
    final frame = _frame.ref;
    _generation = uint64FromNative(frame.generation);
    _width = frame.width;
    _height = frame.height;
    _scaleFactor = frame.scale_factor;
    _frameId = uint64FromNative(frame.frame_id);
    _format = frame.format;
    _layout = frame.layout;
    _imageBits = uint64FromNative(frame.image);
    _imageViewBits = uint64FromNative(frame.image_view);
    _deviceAddress = frame.device.address;
    _leakReporter = NativeLeakReporter(
      this,
      'VulkanOwnedTextureFrame',
      NativeHandle(_frame.address),
    );
    try {
      final session = _session;
      final descriptor = _frame;
      _session._registerTextureFrame(this, () {
        _check(
          raw.mln_vulkan_owned_texture_release_frame(
            session._handle.raw,
            descriptor,
          ),
        );
        calloc.free(descriptor);
      });
    } catch (_) {
      _leakReporter.close();
      rethrow;
    }
  }

  final RenderSessionHandle _session;
  final Pointer<raw.mln_vulkan_owned_texture_frame> _frame;
  late final BigInt _generation;
  late final int _width;
  late final int _height;
  late final double _scaleFactor;
  late final BigInt _frameId;
  late final int _format;
  late final int _layout;
  late final BigInt _imageBits;
  late final BigInt _imageViewBits;
  late final int _deviceAddress;
  late final NativeLeakReporter _leakReporter;
  var _closed = false;

  /// Session-owned target generation that produced this frame.
  BigInt get generation {
    _checkOpen();
    return _generation;
  }

  /// Physical image width in device pixels.
  int get width {
    _checkOpen();
    return _width;
  }

  /// Physical image height in device pixels.
  int get height {
    _checkOpen();
    return _height;
  }

  /// UI-to-device pixel scale used for this frame.
  double get scaleFactor {
    _checkOpen();
    return _scaleFactor;
  }

  /// Backend frame identity within [generation].
  BigInt get frameId {
    _checkOpen();
    return _frameId;
  }

  /// Backend-native Vulkan format value.
  int get format {
    _checkOpen();
    return _format;
  }

  /// Backend-native Vulkan image layout value.
  int get layout {
    _checkOpen();
    return _layout;
  }

  /// Unsafe borrowed VkImage handle.
  ///
  /// The handle is valid only until [close] releases this frame.
  ScopedVulkanHandle get unsafeImage => ScopedVulkanHandle(
    _imageBits,
    checkValid: _checkOpen,
    debugName: 'Vulkan image',
  );

  /// Unsafe borrowed VkImageView handle.
  ///
  /// The handle is valid only until [close] releases this frame.
  ScopedVulkanHandle get unsafeImageView => ScopedVulkanHandle(
    _imageViewBits,
    checkValid: _checkOpen,
    debugName: 'Vulkan image view',
  );

  /// Unsafe borrowed VkDevice pointer.
  ///
  /// The pointer is valid only until [close] releases this frame.
  ScopedNativePointer get unsafeDevice =>
      _borrowedPointer(_deviceAddress, 'Vulkan device');

  /// Releases this frame. The unsafe backend handles become invalid.
  void close() {
    if (_closed) {
      return;
    }
    _session._releaseTextureFrame(this);
    _leakReporter.close();
    _closed = true;
  }

  ScopedNativePointer _borrowedPointer(int address, String name) {
    _checkOpen();
    return ScopedNativePointer(
      address,
      checkValid: _checkOpen,
      debugName: name,
    );
  }

  void _checkOpen() {
    if (_closed) {
      throwInvalidArgument('Vulkan texture frame has already been released');
    }
    final _ = _session._handle.raw;
  }
}

/// Scoped OpenGL texture frame borrowed from a session-owned texture target.
final class OpenGLOwnedTextureFrame implements Finalizable {
  OpenGLOwnedTextureFrame._(this._session, this._frame) {
    final frame = _frame.ref;
    _generation = uint64FromNative(frame.generation);
    _width = frame.width;
    _height = frame.height;
    _scaleFactor = frame.scale_factor;
    _frameId = uint64FromNative(frame.frame_id);
    _texture = frame.texture;
    _target = frame.target;
    _internalFormat = frame.internal_format;
    _format = frame.format;
    _type = frame.type;
    _leakReporter = NativeLeakReporter(
      this,
      'OpenGLOwnedTextureFrame',
      NativeHandle(_frame.address),
    );
    try {
      final session = _session;
      final descriptor = _frame;
      _session._registerTextureFrame(this, () {
        _check(
          raw.mln_opengl_owned_texture_release_frame(
            session._handle.raw,
            descriptor,
          ),
        );
        calloc.free(descriptor);
      });
    } catch (_) {
      _leakReporter.close();
      rethrow;
    }
  }

  final RenderSessionHandle _session;
  final Pointer<raw.mln_opengl_owned_texture_frame> _frame;
  late final BigInt _generation;
  late final int _width;
  late final int _height;
  late final double _scaleFactor;
  late final BigInt _frameId;
  late final int _texture;
  late final int _target;
  late final int _internalFormat;
  late final int _format;
  late final int _type;
  late final NativeLeakReporter _leakReporter;
  var _closed = false;

  /// Session-owned target generation that produced this frame.
  BigInt get generation {
    _checkOpen();
    return _generation;
  }

  /// Physical texture width in device pixels.
  int get width {
    _checkOpen();
    return _width;
  }

  /// Physical texture height in device pixels.
  int get height {
    _checkOpen();
    return _height;
  }

  /// UI-to-device pixel scale used for this frame.
  double get scaleFactor {
    _checkOpen();
    return _scaleFactor;
  }

  /// Backend frame identity within [generation].
  BigInt get frameId {
    _checkOpen();
    return _frameId;
  }

  /// Borrowed OpenGL texture name.
  ScopedNativeInt get texture {
    _checkOpen();
    return ScopedNativeInt(
      _texture,
      checkValid: _checkOpen,
      debugName: 'OpenGL texture',
    );
  }

  /// Backend-native OpenGL texture target.
  int get target {
    _checkOpen();
    return _target;
  }

  /// Backend-native OpenGL internal format.
  int get internalFormat {
    _checkOpen();
    return _internalFormat;
  }

  /// Backend-native OpenGL pixel format.
  int get format {
    _checkOpen();
    return _format;
  }

  /// Backend-native OpenGL pixel type.
  int get type {
    _checkOpen();
    return _type;
  }

  /// Releases this frame. The borrowed texture name becomes invalid.
  void close() {
    if (_closed) {
      return;
    }
    _session._releaseTextureFrame(this);
    _leakReporter.close();
    _closed = true;
  }

  void _checkOpen() {
    if (_closed) {
      throwInvalidArgument('OpenGL texture frame has already been released');
    }
    final _ = _session._handle.raw;
  }
}

/// A reference to a map for attaching a render session, safe to send to another
/// isolate.
///
/// Produced by [MapHandle.attachRef]. Every attach function lives here rather
/// than on [MapHandle], because attaching is the one map operation that runs on
/// the render session's isolate instead of the map's.
///
/// This carries only the map's handle id and does not keep the map alive.
/// Attaching against a closed map is rejected as stale rather than binding the
/// session to a later map.
///
/// Every attach opens with [ensureAbiVersion], because an attach is routinely
/// the first native call on its isolate and a session created by a mismatched
/// library would leave the map attached with nothing able to detach it.
final class MapAttachRef {
  const MapAttachRef._(this._mapId);

  final int _mapId;

  NativeMap get _mapHandle => NativeMap(_mapId);

  /// Attaches a Metal native surface render target to the map.
  RenderSessionHandle attachMetalSurface(MetalSurfaceDescriptor descriptor) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_metal_surface_descriptor>();
      nativeDescriptor.ref = _metalSurfaceDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        raw.mln_metal_surface_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }

  /// Attaches a Vulkan native surface render target to the map.
  RenderSessionHandle attachVulkanSurface(VulkanSurfaceDescriptor descriptor) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_vulkan_surface_descriptor>();
      nativeDescriptor.ref = _vulkanSurfaceDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        raw.mln_vulkan_surface_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }

  /// Attaches an OpenGL native surface render target to the map.
  RenderSessionHandle attachOpenGLSurface(OpenGLSurfaceDescriptor descriptor) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_opengl_surface_descriptor>();
      nativeDescriptor.ref = _openglSurfaceDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        raw.mln_opengl_surface_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }

  /// Attaches a Metal texture render target owned by the render session.
  RenderSessionHandle attachMetalOwnedTexture(
    MetalOwnedTextureDescriptor descriptor,
  ) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_metal_owned_texture_descriptor>();
      nativeDescriptor.ref = _metalOwnedTextureDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        raw.mln_metal_owned_texture_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }

  /// Attaches a Metal caller-owned texture render target to the map.
  RenderSessionHandle attachMetalBorrowedTexture(
    MetalBorrowedTextureDescriptor descriptor,
  ) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeDescriptor =
          arena<raw.mln_metal_borrowed_texture_descriptor>();
      nativeDescriptor.ref = _metalBorrowedTextureDescriptorToNative(
        descriptor,
      );
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        raw.mln_metal_borrowed_texture_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }

  /// Attaches a Vulkan texture render target owned by the render session.
  RenderSessionHandle attachVulkanOwnedTexture(
    VulkanOwnedTextureDescriptor descriptor,
  ) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_vulkan_owned_texture_descriptor>();
      nativeDescriptor.ref = _vulkanOwnedTextureDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        raw.mln_vulkan_owned_texture_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }

  /// Attaches a Vulkan caller-owned texture render target to the map.
  RenderSessionHandle attachVulkanBorrowedTexture(
    VulkanBorrowedTextureDescriptor descriptor,
  ) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeDescriptor =
          arena<raw.mln_vulkan_borrowed_texture_descriptor>();
      nativeDescriptor.ref = _vulkanBorrowedTextureDescriptorToNative(
        descriptor,
      );
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        raw.mln_vulkan_borrowed_texture_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }

  /// Attaches an OpenGL texture render target owned by the render session.
  RenderSessionHandle attachOpenGLOwnedTexture(
    OpenGLOwnedTextureDescriptor descriptor,
  ) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_opengl_owned_texture_descriptor>();
      nativeDescriptor.ref = _openglOwnedTextureDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        raw.mln_opengl_owned_texture_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }

  /// Attaches an OpenGL caller-owned texture render target to the map.
  RenderSessionHandle attachOpenGLBorrowedTexture(
    OpenGLBorrowedTextureDescriptor descriptor,
  ) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final nativeDescriptor =
          arena<raw.mln_opengl_borrowed_texture_descriptor>();
      nativeDescriptor.ref = _openglBorrowedTextureDescriptorToNative(
        descriptor,
      );
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        raw.mln_opengl_borrowed_texture_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }
}
