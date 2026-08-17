part of 'runtime.dart';

/// Owned standalone projection snapshot created from a map.
///
/// Every method is synchronous, runs on the calling isolate's thread, is
/// internally serialized, and may be called from any isolate. A projection
/// copies the map's transform state at creation and never observes map
/// changes made after that; a live projection prevents its map from closing.
final class MapProjectionHandle {
  MapProjectionHandle._(NativeMapProjection handle)
    : _state = NativeHandleState(handle, 'MapProjectionHandle');

  final NativeHandleState<NativeMapProjection> _state;

  /// Whether this projection helper has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  NativeMapProjection get _handle => _state.handle;

  /// Copies the projection camera, observing every earlier projection setter.
  CameraOptions camera() {
    return withNativeArena((arena) {
      final outCamera = arena<raw.mln_camera_options>();
      outCamera.ref.size = sizeOf<raw.mln_camera_options>();
      _check(raw.mln_map_projection_get_camera(_handle.raw, outCamera));
      return native_struct.cameraOptionsFromNative(outCamera.ref);
    });
  }

  /// Applies camera fields before returning, so a later read or conversion
  /// observes them. The map's camera is unaffected.
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

  /// Applies a camera fit for [coordinates] before returning.
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

  /// Applies a camera fit for GeoJSON Geometry bytes before returning.
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

  /// Converts a geographic coordinate to a logical-pixel screen point.
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

  /// Converts a logical-pixel screen point to a geographic coordinate.
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

  /// Closes the projection, waiting for projection calls already running on
  /// other threads, and releases its map reservation before returning.
  void close() {
    _state.close(
      (handle) => raw.mln_map_projection_close(handle.raw),
      _c.threadLastErrorMessage,
    );
  }
}

/// Outcome of one accepted frame demand.
final class RenderResult {
  const RenderResult._(this.rawValue, this.name);

  static const rendered = RenderResult._(0, 'rendered');
  static const noUpdate = RenderResult._(1, 'noUpdate');
  static const sizePending = RenderResult._(2, 'sizePending');
  static const targetNotReady = RenderResult._(3, 'targetNotReady');
  static const superseded = RenderResult._(4, 'superseded');
  static const deadlineMissed = RenderResult._(5, 'deadlineMissed');

  factory RenderResult.fromRawValue(int value) => switch (value) {
    0 => rendered,
    1 => noUpdate,
    2 => sizePending,
    3 => targetNotReady,
    4 => superseded,
    5 => deadlineMissed,
    _ => RenderResult._(value, 'unknown($value)'),
  };

  final int rawValue;
  final String name;

  @override
  bool operator ==(Object other) =>
      other is RenderResult && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Render execution placement selected during attachment.
enum RenderDriver {
  coreWorker(1),
  callerGraphicsThread(2);

  const RenderDriver(this.rawValue);
  final int rawValue;
}

/// Common policy for a render-session attachment.
final class RenderSessionAttachOptions {
  const RenderSessionAttachOptions({
    this.driver = RenderDriver.coreWorker,
    this.requestedTextureRingDepth = 1,
  });

  final RenderDriver driver;
  final int requestedTextureRingDepth;
}

/// One nonblocking request for a rendered frame.
final class FrameDemand {
  const FrameDemand({
    this.renderIfNeeded = true,
    this.present = false,
    this.token = 0,
    this.coalescingBoundary = 0,
    this.presentationTimeNanoseconds = 0,
    this.deadlineNanoseconds = 0,
  });

  final bool renderIfNeeded;
  final bool present;
  final int token;
  final int coalescingBoundary;
  final int presentationTimeNanoseconds;
  final int deadlineNanoseconds;
}

/// Immutable terminal result for one accepted frame demand.
final class RenderFrameResult {
  const RenderFrameResult._({
    required this.disposition,
    required this.token,
    required this.mapUpdateGeneration,
    required this.extentGeneration,
    required this.frameGeneration,
    required this.presentationTimeNanoseconds,
    required this.needsRepaint,
  });

  factory RenderFrameResult._fromNative(raw.mln_render_frame_result value) =>
      RenderFrameResult._(
        disposition: RenderResult.fromRawValue(value.disposition),
        token: value.token,
        mapUpdateGeneration: value.map_update_generation,
        extentGeneration: value.extent_generation,
        frameGeneration: value.frame_generation,
        presentationTimeNanoseconds: value.presentation_time_ns,
        needsRepaint: value.needs_repaint,
      );

  final RenderResult disposition;
  final int token;
  final int mapUpdateGeneration;
  final int extentGeneration;
  final int frameGeneration;
  final int presentationTimeNanoseconds;

