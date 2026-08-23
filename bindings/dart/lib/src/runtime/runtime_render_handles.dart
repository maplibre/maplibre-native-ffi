part of 'runtime.dart';

/// Owned standalone projection snapshot created from a map.
///
/// Every method is synchronous, runs on the calling isolate's thread, is
/// internally serialized, and may be called from any isolate. A projection
/// copies the map's transform state at creation and never observes map
/// changes made after that and remains usable after its source map and runtime
/// close.
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

  /// Closes the projection after waiting for projection calls already running
  /// on other threads.
  void close() {
    _state.close(
      (handle) => raw.mln_map_projection_close(handle.raw),
      _c.threadLastErrorMessage,
    );
  }
}

/// Outcome of one accepted frame demand.
///
/// Each outcome has its own retry condition, so a host that paces its own
/// frame loop keeps them apart.
final class RenderResult {
  const RenderResult._(this.rawValue, this.name);

  /// A frame was rendered for acquisition, presentation, or ordered readback.
  static const rendered = RenderResult._(0, 'rendered');

  /// No newer map update was available.
  static const noUpdate = RenderResult._(1, 'noUpdate');

  /// An ordered extent change had not reached the driver.
  static const sizePending = RenderResult._(2, 'sizePending');

  /// The target could not produce a frame.
  static const targetNotReady = RenderResult._(3, 'targetNotReady');

  /// A newer demand in the same coalescing boundary replaced this demand.
  static const superseded = RenderResult._(4, 'superseded');

  /// The demand's timeout elapsed before driver work began.
  static const deadlineMissed = RenderResult._(5, 'deadlineMissed');

  /// Returns the result for a native value, or an unknown result for a value
  /// that this binding does not name.
  factory RenderResult.fromRawValue(int value) => switch (value) {
    0 => rendered,
    1 => noUpdate,
    2 => sizePending,
    3 => targetNotReady,
    4 => superseded,
    5 => deadlineMissed,
    _ => RenderResult._(value, 'unknown($value)'),
  };

  /// The value that the C API reported.
  final int rawValue;

  /// Diagnostic name of this outcome.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is RenderResult && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Render execution placement selected during attachment.
///
/// A target that requires one placement rejects the other during attachment.
enum RenderDriver {
  /// Native code owns a serial worker that initializes, drives, and tears down
  /// transferable graphics state.
  coreWorker(1),

  /// The host calls [RenderSessionHandle.serviceDriverWork] from the thread
  /// where the target's graphics context is usable, and native code stores
  /// work until it does.
  callerGraphicsThread(2);

  const RenderDriver(this.rawValue);

  /// The value that the C API uses for this placement.
  final int rawValue;
}

/// Common policy for a render-session attachment.
final class RenderSessionAttachOptions {
  /// Creates an attachment policy.
  const RenderSessionAttachOptions({
    this.driver = RenderDriver.coreWorker,
    this.requestedTextureRingDepth = 1,
  });

  /// Execution placement for the attached session.
  final RenderDriver driver;

  /// Requested count of host-acquirable slots in a session-owned texture ring,
  /// from one to three.
  ///
  /// A private texture target grants one slot whatever this value is, and a
  /// target without a ring ignores it. The attached session reports the
  /// granted count as [RenderSessionCapabilities.textureRingDepth].
  final int requestedTextureRingDepth;
}

/// One nonblocking request for a rendered frame.
///
/// Matching demands may coalesce before work starts, and every accepted demand
/// still produces one terminal [RenderFrameResult].
final class FrameDemand {
  /// Creates a frame demand.
  const FrameDemand({
    this.renderIfNeeded = true,
    this.present = false,
    this.token = 0,
    this.coalescingBoundary = 0,
    this.timeoutNanoseconds = 0,
  });

  /// Whether to render only when a newer map update exists.
  ///
  /// A demand that finds nothing newer terminates as [RenderResult.noUpdate].
  final bool renderIfNeeded;

  /// Whether the target presents the rendered frame.
  ///
  /// A target presents only when
  /// [RenderSessionCapabilities.supportsPresentation] is set.
  final bool present;

  /// Host identity returned with the terminal frame result.
  final int token;

  /// Boundary that limits coalescing: two demands coalesce only when this
  /// value and their flags match.
  final int coalescingBoundary;

  /// Time allowed before driver work begins, in nanoseconds; zero has no
  /// limit.
  ///
  /// The timeout starts when native code accepts the demand, and a demand
  /// whose timeout elapses first terminates as [RenderResult.deadlineMissed].
  /// It bounds latency rather than setting a cadence.
  final int timeoutNanoseconds;
}

/// Immutable terminal result for one accepted frame demand.
final class RenderFrameResult {
  const RenderFrameResult._({
    required this.disposition,
    required this.token,
    required this.mapUpdateGeneration,
    required this.extentGeneration,
    required this.frameGeneration,
    required this.needsRepaint,
  });

  factory RenderFrameResult._fromNative(raw.mln_render_frame_result value) =>
      RenderFrameResult._(
        disposition: RenderResult.fromRawValue(value.disposition),
        token: value.token,
        mapUpdateGeneration: value.map_update_generation,
        extentGeneration: value.extent_generation,
        frameGeneration: value.frame_generation,
        needsRepaint: value.needs_repaint,
      );

