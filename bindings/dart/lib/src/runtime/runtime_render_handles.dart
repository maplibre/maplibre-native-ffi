part of 'runtime.dart';

final class _TextureFrameLease {
  _TextureFrameLease(Object owner, this.release)
    : owner = WeakReference<Object>(owner);

  _TextureFrameLease.orphaned(this.release) : owner = null;

  final WeakReference<Object>? owner;
  final void Function() release;
}

final class MapProjectionHandle {
  MapProjectionHandle._(NativeMapProjection handle)
    : _state = NativeHandleState(handle, 'MapProjectionHandle');

  final NativeHandleState<NativeMapProjection> _state;

  /// Whether this projection helper has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  NativeMapProjection get _handle => _state.handle;

  /// Copies the current projection camera options.
  CameraOptions camera() {
    return withNativeArena((arena) {
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      _check(_c.raw.mln_map_projection_get_camera(_handle.raw, outCamera));
      return native_struct.cameraOptionsFromNative(outCamera.ref);
    });
  }

  /// Applies camera fields to the projection helper.
  void setCamera(CameraOptions camera) {
    withNativeArena((arena) {
      final nativeCamera = arena<raw.mln_camera_options>();
      nativeCamera.ref = native_struct.cameraOptionsToNative(
        camera,
        _c.raw.mln_camera_options_default(),
      );
      _check(_c.raw.mln_map_projection_set_camera(_handle.raw, nativeCamera));
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
        _c.raw.mln_map_projection_set_visible_coordinates(
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
    Geometry geometry, {
    EdgeInsets padding = const EdgeInsets(),
  }) {
    withNativeArena((arena) {
      final nativeGeometry = native_geometry.nativeGeometry(geometry, arena);
      _check(
        _c.raw.mln_map_projection_set_visible_geometry(
          _handle.raw,
          nativeGeometry.pointer,
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
        _c.raw.mln_map_projection_pixel_for_lat_lng(
          _handle.raw,
          native_struct.latLngToNative(coordinate),
          outPoint,
        ),
      );
      return native_struct.screenPointFromNative(outPoint.ref);
    });
  }

  /// Converts a screen point to a geographic world coordinate.
  LatLng latLngForPixel(ScreenPoint point) {
    return withNativeArena((arena) {
      final outCoordinate = arena<raw.mln_lat_lng>();
      _check(
        _c.raw.mln_map_projection_lat_lng_for_pixel(
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
      (handle) => _c.raw.mln_map_projection_destroy(handle.raw).value,
      _c.threadLastErrorMessage,
    );
  }
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
  /// map's. It holds no Dart reference to the map: native keeps the map alive by
  /// refusing to destroy one that still has a session attached.
  NativeRenderSession get _handle => _state.handle;

  /// Resizes an attached render session.
  ///
  /// Surface and session-owned texture targets resize in place. A caller-owned
  /// texture is sized by its owner and is rejected here: allocate one at the
  /// new size and hand it over with [setMetalBorrowedTextureTarget] or its
  /// Vulkan or OpenGL counterpart, which keeps this session.
  ///
  /// This session keeps its renderer across a resize, so renderer-held state
  /// such as feature state carries over. A scale factor that differs from this
  /// session's current one is the exception: a renderer compiles its shaders
  /// for one pixel ratio, so that resize starts a new one with renderer-held
  /// state empty. The same exception applies to every `setTarget` method, which
  /// otherwise keeps the renderer.
  void resize(int width, int height, {double scaleFactor = 1}) {
    _checkNoActiveTextureFrame('resize render session');
    _check(
      _c.raw.mln_render_session_resize(_handle.raw, width, height, scaleFactor),
    );
  }

  /// Presents this attached surface session through a new Metal surface.
  ///
  /// A host surface can be destroyed and recreated while the map goes on
  /// living, which is what Android rotation, a Flutter `SurfaceProducer`
  /// lifecycle change, and a window resize that reallocates all look like from
  /// here. Replacing the surface in place keeps this session's renderer, and
  /// with it the tile pyramid, glyph and image atlases, symbol placement, and
  /// feature state.
  ///
  /// [descriptor] names the same graphics context this session attached with,
  /// and its extent applies as [resize] applies one. A descriptor whose
  /// context device is neither null nor this session's device throws an
  /// [InvalidArgumentException] and leaves this session rendering into the
  /// surface it has. The session assigns the layer its own device and pixel
  /// format, so the layer itself carries nothing that has to match.
  void setMetalSurfaceTarget(MetalSurfaceDescriptor descriptor) {
    _checkNoActiveTextureFrame('set Metal surface target');
    withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_metal_surface_descriptor>();
      nativeDescriptor.ref = _metalSurfaceDescriptorToNative(descriptor);
      _check(
        _c.raw.mln_metal_surface_set_target(_handle.raw, nativeDescriptor),
      );
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
      _check(
        _c.raw.mln_vulkan_surface_set_target(_handle.raw, nativeDescriptor),
      );
    });
  }

  /// Presents this attached surface session through a new OpenGL surface.
  ///
  /// See [setMetalSurfaceTarget] for what replacing a surface preserves. The
  /// new surface is made current on the next render, so a host may hand over a
  /// replacement for one it has already destroyed. A surface accepted here can
  /// still prove unusable, which the next [renderUpdate] reports rather than
  /// this call.
  void setOpenGLSurfaceTarget(OpenGLSurfaceDescriptor descriptor) {
    _checkNoActiveTextureFrame('set OpenGL surface target');
    withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_opengl_surface_descriptor>();
      nativeDescriptor.ref = _openglSurfaceDescriptorToNative(descriptor);
      _check(
        _c.raw.mln_opengl_surface_set_target(_handle.raw, nativeDescriptor),
      );
    });
  }

  /// Renders this attached texture session into a new caller-owned Metal
  /// texture.
  ///
  /// A caller-owned texture is sized by its owner, so a host that follows a
  /// resize reallocates rather than resizing and [resize] rejects the attempt.
  /// Handing the replacement over here keeps this session's renderer instead,
  /// so the map does not go cold on every resize.
  ///
  /// The replacement belongs to the device this session attached with, which
  /// throws an [InvalidArgumentException] otherwise, and carries the pixel
  /// format it attached with, which throws an [UnsupportedFeatureException]
  /// otherwise. Both leave this session rendering into the texture it has. The caller owns
  /// the replacement and keeps it valid until the next replacement, [detach],
  /// or [close]. This session never retained the outgoing texture and never
  /// releases it, but reads from it during this call, so keep that one valid
  /// until the call returns.
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
        _c.raw.mln_metal_borrowed_texture_set_target(
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
        _c.raw.mln_vulkan_borrowed_texture_set_target(
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
        _c.raw.mln_opengl_borrowed_texture_set_target(
          _handle.raw,
          nativeDescriptor,
        ),
      );
    });
  }

  /// Processes the latest map render update and reports whether it rendered.
  bool renderUpdate() {
    _checkNoActiveTextureFrame('render update');
    return withNativeArena((arena) {
      final rendered = arena<Bool>();
      _check(_c.raw.mln_render_session_render_update(_handle.raw, rendered));
      return rendered.value;
    });
  }

  /// Detaches backend-bound render resources while keeping the handle live.
  void detach() {
    _checkNoActiveTextureFrame('detach render session');
    _check(_c.raw.mln_render_session_detach(_handle.raw));
  }

  /// Asks the session renderer to release cached resources where possible.
  void reduceMemoryUse() {
    _check(_c.raw.mln_render_session_reduce_memory_use(_handle.raw));
  }

  /// Clears renderer data for the session.
  void clearData() {
    _check(_c.raw.mln_render_session_clear_data(_handle.raw));
  }

  /// Dumps renderer debug logs through MapLibre Native logging.
  void dumpDebugLogs() {
    _check(_c.raw.mln_render_session_dump_debug_logs(_handle.raw));
  }

  /// Sets per-feature state on a render source.
  void setFeatureState(FeatureStateSelector selector, JsonObject state) {
    withNativeArena((arena) {
      final nativeSelector = _featureStateSelectorToNative(selector, arena);
      final nativeState = native_json.nativeJsonValue(state, arena);
      _check(
        _c.raw.mln_render_session_set_feature_state(
          _handle.raw,
          nativeSelector,
          nativeState.pointer,
        ),
      );
    });
  }

  /// Copies per-feature state from a render source.
  JsonValue? getFeatureState(FeatureStateSelector selector) {
    return withNativeArena((arena) {
      final nativeSelector = _featureStateSelectorToNative(selector, arena);
      final outState = arena<Uint64>();
      outState.value = 0;
      _check(
        _c.raw.mln_render_session_get_feature_state(
          _handle.raw,
          nativeSelector,
          outState,
        ),
      );
      return _copyJsonSnapshot(NativeJsonSnapshot(outState.value));
    });
  }

  /// Removes per-feature state from a render source.
  void removeFeatureState(FeatureStateSelector selector) {
    withNativeArena((arena) {
      final nativeSelector = _featureStateSelectorToNative(selector, arena);
      _check(
        _c.raw.mln_render_session_remove_feature_state(
          _handle.raw,
          nativeSelector,
        ),
      );
    });
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
        _c.raw.mln_render_session_query_rendered_features(
          _handle.raw,
          nativeGeometry,
          nativeOptions,
          outResult,
        ),
      );
      return _copyFeatureQueryResult(NativeFeatureQueryResult(outResult.value));
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
        _c.raw.mln_render_session_query_source_features(
          _handle.raw,
          nativeSourceId.value,
          nativeOptions,
          outResult,
        ),
      );
      return _copyFeatureQueryResult(NativeFeatureQueryResult(outResult.value));
    });
  }

  /// Queries a feature extension from the latest render session state.
  FeatureExtensionResult queryFeatureExtensions({
    required String sourceId,
    required FeatureGeoJson feature,
    required String extension,
    required String extensionField,
    JsonValue? arguments,
  }) {
    return withNativeArena((arena) {
      final nativeSourceId = nativeStringView(sourceId, arena);
      final nativeFeature = native_geometry
          .nativeGeoJson(feature, arena)
          .pointer
          .ref
          .data
          .feature;
      final nativeExtension = nativeStringView(extension, arena);
      final nativeExtensionField = nativeStringView(extensionField, arena);
      final nativeArguments = arguments == null
          ? nullptr.cast<raw.mln_json_value>()
          : native_json.nativeJsonValue(arguments, arena).pointer;
      final outResult = arena<Uint64>();
      outResult.value = 0;
      _check(
        _c.raw.mln_render_session_query_feature_extensions(
          _handle.raw,
          nativeSourceId.value,
          nativeFeature,
          nativeExtension.value,
          nativeExtensionField.value,
          nativeArguments,
          outResult,
        ),
      );
      return _copyFeatureExtensionResult(
        NativeFeatureExtensionResult(outResult.value),
      );
    });
  }

  /// Reads the latest rendered session-owned texture as premultiplied RGBA8.
  TextureImageInfo textureImageInfo() {
    _checkNoActiveTextureFrame('read texture image info');
    return withNativeArena((arena) {
      final info = arena<raw.mln_texture_image_info>();
      info.ref = _c.raw.mln_texture_image_info_default();
      // A null buffer with zero capacity is a size probe that reports the
      // required byte length.
      _check(
        _c.raw.mln_texture_read_premultiplied_rgba8(
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
      info.ref = _c.raw.mln_texture_image_info_default();
      _check(
        _c.raw.mln_texture_read_premultiplied_rgba8(
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
      readInfo.ref = _c.raw.mln_texture_image_info_default();
      _check(
        _c.raw.mln_texture_read_premultiplied_rgba8(
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
      _check(
        _c.raw.mln_metal_owned_texture_acquire_frame(_handle.raw, outFrame),
      );
    } catch (_) {
      calloc.free(outFrame);
      rethrow;
    }
    return _constructAcquiredTextureFrame(
      outFrame,
      () => _c.raw.mln_metal_owned_texture_release_frame(_handle.raw, outFrame),
      () => MetalOwnedTextureFrame._(this, outFrame),
    );
  }

  /// Acquires the latest Vulkan texture frame until [VulkanOwnedTextureFrame.close].
  VulkanOwnedTextureFrame acquireVulkanTextureFrame() {
    _checkNoActiveTextureFrame('acquire Vulkan texture frame');
    final outFrame = calloc<raw.mln_vulkan_owned_texture_frame>();
    try {
      outFrame.ref.size = sizeOf<raw.mln_vulkan_owned_texture_frame>();
      _check(
        _c.raw.mln_vulkan_owned_texture_acquire_frame(_handle.raw, outFrame),
      );
    } catch (_) {
      calloc.free(outFrame);
      rethrow;
    }
    return _constructAcquiredTextureFrame(
      outFrame,
      () =>
          _c.raw.mln_vulkan_owned_texture_release_frame(_handle.raw, outFrame),
      () => VulkanOwnedTextureFrame._(this, outFrame),
    );
  }

  /// Acquires the latest OpenGL texture frame until [OpenGLOwnedTextureFrame.close].
  OpenGLOwnedTextureFrame acquireOpenGLTextureFrame() {
    _checkNoActiveTextureFrame('acquire OpenGL texture frame');
    final outFrame = calloc<raw.mln_opengl_owned_texture_frame>();
    try {
      outFrame.ref.size = sizeOf<raw.mln_opengl_owned_texture_frame>();
      _check(
        _c.raw.mln_opengl_owned_texture_acquire_frame(_handle.raw, outFrame),
      );
    } catch (_) {
      calloc.free(outFrame);
      rethrow;
    }
    return _constructAcquiredTextureFrame(
      outFrame,
      () =>
          _c.raw.mln_opengl_owned_texture_release_frame(_handle.raw, outFrame),
      () => OpenGLOwnedTextureFrame._(this, outFrame),
    );
  }

  /// Explicitly destroys this render session.
  void close() {
    _checkNoActiveTextureFrame('close render session');
    _state.close(
      (handle) => _c.raw.mln_render_session_destroy(handle.raw).value,
      _c.threadLastErrorMessage,
    );
  }

  void _registerTextureFrame(Object frame, void Function() release) {
    _checkNoActiveTextureFrame('acquire texture frame');
    _activeTextureFrame = _TextureFrameLease(frame, release);
  }

  T _constructAcquiredTextureFrame<T, F extends NativeType>(
    Pointer<F> descriptor,
    raw.mln_status Function() release,
    T Function() construct,
  ) {
    try {
      return construct();
    } catch (error, stackTrace) {
      cleanupFailedFrameConstruction(
        release: () => _statusCode(release()),
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
/// isolate and completed there. The C API validates the id on every call and
/// rejects a released one, which makes completion and release process-wide
/// one-shot operations even when copies of this handle race across isolates.
extension type const ResourceRequestHandle._(NativeResourceRequest _handle) {
  /// Reports whether MapLibre has cancelled this provider request.
  bool get isCancelled => cancelled();

  /// Reports whether MapLibre has cancelled this provider request.
  bool cancelled() {
    return withNativeArena((arena) {
      final outCancelled = arena<Bool>();
      _check(_c.raw.mln_resource_request_cancelled(_handle.raw, outCancelled));
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
        _check(
          _c.raw.mln_resource_request_complete(_handle.raw, nativeResponse),
        );
      } finally {
        _c.raw.mln_resource_request_release(_handle.raw);
      }
    });
  }

  /// Releases the provider reference without completing it.
  ///
  /// Releasing an already-retired request is a no-op in the C API, so this is
  /// safe to call from any isolate holding a copy.
  void close() {
    _c.raw.mln_resource_request_release(_handle.raw);
  }

  /// Blocks until this request is completed or released, wherever that happens.
  ///
  /// Only native can wait for that synchronously, so a host draining teardown
  /// across isolates uses this rather than building its own rendezvous.
  void waitUntilRetired() {
    _check(_c.raw.mln_resource_request_wait_until_retired(_handle.raw));
  }
}

/// Exposes an attach reference's map id for tests that must reach the C API
/// with a raw id. The safe API has no way to express those calls.
int mapAttachRefIdForTesting(MapAttachRef ref) => ref._mapId;

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
          _c.raw.mln_metal_owned_texture_release_frame(
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
    _imageAddress = frame.image.address;
    _imageViewAddress = frame.image_view.address;
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
          _c.raw.mln_vulkan_owned_texture_release_frame(
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
  late final int _imageAddress;
  late final int _imageViewAddress;
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

  /// Unsafe borrowed VkImage pointer.
  ///
  /// The pointer is valid only until [close] releases this frame.
  ScopedNativePointer get unsafeImage =>
      _borrowedPointer(_imageAddress, 'Vulkan image');

  /// Unsafe borrowed VkImageView pointer.
  ///
  /// The pointer is valid only until [close] releases this frame.
  ScopedNativePointer get unsafeImageView =>
      _borrowedPointer(_imageViewAddress, 'Vulkan image view');

  /// Unsafe borrowed VkDevice pointer.
  ///
  /// The pointer is valid only until [close] releases this frame.
  ScopedNativePointer get unsafeDevice =>
      _borrowedPointer(_deviceAddress, 'Vulkan device');

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
          _c.raw.mln_opengl_owned_texture_release_frame(
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
/// This carries only the map's handle id, because a [MapHandle] cannot cross
/// isolates. It does not keep the map alive: native refuses to destroy a map
/// that still has a session attached, and validates the id under its own
/// registry lock, so attaching against a closed map is rejected as stale rather
/// than binding the session to a later map.
final class MapAttachRef {
  const MapAttachRef._(this._mapId);

  final int _mapId;

  NativeMap get _mapHandle => NativeMap(_mapId);

  /// Attaches a Metal native surface render target to the map.
  RenderSessionHandle attachMetalSurface(MetalSurfaceDescriptor descriptor) {
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_metal_surface_descriptor>();
      nativeDescriptor.ref = _metalSurfaceDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        _c.raw.mln_metal_surface_attach(
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
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_vulkan_surface_descriptor>();
      nativeDescriptor.ref = _vulkanSurfaceDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        _c.raw.mln_vulkan_surface_attach(
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
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_opengl_surface_descriptor>();
      nativeDescriptor.ref = _openglSurfaceDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        _c.raw.mln_opengl_surface_attach(
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
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_metal_owned_texture_descriptor>();
      nativeDescriptor.ref = _metalOwnedTextureDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        _c.raw.mln_metal_owned_texture_attach(
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
    return withNativeArena((arena) {
      final nativeDescriptor =
          arena<raw.mln_metal_borrowed_texture_descriptor>();
      nativeDescriptor.ref = _metalBorrowedTextureDescriptorToNative(
        descriptor,
      );
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        _c.raw.mln_metal_borrowed_texture_attach(
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
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_vulkan_owned_texture_descriptor>();
      nativeDescriptor.ref = _vulkanOwnedTextureDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        _c.raw.mln_vulkan_owned_texture_attach(
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
    return withNativeArena((arena) {
      final nativeDescriptor =
          arena<raw.mln_vulkan_borrowed_texture_descriptor>();
      nativeDescriptor.ref = _vulkanBorrowedTextureDescriptorToNative(
        descriptor,
      );
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        _c.raw.mln_vulkan_borrowed_texture_attach(
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
    return withNativeArena((arena) {
      final nativeDescriptor = arena<raw.mln_opengl_owned_texture_descriptor>();
      nativeDescriptor.ref = _openglOwnedTextureDescriptorToNative(descriptor);
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        _c.raw.mln_opengl_owned_texture_attach(
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
    return withNativeArena((arena) {
      final nativeDescriptor =
          arena<raw.mln_opengl_borrowed_texture_descriptor>();
      nativeDescriptor.ref = _openglBorrowedTextureDescriptorToNative(
        descriptor,
      );
      final outSession = arena<Uint64>();
      outSession.value = 0;
      _check(
        _c.raw.mln_opengl_borrowed_texture_attach(
          _mapHandle.raw,
          nativeDescriptor,
          outSession,
        ),
      );
      return RenderSessionHandle._(NativeRenderSession(outSession.value));
    });
  }
}