  /// Whether the map asked for another frame while it rendered this one, as
  /// during an ongoing camera transition.
  ///
  /// Meaningful only when [disposition] is [RenderResult.rendered]; it reads
  /// false for every other outcome.
  final bool needsRepaint;
}

/// Immutable capabilities fixed during attachment.
final class RenderSessionCapabilities {
  const RenderSessionCapabilities._(
    this.driver,
    this.textureRingDepth,
    this.flags,
  );

  final RenderDriver driver;
  final int textureRingDepth;
  final int flags;
  bool get supportsFrameAcquisition => flags & 1 != 0;
  bool get supportsReadback => flags & 2 != 0;
  bool get supportsConsumerSync => flags & 4 != 0;
  bool get supportsPresentation => flags & 8 != 0;
}

/// Latest any-thread render-session state and generations.
final class RenderSessionSnapshot {
  const RenderSessionSnapshot._({
    required this.state,
    required this.driver,
    required this.latestResult,
    required this.extent,
    required this.generation,
    required this.mapUpdateGeneration,
    required this.renderedUpdateGeneration,
    required this.extentGeneration,
    required this.frameGeneration,
    required this.latestDemandToken,
    required this.pendingDemandCount,
    required this.acquiredFrameCount,
    required this.targetReady,
    required this.pendingChanges,
  });

  final int state;
  final RenderDriver driver;
  final RenderResult latestResult;
  final RenderTargetExtent extent;
  final int generation;
  final int mapUpdateGeneration;
  final int renderedUpdateGeneration;
  final int extentGeneration;
  final int frameGeneration;
  final int latestDemandToken;
  final int pendingDemandCount;
  final int acquiredFrameCount;
  final bool targetReady;
  final bool pendingChanges;
}

/// Result of irreversible target abandonment.
final class RenderAbandonResult {
  const RenderAbandonResult._(this.quarantined, this.quarantinedResourceCount);

  final bool quarantined;
  final int quarantinedResourceCount;
}

/// Synchronization that protects an acquired texture frame.
final class GpuSync {
  const GpuSync.cpuComplete() : kind = 0, object = null, value = 0;
  const GpuSync.native({
    required this.kind,
    required this.object,
    required this.value,
  });

  final int kind;
  final NativePointer? object;
  final int value;
}

/// An attachment that exposes the session while graphics initialization runs.
final class RenderSessionAttachment {
  const RenderSessionAttachment(this.session, this.completed);

  final RenderSessionHandle session;
  final Future<void> completed;
}

/// A render session attached to one map and render target.
final class RenderSessionHandle implements Finalizable {
  RenderSessionHandle._(this._runtime, NativeRenderSession handle)
    : _state = NativeHandleState(handle, 'RenderSessionHandle') {
    _runtime._renderSessions[handle.raw] = WeakReference(this);
  }

  final RuntimeHandle _runtime;
  final NativeHandleState<NativeRenderSession> _state;
  final _frameResultsReady = StreamController<void>.broadcast(sync: true);
  final _driverWorkReady = StreamController<void>.broadcast(sync: true);

  /// Notifications that frame results are available to drain.
  Stream<void> get frameResultsReady => _frameResultsReady.stream;

  /// Notifications that caller-graphics-thread work is ready to service.
  Stream<void> get driverWorkReady => _driverWorkReady.stream;

  void _notifyFramesReady() => _frameResultsReady.add(null);
  void _notifyDriverWorkReady() => _driverWorkReady.add(null);
  bool get isClosed => _state.isClosed;
  Future<void> _completeWhileRetained(Future<void> completion) async {
    await completion;
  }

  NativeRenderSession get _handle => _state.handle;

  RenderSessionCapabilities get capabilities => withNativeArena((arena) {
    final out = arena<raw.mln_render_session_capabilities>()
      ..ref.size = sizeOf<raw.mln_render_session_capabilities>();
    _check(raw.mln_render_session_get_capabilities(_handle.raw, out));
    return RenderSessionCapabilities._(
      RenderDriver.values.firstWhere(
        (value) => value.rawValue == out.ref.driver,
      ),
      out.ref.texture_ring_depth,
      out.ref.flags,
    );
  });