  /// Terminal outcome of the demand.
  final RenderResult disposition;

  /// The [FrameDemand.token] of the demand that produced this result.
  final int token;

  /// Map-update generation that the driver used.
  final int mapUpdateGeneration;

  /// Extent generation that the driver used.
  final int extentGeneration;

  /// Frame generation of the rendered frame, and zero unless [disposition] is
  /// [RenderResult.rendered].
  final int frameGeneration;

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

  /// Execution placement that this session uses.
  final RenderDriver driver;

  /// Granted host-acquirable slot count, and zero for a target without a
  /// texture ring.
  final int textureRingDepth;

  /// Capability bits as the C API reports them, decoded by the getters below.
  final int flags;

  /// Whether [RenderSessionHandle.acquireFrame] can lease rendered texture
  /// frames from this session.
  bool get supportsFrameAcquisition => flags & 1 != 0;

  /// Whether [RenderSessionHandle.readPremultipliedRgba8] can copy the latest
  /// rendered frame to the CPU.
  bool get supportsReadback => flags & 2 != 0;

  /// Whether [AcquiredFrame.release] accepts backend synchronization rather
  /// than CPU-complete synchronization alone.
  bool get supportsConsumerSync => flags & 4 != 0;

  /// Whether the target presents rendered frames, which [FrameDemand.present]
  /// requests.
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

  /// Lifecycle state: 1 attaching, 2 attached, 3 detaching, 4 detached,
  /// 5 target lost, and 6 abandoned.
  final int state;

  /// Execution placement that this session uses.
  final RenderDriver driver;

  /// Most recent terminal frame outcome.
  final RenderResult latestResult;

  /// Logical extent that the session currently renders at.
  final RenderTargetExtent extent;

  /// Session generation, which increases whenever the session changes state,
  /// target, or extent.
  final int generation;

  /// Latest map-update generation published to this session.
  final int mapUpdateGeneration;

  /// Map-update generation of the most recently rendered frame.
  final int renderedUpdateGeneration;

  /// Extent generation, which increases with each applied resize.
  final int extentGeneration;

  /// Frame generation, which increases with each rendered frame.
  final int frameGeneration;

  /// Token of the most recently accepted frame demand.
  final int latestDemandToken;

  /// Count of accepted demands that have no terminal result yet.
  final int pendingDemandCount;

  /// Count of texture-ring slots that the host currently holds.
  final int acquiredFrameCount;

  /// Whether the target can produce a frame.
  final bool targetReady;

  /// Whether the session holds changes that a render-if-needed demand would
  /// render, such as a newer map update or a pending resize.
  final bool pendingChanges;
}

/// Result of irreversible target abandonment.
final class RenderAbandonResult {
  const RenderAbandonResult._(this.quarantined, this.quarantinedResourceCount);

  /// Whether graphics resources could not be destroyed and were quarantined.
  final bool quarantined;

  /// Count of backend resource groups retained until the process exits.
  final int quarantinedResourceCount;
}

/// Synchronization that protects an acquired texture frame.
final class GpuSync {
  /// Creates synchronization for work that completed before the call that
  /// carries it.
  const GpuSync.cpuComplete() : kind = 0, object = null, value = 0;

  /// Creates synchronization from a backend object and its signal value.
  ///
  /// The backend object stays caller-owned. Metal is the exception: the
  /// session retains a shared event. A Vulkan, OpenGL, or WebGPU object stays
  /// borrowed until a later [RenderSessionHandle.barrier] or
  /// [RenderSessionHandle.detach] completes.
  const GpuSync.native({
    required this.kind,
    required this.object,
    required this.value,
  });

  /// Payload kind: 0 CPU-complete, 1 Metal shared event, 2 Vulkan timeline
  /// semaphore, 3 OpenGL fence, and 4 WebGPU token.
  final int kind;

  /// Backend synchronization object, and null for CPU-complete
  /// synchronization.
  final NativePointer? object;

  /// Signal value for a timeline object, such as a Metal shared event or a
  /// Vulkan timeline semaphore.
  final int value;
}

/// An attachment that exposes the session while graphics initialization runs.
final class RenderSessionAttachment {
  /// Creates an attachment from a session and its attachment completion.
  const RenderSessionAttachment(this.session, this.completed);

  /// The session, which is usable for driver work and for state reads while
  /// attachment runs.
  final RenderSessionHandle session;

  /// Completes after the selected driver initializes the target.
  ///
  /// A caller-graphics-thread session completes attachment only after the host
  /// services driver work. A failed attachment still requires
  /// [RenderSessionHandle.detach] or [RenderSessionHandle.abandon] before
  /// [RenderSessionHandle.close].
  final Future<void> completed;
}

/// A render session attached to one map and render target.
///
/// A map has at most one live session, and its style, sources, layers, and
/// camera remain after the session detaches. The session advances through its
/// selected [RenderDriver] rather than through a runtime pump.
///
/// The target setters below start ordered replacements of the session's
/// surface or caller-owned texture, as after a window recreation or a new host
/// allocation. A replacement keeps the session's rendering resources,
/// including loaded tiles, unless the scale factor changes, and map-owned
/// feature state survives either way. It changes the graphics resource only,
/// so the map viewport keeps following map creation and map resize. A setter
/// for a target kind that the session does not use reports an unsupported
/// status, and replacing a texture target while a frame is acquired reports an
/// invalid-state status.
final class RenderSessionHandle implements Finalizable {
  RenderSessionHandle._(this._runtime, NativeRenderSession handle)
    : _state = NativeHandleState(handle, 'RenderSessionHandle');

