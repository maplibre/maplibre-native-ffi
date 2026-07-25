part of 'runtime.dart';

final class _TextureFrameLease {
  _TextureFrameLease(Object owner, this.release)
    : owner = WeakReference<Object>(owner);

  final WeakReference<Object> owner;
  final void Function() release;
}

final class MapProjectionHandle {
  MapProjectionHandle._(Pointer<raw.mln_map_projection> pointer)
    : _state = NativeHandleState(pointer, 'MapProjectionHandle');

  final NativeHandleState<raw.mln_map_projection> _state;

  /// Whether this projection helper has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  Pointer<raw.mln_map_projection> get _pointer => _state.pointer;

  /// Copies the current projection camera options.
  CameraOptions camera() {
    return withNativeArena((arena) {
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      _check(_c.raw.mln_map_projection_get_camera(_pointer, outCamera));
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
      _check(_c.raw.mln_map_projection_set_camera(_pointer, nativeCamera));
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
          _pointer,
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
          _pointer,
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
          _pointer,
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
          _pointer,
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
      (pointer) => _c.raw.mln_map_projection_destroy(pointer).value,
      _c.threadLastErrorMessage,
    );
  }
}

/// Owner-thread render session handle attached to a retained map.
final class RenderSessionHandle {
  RenderSessionHandle._(this._map, Pointer<raw.mln_render_session> pointer)
    : _state = NativeHandleState(pointer, 'RenderSessionHandle');

  final MapHandle _map;
  final NativeHandleState<raw.mln_render_session> _state;
  _TextureFrameLease? _activeTextureFrame;

  /// Whether this render session has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  Pointer<raw.mln_render_session> get _pointer {
    final _ = _map._pointer;
    return _state.pointer;
  }

  /// Resizes an attached render session.
  void resize(int width, int height, {double scaleFactor = 1}) {
    _checkNoActiveTextureFrame('resize render session');
    _check(
      _c.raw.mln_render_session_resize(_pointer, width, height, scaleFactor),
    );
  }

  /// Processes the latest map render update for this session.
  void renderUpdate() {
    _checkNoActiveTextureFrame('render update');
    _check(_c.raw.mln_render_session_render_update(_pointer));
  }

  /// Detaches backend-bound render resources while keeping the handle live.
  void detach() {
    _checkNoActiveTextureFrame('detach render session');
    _check(_c.raw.mln_render_session_detach(_pointer));
  }

  /// Asks the session renderer to release cached resources where possible.
  void reduceMemoryUse() {
    _check(_c.raw.mln_render_session_reduce_memory_use(_pointer));
  }

  /// Clears renderer data for the session.
  void clearData() {
    _check(_c.raw.mln_render_session_clear_data(_pointer));
  }

  /// Dumps renderer debug logs through MapLibre Native logging.
  void dumpDebugLogs() {
    _check(_c.raw.mln_render_session_dump_debug_logs(_pointer));
  }