  RenderSessionSnapshot get snapshot => withNativeArena((arena) {
    final out = arena<raw.mln_render_session_snapshot>()
      ..ref.size = sizeOf<raw.mln_render_session_snapshot>();
    _check(raw.mln_render_session_get_snapshot(_handle.raw, out));
    final value = out.ref;
    return RenderSessionSnapshot._(
      state: value.state,
      driver: RenderDriver.values.firstWhere(
        (driver) => driver.rawValue == value.driver,
      ),
      latestResult: RenderResult.fromRawValue(value.latest_result),
      extent: RenderTargetExtent(
        width: value.extent.width,
        height: value.extent.height,
        scaleFactor: value.extent.scale_factor,
      ),
      generation: value.generation,
      mapUpdateGeneration: value.map_update_generation,
      renderedUpdateGeneration: value.rendered_update_generation,
      extentGeneration: value.extent_generation,
      frameGeneration: value.frame_generation,
      latestDemandToken: value.latest_demand_token,
      pendingDemandCount: value.pending_demand_count,
      acquiredFrameCount: value.acquired_frame_count,
      targetReady: value.target_ready,
      pendingChanges: value.pending_changes,
    );
  });

  void requestFrame([FrameDemand demand = const FrameDemand()]) {
    withNativeArena((arena) {
      final native = arena<raw.mln_frame_demand>()
        ..ref = raw.mln_frame_demand_default();
      native.ref.flags =
          (demand.renderIfNeeded ? 1 : 0) | (demand.present ? 2 : 0);
      native.ref.token = demand.token;
      native.ref.coalescing_boundary = demand.coalescingBoundary;
      native.ref.presentation_time_ns = demand.presentationTimeNanoseconds;
      native.ref.deadline_ns = demand.deadlineNanoseconds;
      _check(raw.mln_render_session_request_frame(_handle.raw, native));
    });
  }

  List<RenderFrameResult> drainFrameResults() {
    return withNativeArena((arena) {
      final outBatch = arena<Uint64>()..value = 0;
      _check(raw.mln_render_session_drain_frame_results(_handle.raw, outBatch));
      final batch = outBatch.value;
      if (batch == 0) return const <RenderFrameResult>[];
      try {
        final count = arena<Size>();
        _check(raw.mln_render_frame_batch_count(batch, count));
        return List<RenderFrameResult>.generate(count.value, (index) {
          final out = arena<raw.mln_render_frame_result>()
            ..ref.size = sizeOf<raw.mln_render_frame_result>();
          _check(raw.mln_render_frame_batch_get(batch, index, out));
          return RenderFrameResult._fromNative(out.ref);
        }, growable: false);
      } finally {
        raw.mln_render_frame_batch_release(batch);
      }
    });
  }

  int serviceDriverWork({int maxWork = 0}) {
    if (maxWork < 0) {
      throwInvalidArgument('maxWork must not be negative');
    }
    return withNativeArena((arena) {
      final serviced = arena<Size>();
      _check(
        raw.mln_render_session_service_driver_work(
          _handle.raw,
          maxWork,
          serviced,
        ),
      );
      return serviced.value;
    });
  }