  final RuntimeHandle _runtime;
  final NativeHandleState<NativeRenderSession> _state;
  final _frameResultsReady = StreamController<void>.broadcast(sync: true);
  final _driverWorkReady = StreamController<void>.broadcast(sync: true);

  /// Notifications that frame results are available to drain.
  ///
  /// Readiness is level-triggered and notifications may coalesce, so call
  /// [drainFrameResults] until it returns an empty batch. Events arrive on the
  /// isolate that attached the session.
  Stream<void> get frameResultsReady => _frameResultsReady.stream;

  /// Notifications that caller-graphics-thread work is ready to service.
  ///
  /// Notifications may coalesce, so call [serviceDriverWork] until the mailbox
  /// drains. Events arrive on the isolate that attached the session.
  Stream<void> get driverWorkReady => _driverWorkReady.stream;

  void _notifyFramesReady() => _frameResultsReady.add(null);
  void _notifyDriverWorkReady() => _driverWorkReady.add(null);

  /// Whether this session has been closed by the Dart binding.
  bool get isClosed => _state.isClosed;

  Future<void> _completeWhileRetained(Future<void> completion) async {
    final retained = this;
    try {
      await completion;
    } finally {
      retained.isClosed;
    }
  }

  NativeRenderSession get _handle => _state.handle;

  /// Copies the capabilities that attachment fixed for this session.
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

  /// Copies the latest session state and generations.
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

  /// Requests one frame and returns without waiting.
  ///
  /// A core-worker session wakes its graphics worker, and a
  /// caller-graphics-thread session publishes driver work and notifies
  /// [driverWorkReady]. The demand captures the current extent generation, and
  /// its terminal result arrives through [drainFrameResults].
  void requestFrame([FrameDemand demand = const FrameDemand()]) {
    withNativeArena((arena) {
      final native = arena<raw.mln_frame_demand>()
        ..ref = raw.mln_frame_demand_default();
      native.ref.flags =
          (demand.renderIfNeeded ? 1 : 0) | (demand.present ? 2 : 0);
      native.ref.token = demand.token;
      native.ref.coalescing_boundary = demand.coalescingBoundary;
      native.ref.timeout_ns = demand.timeoutNanoseconds;
      _check(raw.mln_render_session_request_frame(_handle.raw, native));
    });
  }

  /// Drains every queued terminal frame result as one owned batch.
  ///
  /// The C API hands over a batch whose records stay stable until release, and
  /// this method copies the records into Dart values and releases the batch
  /// before returning. Draining again after an empty result is valid.
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

  /// Services up to [maxWork] driver work items and returns the count
  /// serviced; zero services every item currently queued.
  ///
  /// Call this on the thread where the target's graphics context is current.
  /// The first successful call fixes the session's graphics-thread identity,
  /// and a later call from another thread reports a wrong-thread status.
  /// Attach, resize, target replacement, queries, readback, maintenance,
  /// barriers, and detach all use this mailbox, so keep servicing it while
  /// presentation is paused. A core-worker session reports an invalid-state status, because
  /// its own worker owns execution.
  ///
  /// [maxWork] must be zero or positive.
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

