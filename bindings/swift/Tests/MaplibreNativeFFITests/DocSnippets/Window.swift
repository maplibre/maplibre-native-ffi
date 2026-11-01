// Snippets for the docs site's Render to a window page. They compile with the
// tests, which never run them.

#if os(Android) || os(Linux) || os(macOS) || os(Windows)
  import MaplibreNativeFFI

  /// Stand-ins for the host's event loop, display timer, graphics thread, and
  /// session replacement.
  private func runOnEventLoop(
    _: @escaping @Sendable @MainActor () throws -> Void
  ) {}

  @MainActor private func atNextRefresh(
    _: @escaping @MainActor () throws -> Void
  ) {}

  private func runOnGraphicsThread(_: @escaping @Sendable () throws -> Void) {}

  @MainActor private func replaceSession() {}

  private func destroySurface() {}

  // #region attach

  @MainActor private var session: RenderSessionHandle!
  // #endregion attach

  // #region frame-loop

  /// Set while the window has no surface, and when it closes.
  @MainActor private var paused = false

  @MainActor private func requestFrame() throws {
    try session.requestFrame(
      demand: FrameDemand(flags: [.present, .waitForUpdate])
    )
  }

  @MainActor private func drainFrameResults() throws {
    let results = try session.drainFrameResults()
    guard !results.isEmpty else {
      return // No results yet.
    }

    if results.contains(where: { $0.disposition == .failed }) {
      replaceSession() // Close the session and attach a new one.
    } else if !paused {
      atNextRefresh(requestFrame)
    }
  }

  // #endregion frame-loop

  // #region frame-wake

  /// Runs on a library thread.
  private func onFrameWake() {
    runOnEventLoop(drainFrameResults)
  }

  // #endregion frame-wake

  // #region resize

  @MainActor private func onWindowResized(
    width: UInt32,
    height: UInt32
  ) throws {
    // The scale factor the session has now.
    try session.discarding.resize(
      extent: LogicalExtent(width: width, height: height, scaleFactor: 2)
    )
  }

  // #endregion resize

  // #region service

  /// Runs on a library thread.
  private func onDriverWorkWake() {
    // Read the session on the main actor, which owns it.
    runOnEventLoop {
      let current: RenderSessionHandle = session
      runOnGraphicsThread {
        // Your graphics context is current on this thread.
        try current.serviceDriverWork()
      }
    }
  }

  // #endregion service

  @MainActor private func surfaceLost() async throws {
    // #region lost-surface
    paused = true
    try await session.releaseTarget()
    // #endregion lost-surface
  }

  @MainActor private func surfaceRestored(
    vulkan: VulkanContextDescriptor,
    newSurface: UInt64
  ) async throws {
    // #region lost-surface
    try await session.setVulkanSurfaceTarget(
      descriptor: VulkanSurfaceDescriptor(
        extent: LogicalExtent(width: 800, height: 600, scaleFactor: 2),
        context: vulkan,
        surface: newSurface
      )
    )
    paused = false
    try requestFrame()
    // #endregion lost-surface
  }

  @MainActor private func renderToWindow(
    map: MapHandle,
    vulkan: VulkanContextDescriptor,
    windowSurface: UInt64
  ) async throws {
    // #region attach
    let attached = try map.attachVulkanSurface(
      descriptor: VulkanSurfaceDescriptor(
        // The window's size in logical pixels, and its scale factor.
        extent: LogicalExtent(width: 800, height: 600, scaleFactor: 2),
        // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
        context: vulkan,
        // Your VkSurfaceKHR.
        surface: windowSurface
      ),
      options: RenderSessionAttachOptions(
        driver: .coreWorker,
        frameWake: Wake(callback: onFrameWake)
      )
    )
    session = attached.session
    try await attached.ready.value
    // #endregion attach

    try requestFrame()
    try onWindowResized(width: 1024, height: 768)

    // #region service
    let graphicsThreadOptions = RenderSessionAttachOptions(
      driver: .callerGraphicsThread,
      frameWake: Wake(callback: onFrameWake),
      driverWorkWake: Wake(callback: onDriverWorkWake)
    )
    // #endregion service
    _ = graphicsThreadOptions

    // #region close
    paused = true
    try await session.close()
    destroySurface() // For example, with vkDestroySurfaceKHR.
    // #endregion close
  }
#endif