  Future<void> resize(RenderTargetExtent extent) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_render_target_extent>()
        ..ref = _renderTargetExtentToNative(extent);
      return raw.mln_render_session_resize_start(_handle.raw, native, out);
    }),
  );

  Future<void> setMetalSurfaceTarget(MetalSurfaceDescriptor descriptor) =>
      _voidOperation(
        (out) => withNativeArena((arena) {
          final native = arena<raw.mln_metal_surface_descriptor>()
            ..ref = _metalSurfaceDescriptorToNative(descriptor);
          return raw.mln_metal_surface_set_target_start(
            _handle.raw,
            native,
            out,
          );
        }),
      );

  Future<void> setVulkanSurfaceTarget(VulkanSurfaceDescriptor descriptor) =>
      _voidOperation(
        (out) => withNativeArena((arena) {
          final native = arena<raw.mln_vulkan_surface_descriptor>()
            ..ref = _vulkanSurfaceDescriptorToNative(descriptor);
          return raw.mln_vulkan_surface_set_target_start(
            _handle.raw,
            native,
            out,
          );
        }),
      );

  Future<void> setOpenGLSurfaceTarget(OpenGLSurfaceDescriptor descriptor) =>
      _voidOperation(
        (out) => withNativeArena((arena) {
          final native = arena<raw.mln_opengl_surface_descriptor>()
            ..ref = _openglSurfaceDescriptorToNative(descriptor, arena);
          return raw.mln_opengl_surface_set_target_start(
            _handle.raw,
            native,
            out,
          );
        }),
      );

  Future<void> setWebGPUSurfaceTarget(WebGPUSurfaceDescriptor descriptor) =>
      _voidOperation(
        (out) => withNativeArena((arena) {
          final native = arena<raw.mln_webgpu_surface_descriptor>()
            ..ref = _webGPUSurfaceDescriptorToNative(descriptor);
          return raw.mln_webgpu_surface_set_target_start(
            _handle.raw,
            native,
            out,
          );
        }),
      );

  Future<void> setMetalBorrowedTextureTarget(
    MetalBorrowedTextureDescriptor descriptor,
  ) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_metal_borrowed_texture_descriptor>()
        ..ref = _metalBorrowedTextureDescriptorToNative(descriptor);
      return raw.mln_metal_borrowed_texture_set_target_start(
        _handle.raw,
        native,
        out,
      );
    }),
  );

  Future<void> setVulkanBorrowedTextureTarget(
    VulkanBorrowedTextureDescriptor descriptor,
  ) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_vulkan_borrowed_texture_descriptor>()
        ..ref = _vulkanBorrowedTextureDescriptorToNative(descriptor);
      return raw.mln_vulkan_borrowed_texture_set_target_start(
        _handle.raw,
        native,
        out,
      );
    }),
  );

  Future<void> setOpenGLBorrowedTextureTarget(
    OpenGLBorrowedTextureDescriptor descriptor,
  ) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_opengl_borrowed_texture_descriptor>()
        ..ref = _openglBorrowedTextureDescriptorToNative(descriptor, arena);
      return raw.mln_opengl_borrowed_texture_set_target_start(
        _handle.raw,
        native,
        out,
      );
    }),
  );

  Future<void> setWebGPUBorrowedTextureTarget(
    WebGPUBorrowedTextureDescriptor descriptor,
  ) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_webgpu_borrowed_texture_descriptor>()
        ..ref = _webGPUBorrowedTextureDescriptorToNative(descriptor);
      return raw.mln_webgpu_borrowed_texture_set_target_start(
        _handle.raw,
        native,
        out,
      );
    }),
  );

  Future<void> barrier({int minUpdateGeneration = 0}) => _voidOperation(
    (out) => raw.mln_render_session_barrier_start(
      _handle.raw,
      minUpdateGeneration,
      out,
    ),
  );

  Future<void> reduceMemoryUse() => _voidOperation(
    (out) => raw.mln_render_session_reduce_memory_use_start(_handle.raw, out),
  );
  Future<void> clearData() => _voidOperation(
    (out) => raw.mln_render_session_clear_data_start(_handle.raw, out),
  );
  Future<void> dumpDebugLogs() => _voidOperation(
    (out) => raw.mln_render_session_dump_debug_logs_start(_handle.raw, out),
  );

  Future<List<QueriedFeature>> queryRenderedFeatures(
    RenderedQueryGeometry geometry, {
    RenderedFeatureQueryOptions? options,
  }) => withNativeArena((arena) {
    final nativeGeometry = arena<raw.mln_rendered_query_geometry>()
      ..ref = _renderedQueryGeometryToNative(geometry, arena);
    final nativeOptions = _renderedFeatureQueryOptionsToNative(
      options ?? RenderedFeatureQueryOptions(),
      arena,
    );
    final operation = arena<Uint64>()..value = 0;
    _check(
      raw.mln_render_session_query_rendered_features_start(
        _handle.raw,
        nativeGeometry,
        nativeOptions,
        operation,
      ),
    );
    return _takeQueriedFeaturesOperation(operation.value);
  });

  Future<List<QueriedFeature>> querySourceFeatures(
    String sourceId, {
    SourceFeatureQueryOptions? options,
  }) => withNativeArena((arena) {
    final nativeOptions = _sourceFeatureQueryOptionsToNative(
      options ?? SourceFeatureQueryOptions(),
      arena,
    );
    final operation = arena<Uint64>()..value = 0;
    _check(
      raw.mln_render_session_query_source_features_start(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        nativeOptions,
        operation,
      ),
    );
    return _takeQueriedFeaturesOperation(operation.value);
  });

  Future<Uint8List> queryFeatureExtensions({
    required String sourceId,
    required Uint8List feature,
    required String extension,
    required String extensionField,
    Uint8List? arguments,
  }) => withNativeArena((arena) {
    final operation = arena<Uint64>()..value = 0;
    final nativeArguments = arguments == null
        ? nullptr.cast<raw.mln_buffer_view>()
        : (arena<raw.mln_buffer_view>()
            ..ref = nativeBufferView(arguments, arena));
    _check(
      raw.mln_render_session_query_feature_extensions_start(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        nativeBufferView(feature, arena),
        nativeStringView(extension, arena).value,
        nativeStringView(extensionField, arena).value,
        nativeArguments,
        operation,
      ),
    );
    return _takeBufferOperation(
      operation.value,
      raw.mln_render_query_take_result,
    );
  });

  Future<Uint8List> _takeBufferOperation(
    int operation,
    int Function(int, Pointer<raw.mln_buffer>) take,
  ) => _runtime._takeOperation(operation, () {
    return withNativeArena((arena) {
      final result = arena<Uint64>()..value = 0;
      _check(take(operation, result.cast<raw.mln_buffer>()));
      return copyOwnedBuffer(NativeOwnedBufferHandle(result.value));
    });
  });

  Future<List<QueriedFeature>> _takeQueriedFeaturesOperation(int operation) =>
      _runtime._takeOperation(operation, () {
        return withNativeArena((arena) {
          final result = arena<Uint64>()..value = 0;
          _check(raw.mln_render_query_features_take_result(operation, result));
          return _copyQueriedFeatureList(
            NativeQueriedFeatureList(result.value),
          );
        });
      });

  Future<void> setFeatureState(
    FeatureStateSelector selector,
    Uint8List state,
  ) => withNativeArena((arena) {
    final operation = arena<Uint64>()..value = 0;
    _check(
      raw.mln_render_session_set_feature_state_start(
        _handle.raw,
        nativeStringView(selector.sourceId, arena).value,
        nativeStringView(selector.sourceLayerId ?? '', arena).value,
        nativeStringView(selector.featureId ?? '', arena).value,
        nativeBufferView(state, arena),
        operation,
      ),
    );
    return _runtime._finishOperation(operation.value);
  });

  Future<Uint8List> getFeatureState(FeatureStateSelector selector) =>
      withNativeArena((arena) {
        final operation = arena<Uint64>()..value = 0;
        _check(
          raw.mln_render_session_get_feature_state_start(
            _handle.raw,
            nativeStringView(selector.sourceId, arena).value,
            nativeStringView(selector.sourceLayerId ?? '', arena).value,
            nativeStringView(selector.featureId ?? '', arena).value,
            operation,
          ),
        );
        return _takeBufferOperation(
          operation.value,
          raw.mln_render_session_get_feature_state_take_result,
        );
      });

  Future<void> removeFeatureState(FeatureStateSelector selector) =>
      withNativeArena((arena) {
        final operation = arena<Uint64>()..value = 0;
        _check(
          raw.mln_render_session_remove_feature_state_start(
            _handle.raw,
            nativeStringView(selector.sourceId, arena).value,
            nativeStringView(selector.sourceLayerId ?? '', arena).value,
            nativeStringView(selector.featureId ?? '', arena).value,
            nativeStringView(selector.stateKey ?? '', arena).value,
            operation,
          ),
        );
        return _runtime._finishOperation(operation.value);
      });

  Future<TextureImage> readPremultipliedRgba8() => withNativeArena((arena) {
    final operation = arena<Uint64>()..value = 0;
    _check(
      raw.mln_texture_read_premultiplied_rgba8_start(_handle.raw, operation),
    );
    final id = operation.value;
    return _runtime._takeOperation(
      id,
      () => withNativeArena((resultArena) {
        final data = resultArena<Uint64>()..value = 0;
        final info = resultArena<raw.mln_texture_image_info>()
          ..ref = raw.mln_texture_image_info_default();
        _check(
          raw.mln_texture_read_premultiplied_rgba8_take_result(id, data, info),
        );
        return TextureImage(
          info: TextureImageInfo._fromNative(info.ref),
          bytes: copyOwnedBuffer(NativeOwnedBufferHandle(data.value)),
        );
      }),
    );
  });

  AcquiredFrame acquireFrame() => withNativeArena((arena) {
    final out = arena<Uint64>()..value = 0;
    _check(raw.mln_render_session_acquire_frame(_handle.raw, out));
    return AcquiredFrame._(_runtime, out.value);
  });

  Future<void> detach() => _voidOperation(
    (out) => raw.mln_render_session_detach_start(_handle.raw, out),
  );

  RenderAbandonResult abandon() => withNativeArena((arena) {
    final out = arena<raw.mln_render_abandon_result>()
      ..ref.size = sizeOf<raw.mln_render_abandon_result>();
    _check(raw.mln_render_session_abandon(_handle.raw, out));
    return RenderAbandonResult._(
      out.ref.disposition == 1,
      out.ref.quarantined_resource_count,
    );
  });

  Future<void> _voidOperation(
    int Function(Pointer<Uint64> outOperation) start,
  ) => withNativeArena((arena) {
    final out = arena<Uint64>()..value = 0;
    _check(start(out));
    return _runtime._finishOperation(out.value);
  });

  void close() {
    final id = _state.handleId;
    _state.close(
      (handle) => raw.mln_render_session_destroy(handle.raw),
      _c.threadLastErrorMessage,
    );
    _runtime._renderSessions.remove(id);
    _frameResultsReady.close();
    _driverWorkReady.close();
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
      }
    });
  }

  /// Releases the provider reference without completing it.
  ///
  /// Releasing an already-retired request is a no-op in the C API, so this is
  /// safe to call from any isolate holding a copy.
  void close() {
    raw.mln_resource_request_release(_checked.raw);
  }

  /// Blocks until this request is completed or released, wherever that happens.
  void waitUntilRetired() {
    _check(raw.mln_resource_request_wait_until_retired(_checked.raw));
  }
}