  /// Starts an ordered resize and completes after the driver applies the
  /// extent and updates the map viewport.
  ///
  /// A surface or session-owned texture target resizes in place and keeps the
  /// renderer, which carries the tile pyramid, the glyph and image atlases,
  /// and symbol placement. A changed scale factor retires the renderer
  /// instead, because its shaders are compiled for one pixel ratio.
  /// Map-owned feature state survives either way and reaches the replacement
  /// renderer on the next render update.
  ///
  /// A caller-owned texture is sized by its owner, so resizing one reports an
  /// unsupported status: allocate a texture at the new size and hand it over
  /// with the backend's borrowed-texture target setter. A resize applied while
  /// a texture frame is acquired fails with an invalid-state status.
  Future<void> resize(RenderTargetExtent extent) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_render_target_extent>()
        ..ref = _renderTargetExtentToNative(extent);
      return raw.mln_render_session_resize(_handle.raw, native, out);
    }),
  );

  /// Starts an ordered Metal surface replacement.
  Future<void> setMetalSurfaceTarget(MetalSurfaceDescriptor descriptor) =>
      _voidOperation(
        (out) => withNativeArena((arena) {
          final native = arena<raw.mln_metal_surface_descriptor>()
            ..ref = _metalSurfaceDescriptorToNative(descriptor);
          return raw.mln_metal_surface_set_target(_handle.raw, native, out);
        }),
      );

  /// Starts an ordered Vulkan surface replacement.
  Future<void> setVulkanSurfaceTarget(VulkanSurfaceDescriptor descriptor) =>
      _voidOperation(
        (out) => withNativeArena((arena) {
          final native = arena<raw.mln_vulkan_surface_descriptor>()
            ..ref = _vulkanSurfaceDescriptorToNative(descriptor);
          return raw.mln_vulkan_surface_set_target(_handle.raw, native, out);
        }),
      );

  /// Starts an ordered OpenGL surface replacement.
  Future<void> setOpenGLSurfaceTarget(OpenGLSurfaceDescriptor descriptor) =>
      _voidOperation(
        (out) => withNativeArena((arena) {
          final native = arena<raw.mln_opengl_surface_descriptor>()
            ..ref = _openglSurfaceDescriptorToNative(descriptor, arena);
          return raw.mln_opengl_surface_set_target(_handle.raw, native, out);
        }),
      );

  /// Starts an ordered WebGPU surface replacement.
  Future<void> setWebGPUSurfaceTarget(WebGPUSurfaceDescriptor descriptor) =>
      _voidOperation(
        (out) => withNativeArena((arena) {
          final native = arena<raw.mln_webgpu_surface_descriptor>()
            ..ref = _webGPUSurfaceDescriptorToNative(descriptor);
          return raw.mln_webgpu_surface_set_target(_handle.raw, native, out);
        }),
      );

  /// Starts an ordered caller-owned Metal texture replacement.
  Future<void> setMetalBorrowedTextureTarget(
    MetalBorrowedTextureDescriptor descriptor,
  ) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_metal_borrowed_texture_descriptor>()
        ..ref = _metalBorrowedTextureDescriptorToNative(descriptor);
      return raw.mln_metal_borrowed_texture_set_target(
        _handle.raw,
        native,
        out,
      );
    }),
  );

  /// Starts an ordered caller-owned Vulkan texture replacement.
  Future<void> setVulkanBorrowedTextureTarget(
    VulkanBorrowedTextureDescriptor descriptor,
  ) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_vulkan_borrowed_texture_descriptor>()
        ..ref = _vulkanBorrowedTextureDescriptorToNative(descriptor);
      return raw.mln_vulkan_borrowed_texture_set_target(
        _handle.raw,
        native,
        out,
      );
    }),
  );

  /// Starts an ordered caller-owned OpenGL texture replacement.
  Future<void> setOpenGLBorrowedTextureTarget(
    OpenGLBorrowedTextureDescriptor descriptor,
  ) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_opengl_borrowed_texture_descriptor>()
        ..ref = _openglBorrowedTextureDescriptorToNative(descriptor, arena);
      return raw.mln_opengl_borrowed_texture_set_target(
        _handle.raw,
        native,
        out,
      );
    }),
  );

  /// Starts an ordered caller-owned WebGPU texture replacement.
  Future<void> setWebGPUBorrowedTextureTarget(
    WebGPUBorrowedTextureDescriptor descriptor,
  ) => _voidOperation(
    (out) => withNativeArena((arena) {
      final native = arena<raw.mln_webgpu_borrowed_texture_descriptor>()
        ..ref = _webGPUBorrowedTextureDescriptorToNative(descriptor);
      return raw.mln_webgpu_borrowed_texture_set_target(
        _handle.raw,
        native,
        out,
      );
    }),
  );

  /// Starts a barrier and completes after every render-session operation
  /// accepted before it reaches a terminal result.
  ///
  /// A barrier requests no frame of its own, so it observes the work that is
  /// already accepted rather than driving new work.
  Future<void> barrier() =>
      _voidOperation((out) => raw.mln_render_session_barrier(_handle.raw, out));

  /// Starts a best-effort release of renderer caches and completes after the
  /// driver runs it.
  Future<void> reduceMemoryUse() => _voidOperation(
    (out) => raw.mln_render_session_reduce_memory_use(_handle.raw, out),
  );

  /// Starts renderer-data clearing and completes after the driver runs it.
  Future<void> clearData() => _voidOperation(
    (out) => raw.mln_render_session_clear_data(_handle.raw, out),
  );

  /// Starts renderer diagnostic-log emission and completes after the driver
  /// runs it.
  Future<void> dumpDebugLogs() => _voidOperation(
    (out) => raw.mln_render_session_dump_debug_logs(_handle.raw, out),
  );

  /// Starts a rendered-feature query against the session's latest driver state
  /// and completes with one copied hit per match.
  ///
  /// Every input is copied before this returns. A core-worker session runs the
  /// query on its worker, and a caller-graphics-thread session publishes driver
  /// work and completes the query only after the host services it.
  ///
  /// A [RenderedQueryBox] is normalized and clipped to the viewport, so a box
  /// larger than the viewport queries everything visible and a box outside the
  /// viewport matches nothing. A [RenderedQueryPoint] and a
  /// [RenderedQueryLineString] are queried as given.
  Future<List<QueriedFeature>> queryRenderedFeatures(
    RenderedQueryGeometry geometry, {
    RenderedFeatureQueryOptions? options,
  }) => _queryFeatures(
    (completion) => withNativeArena((arena) {
      final nativeGeometry = arena<raw.mln_rendered_query_geometry>()
        ..ref = _renderedQueryGeometryToNative(geometry, arena);
      final nativeOptions = _renderedFeatureQueryOptionsToNative(
        options ?? RenderedFeatureQueryOptions(),
        arena,
      );
      return raw.mln_render_session_query_rendered_features(
        _handle.raw,
        nativeGeometry,
        nativeOptions,
        completion,
      );
    }),
  );

  /// Starts a source-feature query for the source with [sourceId] and
  /// completes with one copied hit per match.
  ///
  /// The query runs against the session's latest driver state, like
  /// [queryRenderedFeatures], and covers the features that the source holds
  /// rather than the features that the style renders.
  Future<List<QueriedFeature>> querySourceFeatures(
    String sourceId, {
    SourceFeatureQueryOptions? options,
  }) => _queryFeatures(
    (completion) => withNativeArena((arena) {
      final nativeOptions = _sourceFeatureQueryOptionsToNative(
        options ?? SourceFeatureQueryOptions(),
        arena,
      );
      return raw.mln_render_session_query_source_features(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        nativeOptions,
        completion,
      );
    }),
  );

  /// Starts a feature-extension query against the session's latest driver
  /// state and completes with copied JSON bytes.
  ///
  /// [feature] is one UTF-8 GeoJSON Feature, and [extension] and
  /// [extensionField] name the extension to evaluate for it. [arguments] is
  /// optional UTF-8 JSON, and must be a JSON object when it is present.
  Future<Uint8List> queryFeatureExtensions({
    required String sourceId,
    required Uint8List feature,
    required String extension,
    required String extensionField,
    Uint8List? arguments,
  }) => _bufferOperation(
    (completion) => withNativeArena((arena) {
      final nativeArguments = arguments == null
          ? nullptr.cast<raw.mln_buffer_view>()
          : (arena<raw.mln_buffer_view>()
              ..ref = nativeBufferView(arguments, arena));
      return raw.mln_render_session_query_feature_extensions(
        _handle.raw,
        nativeStringView(sourceId, arena).value,
        nativeBufferView(feature, arena),
        nativeStringView(extension, arena).value,
        nativeStringView(extensionField, arena).value,
        nativeArguments,
        completion,
      );
    }),
  );

  Future<Uint8List> _bufferOperation(NativeCompletionStart start) =>
      _runtime._startValue(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_BUFFER_VIEWS,
        elementSize: sizeOf<raw.mln_buffer_view>(),
        start: start,
        decode: (result) =>
            _copyBufferView(result.value.cast<raw.mln_buffer_view>().ref),
      );

  Future<List<QueriedFeature>> _queryFeatures(NativeCompletionStart start) =>
      _runtime._startValue(
        copyKind: raw
            .mln_adapter_completion_copy_kind
            .MLN_ADAPTER_COMPLETION_COPY_QUERIED_FEATURES,
        elementSize: sizeOf<raw.mln_queried_feature>(),
        start: start,
        decode: (result) => [
          for (var index = 0; index < result.value_count; index += 1)
            _queriedFeatureFromNative(
              result.value.cast<raw.mln_queried_feature>()[index],
            ),
        ],
      );

  /// Starts readback of the latest rendered texture frame and completes with
  /// Dart-owned premultiplied RGBA8 bytes.
  ///
  /// The C API borrows the pixels for its completion only, and this method
  /// copies them before the returned future completes. A session whose
  /// [RenderSessionCapabilities.supportsReadback] is false reports an
  /// unsupported status.
  ///
  /// Readback returns the frame that the driver already rendered for the
  /// session's current generation, so request a frame and let it terminate
  /// after each resize or target replacement. Reading back before that frame
  /// exists reports an invalid-state status.
  Future<TextureImage> readPremultipliedRgba8() => _runtime._startValue(
    copyKind: raw
        .mln_adapter_completion_copy_kind
        .MLN_ADAPTER_COMPLETION_COPY_TEXTURE_READBACK,
    elementSize: sizeOf<raw.mln_texture_readback_result>(),
    start: (completion) =>
        raw.mln_texture_read_premultiplied_rgba8(_handle.raw, completion),
    decode: (result) {
      final value = result.value.cast<raw.mln_texture_readback_result>().ref;
      return TextureImage(
        info: TextureImageInfo._fromNative(value.info),
        bytes: _copyBufferView(value.data),
      );
    },
  );

  /// Leases the oldest rendered texture-ring slot that no lease already holds.
  ///
  /// The call is nonblocking, and the lease owns its slot until
  /// [AcquiredFrame.release]. A session whose
  /// [RenderSessionCapabilities.supportsFrameAcquisition] is false reports an
  /// unsupported status, and a session with no rendered slot ready reports a
  /// not-ready status, so pace acquisition with [drainFrameResults] or
  /// [RenderSessionSnapshot.frameGeneration].
  AcquiredFrame acquireFrame() => withNativeArena((arena) {
    final out = arena<Uint64>()..value = 0;
    _check(raw.mln_render_session_acquire_frame(_handle.raw, out));
    return AcquiredFrame._(out.value);
  });

  /// Starts normal graphics teardown and map detachment, and completes after
  /// the driver destroys the session's graphics resources.
  ///
  /// Every mailbox operation accepted earlier reaches a terminal result before
  /// teardown runs, so the host's target, device, and borrowed synchronization
  /// objects stay in use until the returned future completes. A session that
  /// still holds an acquired frame reports an invalid-state status and stays
  /// attached, so release every [AcquiredFrame] first.
  Future<void> detach() =>
      _voidOperation((out) => raw.mln_render_session_detach(_handle.raw, out));

  /// Irreversibly closes this session's control and mailboxes without calling
  /// the graphics driver.
  ///
  /// The call waits for the map's in-flight tile work, which can still reach
  /// the host's graphics objects through quarantined renderer resources, so the
  /// host may destroy its target and device as soon as this returns. Do not
  /// call it from a MapLibre callback. Resources that could not be destroyed
  /// are reported as quarantined.
  RenderAbandonResult abandon() => withNativeArena((arena) {
    final out = arena<raw.mln_render_abandon_result>()
      ..ref.size = sizeOf<raw.mln_render_abandon_result>();
    _check(raw.mln_render_session_abandon(_handle.raw, out));
    return RenderAbandonResult._(
      out.ref.disposition == 1,
      out.ref.quarantined_resource_count,
    );
  });

  Future<void> _voidOperation(NativeCompletionStart start) =>
      _runtime._startUnit(start);

  /// Retires the native session handle and closes [frameResultsReady] and
  /// [driverWorkReady].
  ///
  /// Close a session after [detach] completes or after [abandon] returns. The
  /// call is CPU-only and may run on any isolate, and it waits for a
  /// core-worker session's worker to stop. A session that is still attached
  /// reports an invalid-state status, as does a detached session that still
  /// holds an acquired frame.
  void close() {
    _state.close(
      (handle) => raw.mln_render_session_destroy(handle.raw),
      _c.threadLastErrorMessage,
    );
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
  /// Creates a texture image, copying [bytes] into an unmodifiable view.
  TextureImage({required this.info, required Uint8List bytes})
    : bytes = Uint8List.fromList(bytes).asUnmodifiableView();

  /// Image metadata.
  final TextureImageInfo info;

  /// Copied premultiplied RGBA8 bytes.
  final Uint8List bytes;
}