  /// Sets per-feature state on a render source.
  void setFeatureState(FeatureStateSelector selector, JsonObject state) {
    withNativeArena((arena) {
      final nativeSelector = _featureStateSelectorToNative(selector, arena);
      final nativeState = native_json.nativeJsonValue(state, arena);
      _check(
        _c.raw.mln_render_session_set_feature_state(
          _pointer,
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
      final outState = arena<Pointer<raw.mln_json_snapshot>>();
      outState.value = nullptr;
      _check(
        _c.raw.mln_render_session_get_feature_state(
          _pointer,
          nativeSelector,
          outState,
        ),
      );
      return _copyJsonSnapshot(outState.value);
    });
  }

  /// Removes per-feature state from a render source.
  void removeFeatureState(FeatureStateSelector selector) {
    withNativeArena((arena) {
      final nativeSelector = _featureStateSelectorToNative(selector, arena);
      _check(
        _c.raw.mln_render_session_remove_feature_state(
          _pointer,
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
      final outResult = arena<Pointer<raw.mln_feature_query_result>>();
      outResult.value = nullptr;
      _check(
        _c.raw.mln_render_session_query_rendered_features(
          _pointer,
          nativeGeometry,
          nativeOptions,
          outResult,
        ),
      );
      return _copyFeatureQueryResult(outResult.value);
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
      final outResult = arena<Pointer<raw.mln_feature_query_result>>();
      outResult.value = nullptr;
      _check(
        _c.raw.mln_render_session_query_source_features(
          _pointer,
          nativeSourceId.value,
          nativeOptions,
          outResult,
        ),
      );
      return _copyFeatureQueryResult(outResult.value);
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
      final outResult = arena<Pointer<raw.mln_feature_extension_result>>();
      outResult.value = nullptr;
      _check(
        _c.raw.mln_render_session_query_feature_extensions(
          _pointer,
          nativeSourceId.value,
          nativeFeature,
          nativeExtension.value,
          nativeExtensionField.value,
          nativeArguments,
          outResult,
        ),
      );
      return _copyFeatureExtensionResult(outResult.value);
    });
  }

  /// Reads the latest rendered session-owned texture as premultiplied RGBA8.
  TextureImageInfo textureImageInfo() {
    _checkNoActiveTextureFrame('read texture image info');
    return withNativeArena((arena) {
      final info = arena<raw.mln_texture_image_info>();
      info.ref = _c.raw.mln_texture_image_info_default();
      final probeStatus = _c.raw.mln_texture_read_premultiplied_rgba8(
        _pointer,
        nullptr.cast<Uint8>(),
        0,
        info,
      );
      if (_statusCode(probeStatus) != nativeStatusInvalidArgument ||
          info.ref.byte_length == 0) {
        _check(probeStatus);
      }
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
          _pointer,
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
          _pointer,
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
      _check(_c.raw.mln_metal_owned_texture_acquire_frame(_pointer, outFrame));
      try {
        return MetalOwnedTextureFrame._(this, outFrame);
      } catch (_) {
        _c.raw.mln_metal_owned_texture_release_frame(_pointer, outFrame);
        rethrow;
      }
    } catch (_) {
      calloc.free(outFrame);
      rethrow;
    }
  }

  /// Acquires the latest Vulkan texture frame until [VulkanOwnedTextureFrame.close].
  VulkanOwnedTextureFrame acquireVulkanTextureFrame() {
    _checkNoActiveTextureFrame('acquire Vulkan texture frame');
    final outFrame = calloc<raw.mln_vulkan_owned_texture_frame>();
    try {
      outFrame.ref.size = sizeOf<raw.mln_vulkan_owned_texture_frame>();
      _check(_c.raw.mln_vulkan_owned_texture_acquire_frame(_pointer, outFrame));
      try {
        return VulkanOwnedTextureFrame._(this, outFrame);
      } catch (_) {
        _c.raw.mln_vulkan_owned_texture_release_frame(_pointer, outFrame);
        rethrow;
      }
    } catch (_) {
      calloc.free(outFrame);
      rethrow;
    }
  }

  /// Acquires the latest OpenGL texture frame until [OpenGLOwnedTextureFrame.close].
  OpenGLOwnedTextureFrame acquireOpenGLTextureFrame() {
    _checkNoActiveTextureFrame('acquire OpenGL texture frame');
    final outFrame = calloc<raw.mln_opengl_owned_texture_frame>();
    try {
      outFrame.ref.size = sizeOf<raw.mln_opengl_owned_texture_frame>();
      _check(_c.raw.mln_opengl_owned_texture_acquire_frame(_pointer, outFrame));
      try {
        return OpenGLOwnedTextureFrame._(this, outFrame);
      } catch (_) {
        _c.raw.mln_opengl_owned_texture_release_frame(_pointer, outFrame);
        rethrow;
      }
    } catch (_) {
      calloc.free(outFrame);
      rethrow;
    }
  }

  /// Explicitly destroys this render session.
  void close() {
    _checkNoActiveTextureFrame('close render session');
    _state.close(
      (pointer) => _c.raw.mln_render_session_destroy(pointer).value,
      _c.threadLastErrorMessage,
    );
  }

  void _registerTextureFrame(Object frame, void Function() release) {
    _checkNoActiveTextureFrame('acquire texture frame');
    _activeTextureFrame = _TextureFrameLease(frame, release);
  }

  void _releaseTextureFrame(Object frame) {
    final lease = _activeTextureFrame;
    if (lease == null || !identical(lease.owner.target, frame)) {
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
    if (lease.owner.target != null) {
      throwInvalidState(
        '$operation requires releasing the active texture frame',
      );
    }
    lease.release();
    _activeTextureFrame = null;
  }
}

/// Releasable handle for a resource request owned by a Dart provider.
final class ResourceRequestHandle implements Finalizable {
  ResourceRequestHandle._(this._pointer)
    : _ownerIsolateHash = Isolate.current.hashCode {
    _leakReporter = NativeLeakReporter(
      this,
      'ResourceRequestHandle',
      _pointer.cast<Void>(),
    );
  }

  Pointer<raw.mln_resource_request_handle> _pointer;
  final int _ownerIsolateHash;
  late final NativeLeakReporter _leakReporter;
  var _released = false;

  /// Whether this provider reference has been released by Dart.
  bool get isReleased => _released;

  /// Moves this provider reference into an isolate-transferable token.
  ///
  /// The returned token owns completion and release. This handle becomes
  /// terminal immediately and must not be used again.
  ResourceRequestToken transfer() {
    final pointer = _livePointer;
    final token = _c.raw.mln_dart_resource_request_token_create(pointer);
    if (token == 0) {
      throwInvalidState('failed to create a transferable resource request');
    }
    _pointer = nullptr;
    _released = true;
    _leakReporter.close();
    return ResourceRequestToken._(token);
  }

  /// Reports whether MapLibre has cancelled this provider request.
  bool get isCancelled => cancelled();

  /// Reports whether MapLibre has cancelled this provider request.
  bool cancelled() {
    return withNativeArena((arena) {
      final outCancelled = arena<Bool>();
      _check(_c.raw.mln_resource_request_cancelled(_livePointer, outCancelled));
      return outCancelled.value;
    });
  }

  /// Completes this request with [response] and releases it. Completion is one-shot.
  void complete(ResourceResponse response) {
    _checkResourceResponseNativeStrings(response);
    final pointer = _livePointer;
    withNativeArena((arena) {
      final nativeResponse = arena<raw.mln_resource_response>();
      nativeResponse.ref = _resourceResponseToNative(response, arena);
      _pointer = nullptr;
      _released = true;
      _leakReporter.close();
      try {
        _check(_c.raw.mln_resource_request_complete(pointer, nativeResponse));
      } finally {
        _c.raw.mln_resource_request_release(pointer);
      }
    });
  }

  /// Releases the provider reference. The handle must not be used afterwards.
  void close() {
    _checkOwnerIsolate();
    if (_released) {
      return;
    }
    _c.raw.mln_resource_request_release(_pointer);
    _pointer = nullptr;
    _released = true;
    _leakReporter.close();
  }

  Pointer<raw.mln_resource_request_handle> get _livePointer {
    _checkOwnerIsolate();
    if (_released || _pointer == nullptr) {
      throwInvalidArgument('resource request handle has been released');
    }
    return _pointer;
  }

  void _checkOwnerIsolate() {
    if (Isolate.current.hashCode != _ownerIsolateHash) {
      throwWrongThread(
        'ResourceRequestHandle belongs to a different Dart isolate',
      );
    }
  }
}

/// Isolate-transferable ownership token for one resource request.
///
/// A token may be sent to another Dart isolate because it contains only an
/// opaque integer identity. [complete] and [close] are process-wide one-shot
/// operations even when copies of the token race across isolates.
final class ResourceRequestToken {
  ResourceRequestToken._(this._token);

  int _token;

  /// Whether this token copy has attempted its terminal operation.
  bool get isReleased => _token == 0;

  /// Reports whether MapLibre has cancelled this request.
  bool cancelled() {
    final token = _liveToken;
    return withNativeArena((arena) {
      final outCancelled = arena<Bool>();
      _checkResourceRequestToken(
        _c.raw.mln_dart_resource_request_token_cancelled(token, outCancelled),
      );
      return outCancelled.value;
    });
  }

  /// Completes and releases this request from the current isolate.
  void complete(ResourceResponse response) {
    _checkResourceResponseNativeStrings(response);
    withNativeArena((arena) {
      final nativeResponse = arena<raw.mln_resource_response>();
      nativeResponse.ref = _resourceResponseToNative(response, arena);
      final token = _takeToken();
      _checkResourceRequestToken(
        _c.raw.mln_dart_resource_request_token_complete(token, nativeResponse),
      );
    });
  }

  /// Releases this request without completing it.
  void close() {
    if (_token == 0) {
      return;
    }
    final token = _takeToken();
    _checkResourceRequestToken(
      _c.raw.mln_dart_resource_request_token_release(token),
    );
  }

  /// Blocks until another isolate completes or releases this request.
  ///
  /// This token copy becomes terminal when the remote terminal operation
  /// finishes.
  void waitUntilReleased() {
    final token = _liveToken;
    _checkResourceRequestToken(
      _c.raw.mln_dart_resource_request_token_wait(token),
    );
    _token = 0;
  }

  int get _liveToken {
    if (_token == 0) {
      throwInvalidArgument('resource request token has been released');
    }
    return _token;
  }

  int _takeToken() {
    final token = _liveToken;
    _token = 0;
    return token;
  }
}

void _checkResourceRequestToken(raw.mln_status status) {
  if (_statusCode(status) == nativeStatusInvalidArgument) {
    throwInvalidArgument('resource request token is no longer live');
  }
  _check(status);
}

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
      _frame.cast<Void>(),
    );
    try {
      final session = _session;
      final descriptor = _frame;
      _session._registerTextureFrame(this, () {
        _check(
          _c.raw.mln_metal_owned_texture_release_frame(
            session._pointer,
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
    final _ = _session._pointer;
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
      _frame.cast<Void>(),
    );
    try {
      final session = _session;
      final descriptor = _frame;
      _session._registerTextureFrame(this, () {
        _check(
          _c.raw.mln_vulkan_owned_texture_release_frame(
            session._pointer,
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
    final _ = _session._pointer;
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
      _frame.cast<Void>(),
    );
    try {
      final session = _session;
      final descriptor = _frame;
      _session._registerTextureFrame(this, () {
        _check(
          _c.raw.mln_opengl_owned_texture_release_frame(
            session._pointer,
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
    final _ = _session._pointer;
  }
}