/// Exposes a map's handle id for tests that must reach the C API with a raw id.
int mapHandleIdForTesting(MapHandle map) => map._state.handleId;

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

/// Scoped lease on one rendered texture-ring slot.
final class AcquiredFrame {
  AcquiredFrame._(this._runtime, this._handle);

  final RuntimeHandle _runtime;
  int _handle;

  void _checkOpen() {
    if (_handle == 0) {
      throwInvalidState('acquired frame has already been released');
    }
  }

  RenderFrameResult get result => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_render_frame_result>()
      ..ref.size = sizeOf<raw.mln_render_frame_result>();
    _check(raw.mln_acquired_frame_get_result(_handle, out));
    return RenderFrameResult._fromNative(out.ref);
  });

  GpuSync get producerSync => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_gpu_sync>()
      ..ref.size = sizeOf<raw.mln_gpu_sync>();
    _check(raw.mln_acquired_frame_get_producer_sync(_handle, out));
    return GpuSync.native(
      kind: out.ref.kind,
      object: out.ref.object == nullptr
          ? null
          : NativePointer(out.ref.object.address),
      value: out.ref.value,
    );
  });

  WebGPUOwnedTextureFrame get webGPUTexture => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_webgpu_owned_texture_frame>()
      ..ref.size = sizeOf<raw.mln_webgpu_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_webgpu_texture(_handle, out));
    return WebGPUOwnedTextureFrame._fromNative(out.ref, this);
  });

  MetalOwnedTextureFrame get metalTexture => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_metal_owned_texture_frame>()
      ..ref.size = sizeOf<raw.mln_metal_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_metal_texture(_handle, out));
    return MetalOwnedTextureFrame._fromNative(out.ref, this);
  });

  VulkanOwnedTextureFrame get vulkanTexture => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_vulkan_owned_texture_frame>()
      ..ref.size = sizeOf<raw.mln_vulkan_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_vulkan_texture(_handle, out));
    return VulkanOwnedTextureFrame._fromNative(out.ref, this);
  });

  OpenGLOwnedTextureFrame get openGLTexture => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_opengl_owned_texture_frame>()
      ..ref.size = sizeOf<raw.mln_opengl_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_opengl_texture(_handle, out));
    return OpenGLOwnedTextureFrame._fromNative(out.ref, this);
  });

  Future<void> release({GpuSync sync = const GpuSync.cpuComplete()}) =>
      withNativeArena((arena) {
        _checkOpen();
        final frame = arena<Uint64>()..value = _handle;
        final nativeSync = arena<raw.mln_gpu_sync>()
          ..ref = raw.mln_gpu_sync_default()
          ..ref.kind = sync.kind
          ..ref.object = Pointer<Void>.fromAddress(sync.object?.address ?? 0)
          ..ref.value = sync.value;
        final operation = arena<Uint64>()..value = 0;
        _check(
          raw.mln_acquired_frame_release_start(frame, nativeSync, operation),
        );
        _handle = 0;
        return _runtime._finishOperation(operation.value);
      });
}