/// Scoped lease on one rendered texture-ring slot.
final class AcquiredFrame {
  AcquiredFrame._(this._handle);

  int _handle;

  void _checkOpen() {
    if (_handle == 0) {
      throwInvalidState('acquired frame has already been released');
    }
  }

  /// Copies the frame result of the demand that produced this frame.
  RenderFrameResult get result => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_render_frame_result>()
      ..ref.size = sizeOf<raw.mln_render_frame_result>();
    _check(raw.mln_acquired_frame_get_result(_handle, out));
    return RenderFrameResult._fromNative(out.ref);
  });

  /// Copies the synchronization that the session signals when the rendered
  /// contents of this frame are ready for the consumer.
  ///
  /// Wait on it before the consumer reads the frame's texture.
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

  /// Copies WebGPU-native metadata for this frame.
  ///
  /// A frame from another render backend reports an unsupported status.
  WebGPUOwnedTextureFrame get webGPUTexture => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_webgpu_owned_texture_frame>()
      ..ref.size = sizeOf<raw.mln_webgpu_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_webgpu_texture(_handle, out));
    return WebGPUOwnedTextureFrame._fromNative(out.ref, this);
  });

  /// Copies Metal-native metadata for this frame.
  ///
  /// A frame from another render backend reports an unsupported status.
  MetalOwnedTextureFrame get metalTexture => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_metal_owned_texture_frame>()
      ..ref.size = sizeOf<raw.mln_metal_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_metal_texture(_handle, out));
    return MetalOwnedTextureFrame._fromNative(out.ref, this);
  });

  /// Copies Vulkan-native metadata for this frame.
  ///
  /// A frame from another render backend reports an unsupported status.
  VulkanOwnedTextureFrame get vulkanTexture => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_vulkan_owned_texture_frame>()
      ..ref.size = sizeOf<raw.mln_vulkan_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_vulkan_texture(_handle, out));
    return VulkanOwnedTextureFrame._fromNative(out.ref, this);
  });

  /// Copies OpenGL-native metadata for this frame.
  ///
  /// Read it on the thread where the session's OpenGL context is current. A
  /// frame from another render backend reports an unsupported status.
  OpenGLOwnedTextureFrame get openGLTexture => withNativeArena((arena) {
    _checkOpen();
    final out = arena<raw.mln_opengl_owned_texture_frame>()
      ..ref.size = sizeOf<raw.mln_opengl_owned_texture_frame>();
    _check(raw.mln_acquired_frame_get_opengl_texture(_handle, out));
    return OpenGLOwnedTextureFrame._fromNative(out.ref, this);
  });

  /// Returns the leased slot to the session after consumer work on [sync].
  ///
  /// The session retires the slot through its driver before it renders into
  /// the slot again, and every pointer that this lease exposed becomes invalid
  /// here. Release is one-shot, and a second call throws an invalid-state
  /// error. A backend synchronization kind that the session does not accept
  /// reports an unsupported status and keeps the lease, so the host may retry
  /// with [GpuSync.cpuComplete].
  void release({GpuSync sync = const GpuSync.cpuComplete()}) {
    withNativeArena((arena) {
      _checkOpen();
      final frame = arena<Uint64>()..value = _handle;
      final nativeSync = arena<raw.mln_gpu_sync>()
        ..ref = raw.mln_gpu_sync_default()
        ..ref.kind = sync.kind
        ..ref.object = Pointer<Void>.fromAddress(sync.object?.address ?? 0)
        ..ref.value = sync.value;
      _check(raw.mln_acquired_frame_release(frame, nativeSync));
      _handle = 0;
    });
  }
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

  /// Session generation that produced this frame.
  final int generation;

  /// Physical Metal texture width in device pixels.
  final int width;

  /// Physical Metal texture height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque frame identity that the session uses to reject a stale release.
  final int frameId;

  /// Backend-native `MTLPixelFormat` value.
  final int pixelFormat;

  final int _textureAddress;
  final int _deviceAddress;
  final AcquiredFrame _owner;

  /// Borrowed `id<MTLTexture>`, valid until the owning frame is released.
  ScopedNativePointer get unsafeTexture =>
      _scopedPointer(_textureAddress, 'Metal texture', _owner);

  /// Borrowed `id<MTLDevice>`, valid until the owning frame is released.
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

  /// Session generation that produced this frame.
  final int generation;

  /// Physical Vulkan image width in device pixels.
  final int width;

  /// Physical Vulkan image height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque frame identity that the session uses to reject a stale release.
  final int frameId;

  /// Backend-native `VkFormat` value.
  final int format;

  /// Backend-native `VkImageLayout` value that the image is left in, which
  /// allows the host to sample the frame.
  final int layout;

  final int _imageAddress;
  final int _imageViewAddress;
  final int _deviceAddress;
  final AcquiredFrame _owner;

  /// Borrowed `VkImage`, valid until the owning frame is released.
  ScopedNativePointer get unsafeImage =>
      _scopedPointer(_imageAddress, 'Vulkan image', _owner);

  /// Borrowed `VkImageView`, valid until the owning frame is released.
  ScopedNativePointer get unsafeImageView =>
      _scopedPointer(_imageViewAddress, 'Vulkan image view', _owner);

  /// Borrowed `VkDevice`, valid until the owning frame is released.
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

  /// Session generation that produced this frame.
  final int generation;

  /// Physical WebGPU texture width in device pixels.
  final int width;

  /// Physical WebGPU texture height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque frame identity that the session uses to reject a stale release.
  final int frameId;

  /// Backend-native `WGPUTextureFormat` value.
  final int format;

  final int _textureAddress;
  final int _textureViewAddress;
  final int _deviceAddress;
  final AcquiredFrame _owner;

  /// Borrowed `WGPUTexture`, valid until the owning frame is released.
  ScopedNativePointer get unsafeTexture =>
      _scopedPointer(_textureAddress, 'WebGPU texture', _owner);

  /// Borrowed `WGPUTextureView`, valid until the owning frame is released.
  ScopedNativePointer get unsafeTextureView =>
      _scopedPointer(_textureViewAddress, 'WebGPU texture view', _owner);

  /// Borrowed `WGPUDevice`, valid until the owning frame is released.
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

  /// Session generation that produced this frame.
  final int generation;

  /// Physical OpenGL texture width in device pixels.
  final int width;

  /// Physical OpenGL texture height in device pixels.
  final int height;

  /// UI-to-device pixel scale used for this frame.
  final double scaleFactor;

  /// Opaque frame identity that the session uses to reject a stale release.
  final int frameId;

  /// Borrowed OpenGL texture object name, valid until the owning frame is
  /// released.
  final int texture;

  /// OpenGL texture target, which is expected to be `GL_TEXTURE_2D`.
  final int target;

  /// OpenGL internal format, such as `GL_RGBA8`.
  final int internalFormat;

  /// OpenGL pixel format, such as `GL_RGBA`.
  final int format;

  /// OpenGL pixel type, such as `GL_UNSIGNED_BYTE`.
  final int type;

  final AcquiredFrame _owner;

  /// Throws when the owning frame is already released, so a host can check
  /// [texture] before it uses the name.
  void ensureValid() => _owner._checkOpen();
}