/// Scoped Metal metadata from an [AcquiredFrame].
final class MetalOwnedTextureFrame {
  const MetalOwnedTextureFrame._(
    this.generation,
    this.width,
    this.height,
    this.scaleFactor,
    this.frameId,
    this.pixelFormat,
    this._textureAddress,
    this._deviceAddress,
    this._owner,
  );

  factory MetalOwnedTextureFrame._fromNative(
    raw.mln_metal_owned_texture_frame value,
    AcquiredFrame owner,
  ) => MetalOwnedTextureFrame._(
    value.generation,
    value.width,
    value.height,
    value.scale_factor,
    value.frame_id,
    value.pixel_format,
    value.texture.address,
    value.device.address,
    owner,
  );

  final int generation;
  final int width;
  final int height;
  final double scaleFactor;
  final int frameId;
  final int pixelFormat;
  final int _textureAddress;
  final int _deviceAddress;
  final AcquiredFrame _owner;
  ScopedNativePointer get unsafeTexture =>
      _scopedPointer(_textureAddress, 'Metal texture', _owner);
  ScopedNativePointer get unsafeDevice =>
      _scopedPointer(_deviceAddress, 'Metal device', _owner);
}

/// Scoped Vulkan metadata from an [AcquiredFrame].
final class VulkanOwnedTextureFrame {
  const VulkanOwnedTextureFrame._(
    this.generation,
    this.width,
    this.height,
    this.scaleFactor,
    this.frameId,
    this.format,
    this.layout,
    this._imageAddress,
    this._imageViewAddress,
    this._deviceAddress,
    this._owner,
  );

  factory VulkanOwnedTextureFrame._fromNative(
    raw.mln_vulkan_owned_texture_frame value,
    AcquiredFrame owner,
  ) => VulkanOwnedTextureFrame._(
    value.generation,
    value.width,
    value.height,
    value.scale_factor,
    value.frame_id,
    value.format,
    value.layout,
    value.image.address,
    value.image_view.address,
    value.device.address,
    owner,
  );

  final int generation;
  final int width;
  final int height;
  final double scaleFactor;
  final int frameId;
  final int format;
  final int layout;
  final int _imageAddress;
  final int _imageViewAddress;
  final int _deviceAddress;
  final AcquiredFrame _owner;
  ScopedNativePointer get unsafeImage =>
      _scopedPointer(_imageAddress, 'Vulkan image', _owner);
  ScopedNativePointer get unsafeImageView =>
      _scopedPointer(_imageViewAddress, 'Vulkan image view', _owner);
  ScopedNativePointer get unsafeDevice =>
      _scopedPointer(_deviceAddress, 'Vulkan device', _owner);
}

/// Scoped WebGPU metadata from an [AcquiredFrame].
final class WebGPUOwnedTextureFrame {
  const WebGPUOwnedTextureFrame._(
    this.generation,
    this.width,
    this.height,
    this.scaleFactor,
    this.frameId,
    this.format,
    this._textureAddress,
    this._textureViewAddress,
    this._deviceAddress,
    this._owner,
  );

  factory WebGPUOwnedTextureFrame._fromNative(
    raw.mln_webgpu_owned_texture_frame value,
    AcquiredFrame owner,
  ) => WebGPUOwnedTextureFrame._(
    value.generation,
    value.width,
    value.height,
    value.scale_factor,
    value.frame_id,
    value.format,
    value.texture.address,
    value.texture_view.address,
    value.device.address,
    owner,
  );

  final int generation;
  final int width;
  final int height;
  final double scaleFactor;
  final int frameId;
  final int format;
  final int _textureAddress;
  final int _textureViewAddress;
  final int _deviceAddress;
  final AcquiredFrame _owner;
  ScopedNativePointer get unsafeTexture =>
      _scopedPointer(_textureAddress, 'WebGPU texture', _owner);
  ScopedNativePointer get unsafeTextureView =>
      _scopedPointer(_textureViewAddress, 'WebGPU texture view', _owner);
  ScopedNativePointer get unsafeDevice =>
      _scopedPointer(_deviceAddress, 'WebGPU device', _owner);
}

/// Scoped OpenGL metadata from an [AcquiredFrame].
final class OpenGLOwnedTextureFrame {
  const OpenGLOwnedTextureFrame._(
    this.generation,
    this.width,
    this.height,
    this.scaleFactor,
    this.frameId,
    this.texture,
    this.target,
    this.internalFormat,
    this.format,
    this.type,
    this._owner,
  );

  factory OpenGLOwnedTextureFrame._fromNative(
    raw.mln_opengl_owned_texture_frame value,
    AcquiredFrame owner,
  ) => OpenGLOwnedTextureFrame._(
    value.generation,
    value.width,
    value.height,
    value.scale_factor,
    value.frame_id,
    value.texture,
    value.target,
    value.internal_format,
    value.format,
    value.type,
    owner,
  );

  final int generation;
  final int width;
  final int height;
  final double scaleFactor;
  final int frameId;
  final int texture;
  final int target;
  final int internalFormat;
  final int format;
  final int type;
  final AcquiredFrame _owner;

  void ensureValid() => _owner._checkOpen();
}

ScopedNativePointer _scopedPointer(
  int address,
  String name,
  AcquiredFrame owner,
) =>
    ScopedNativePointer(address, checkValid: owner._checkOpen, debugName: name);