ScopedNativePointer _scopedPointer(
  int address,
  String name,
  AcquiredFrame owner,
) =>
    ScopedNativePointer(address, checkValid: owner._checkOpen, debugName: name);

/// Render-session attachment operations on an any-thread map handle.
///
/// Every attachment copies its descriptor and its
/// [RenderSessionAttachOptions] before returning, and returns a
/// [RenderSessionAttachment] whose session is usable at once. The session
/// finishes initializing its target when the attachment's completion resolves.
/// The isolate that attaches a session receives its
/// [RenderSessionHandle.frameResultsReady] and
/// [RenderSessionHandle.driverWorkReady] notifications.
///
/// A target that requires one [RenderDriver] rejects the other during
/// attachment, which is why the defaults below differ per backend.
extension MapRenderAttachments on MapHandle {
  /// Starts attachment of a Metal surface target.
  ///
  /// A core-worker session retains the Metal layer and device on its worker. A
  /// caller-graphics-thread session initializes the target when the host
  /// services driver work with the Metal context usable on that thread.
  RenderSessionAttachment attachMetalSurface(
    MetalSurfaceDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_metal_surface_descriptor>()
      ..ref = _metalSurfaceDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_metal_surface_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a Vulkan surface target.
  RenderSessionAttachment attachVulkanSurface(
    VulkanSurfaceDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_vulkan_surface_descriptor>()
      ..ref = _vulkanSurfaceDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_vulkan_surface_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of an OpenGL surface target.
  ///
  /// WGL, EGL, and existing WebGL contexts require
  /// [RenderDriver.callerGraphicsThread]. A transferred WebGL canvas also
  /// supports [RenderDriver.coreWorker]. Context ownership stays with the
  /// host.
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
      (policy, session, operation) => raw.mln_opengl_surface_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a session-owned Metal texture ring.
  RenderSessionAttachment attachMetalOwnedTexture(
    MetalOwnedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_metal_owned_texture_descriptor>()
      ..ref = _metalOwnedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_metal_owned_texture_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a caller-owned Metal texture target.
  ///
  /// The host owns the texture and keeps it valid until the session detaches
  /// or closes.
  RenderSessionAttachment attachMetalBorrowedTexture(
    MetalBorrowedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_metal_borrowed_texture_descriptor>()
      ..ref = _metalBorrowedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_metal_borrowed_texture_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a session-owned Vulkan texture ring.
  RenderSessionAttachment attachVulkanOwnedTexture(
    VulkanOwnedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_vulkan_owned_texture_descriptor>()
      ..ref = _vulkanOwnedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_vulkan_owned_texture_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a caller-owned Vulkan texture target.
  ///
  /// The host owns the image, its view, and the context, and keeps them valid
  /// until the session detaches or closes.
  RenderSessionAttachment attachVulkanBorrowedTexture(
    VulkanBorrowedTextureDescriptor descriptor, {
    RenderSessionAttachOptions options = const RenderSessionAttachOptions(),
  }) => withNativeArena((arena) {
    final native = arena<raw.mln_vulkan_borrowed_texture_descriptor>()
      ..ref = _vulkanBorrowedTextureDescriptorToNative(descriptor);
    return _startRenderAttachment(
      options,
      (policy, session, operation) => raw.mln_vulkan_borrowed_texture_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a session-owned OpenGL texture ring.
  ///
  /// Shared WGL, EGL, and existing WebGL contexts require
  /// [RenderDriver.callerGraphicsThread] and grant frame acquisition,
  /// readback, and consumer synchronization. Dedicated EGL and transferred
  /// WebGL contexts require [RenderDriver.coreWorker] and grant readback alone
  /// with a ring depth of one.
  ///
  /// The host keeps every backend handle that the descriptor names valid until
  /// detach completes, including an initialized `EGLDisplay`.
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
      (policy, session, operation) => raw.mln_opengl_owned_texture_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a caller-owned OpenGL texture target.
  ///
  /// The texture belongs to the descriptor's context or to a context in the
  /// same share group, and the host keeps both valid until the session
  /// detaches or closes.
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
      (policy, session, operation) => raw.mln_opengl_borrowed_texture_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a WebGPU surface target.
  ///
  /// Browser targets require [RenderDriver.callerGraphicsThread], because
  /// WebGPU objects stay in the agent that created them.
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
      (policy, session, operation) => raw.mln_webgpu_surface_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a session-owned WebGPU texture ring.
  ///
  /// Browser targets require [RenderDriver.callerGraphicsThread].
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
      (policy, session, operation) => raw.mln_webgpu_owned_texture_attach(
        _handle.raw,
        native,
        policy,
        session,
        operation,
      ),
    );
  });

  /// Starts attachment of a caller-owned WebGPU texture target.
  ///
  /// The descriptor's device creates the texture and its view, and the host
  /// keeps them valid until the session detaches or closes.
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
      (policy, session, operation) => raw.mln_webgpu_borrowed_texture_attach(
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
      Pointer<raw.mln_completion>,
    )
    start,
  ) {
    ensureAbiVersion();
    RenderSessionHandle? renderSession;
    final frameWake = NativeWakeState(() {
      final target = renderSession;
      if (target != null && !target.isClosed) target._notifyFramesReady();
    });
    final driverWorkWake = NativeWakeState(() {
      final target = renderSession;
      if (target != null && !target.isClosed) target._notifyDriverWorkReady();
    });
    try {
      final completed = _runtime._startUnit(
        (completion) => withNativeArena((arena) {
          final policy = arena<raw.mln_render_session_attach_options>()
            ..ref = raw.mln_render_session_attach_options_default();
          policy.ref.driver = options.driver.rawValue;
          policy.ref.requested_texture_ring_depth =
              options.requestedTextureRingDepth;
          frameWake.writeTo(policy.ref.frame_wake);
          driverWorkWake.writeTo(policy.ref.driver_work_wake);
          final session = arena<Uint64>()..value = 0;
          final status = start(policy, session, completion);
          if (status == nativeStatusOk) {
            renderSession = RenderSessionHandle._(
              _runtime,
              NativeRenderSession(session.value),
            );
          }
          return status;
        }),
      );
      final session = renderSession!;
      return RenderSessionAttachment(
        session,
        session._completeWhileRetained(completed),
      );
    } catch (_) {
      frameWake.reject();
      driverWorkWake.reject();
      rethrow;
    }
  }
}