/// Render-session attachment operations on an any-thread map handle.
extension MapRenderAttachments on MapHandle {
  RenderSessionAttachment attachMetalSurface(
    MetalSurfaceDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_metal_surface_descriptor>()
      ..ref = _metalSurfaceDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_metal_surface_attach_start(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  RenderSessionAttachment attachVulkanSurface(
    VulkanSurfaceDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_vulkan_surface_descriptor>()
      ..ref = _vulkanSurfaceDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_vulkan_surface_attach_start(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  RenderSessionAttachment attachOpenGLSurface(
    OpenGLSurfaceDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(
      driver: RenderDriver.callerGraphicsThread,
    ),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_opengl_surface_descriptor>()
      ..ref = _openglSurfaceDescriptorToNative(descriptor, arena);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_opengl_surface_attach_start(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  RenderSessionAttachment attachMetalOwnedTexture(
    MetalOwnedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_metal_owned_texture_descriptor>()
      ..ref = _metalOwnedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_metal_owned_texture_attach_start(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  RenderSessionAttachment attachMetalBorrowedTexture(
    MetalBorrowedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_metal_borrowed_texture_descriptor>()
      ..ref = _metalBorrowedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) =>
          raw.mln_metal_borrowed_texture_attach_start(
            _handle.raw,
            native,
            policy,
            session,
            operation,
          ),
    );
  });

  RenderSessionAttachment attachVulkanOwnedTexture(
    VulkanOwnedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_vulkan_owned_texture_descriptor>()
      ..ref = _vulkanOwnedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_vulkan_owned_texture_attach_start(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  RenderSessionAttachment attachVulkanBorrowedTexture(
    VulkanBorrowedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_vulkan_borrowed_texture_descriptor>()
      ..ref = _vulkanBorrowedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) =>
          raw.mln_vulkan_borrowed_texture_attach_start(
            _handle.raw,
            native,
            policy,
            session,
            operation,
          ),
    );
  });

  RenderSessionAttachment attachOpenGLOwnedTexture(
    OpenGLOwnedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(
      driver: RenderDriver.callerGraphicsThread,
    ),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_opengl_owned_texture_descriptor>()
      ..ref = _openglOwnedTextureDescriptorToNative(descriptor, arena);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_opengl_owned_texture_attach_start(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  RenderSessionAttachment attachOpenGLBorrowedTexture(
    OpenGLBorrowedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(
      driver: RenderDriver.callerGraphicsThread,
    ),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_opengl_borrowed_texture_descriptor>()
      ..ref = _openglBorrowedTextureDescriptorToNative(descriptor, arena);
    return _startRenderAttachment(
      options,
      (policy, session, operation) =>
          raw.mln_opengl_borrowed_texture_attach_start(
            _handle.raw,
            native,
            policy,
            session,
            operation,
          ),
    );
  });

  RenderSessionAttachment attachWebGPUSurface(
    WebGPUSurfaceDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(
      driver: RenderDriver.callerGraphicsThread,
    ),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_webgpu_surface_descriptor>()
      ..ref = _webGPUSurfaceDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_webgpu_surface_attach_start(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  RenderSessionAttachment attachWebGPUOwnedTexture(
    WebGPUOwnedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(
      driver: RenderDriver.callerGraphicsThread,
    ),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_webgpu_owned_texture_descriptor>()
      ..ref = _webGPUOwnedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_webgpu_owned_texture_attach_start(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  RenderSessionAttachment attachWebGPUBorrowedTexture(
    WebGPUBorrowedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(
      driver: RenderDriver.callerGraphicsThread,
    ),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_webgpu_borrowed_texture_descriptor>()
      ..ref = _webGPUBorrowedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) =>
          raw.mln_webgpu_borrowed_texture_attach_start(
            _handle.raw,
            native,
            policy,
            session,
            operation,
          ),
    );
  });

  RenderSessionAttachment _startRenderAttachment(
    RenderSessionAttachOptions options,
    int Function(
      Pointer<raw.mln_render_session_attach_options>,
      Pointer<Uint64>,
      Pointer<Uint64>,
    )
    start,
  ) {
    ensureAbiVersion();
    return withNativeArena((arena) {
      final policy = arena<raw.mln_render_session_attach_options>()
        ..ref = raw.mln_render_session_attach_options_default();
      policy.ref.driver = options.driver.rawValue;
      policy.ref.requested_texture_ring_depth =
          options.requestedTextureRingDepth;
      final session = arena<Uint64>()..value = 0;
      final operation = arena<Uint64>()..value = 0;
      _check(start(policy, session, operation));
      final renderSession = RenderSessionHandle._(
        _runtime,
        NativeRenderSession(session.value),
      );
      return RenderSessionAttachment(
        renderSession,
        renderSession._completeWhileRetained(
          _runtime._finishOperation(operation.value),
        ),
      );
    });
  }
}
