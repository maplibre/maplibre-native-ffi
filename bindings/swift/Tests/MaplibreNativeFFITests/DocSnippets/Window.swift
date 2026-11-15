// Snippets for the docs site's Render to a window page. They compile
// with the
// tests, which never run them.

import MaplibreNativeFFI

/// Stand-ins for the host's event loop, display timer, graphics
/// thread, and
/// session replacement.
private func runOnEventLoop(
  _: @escaping @Sendable @MainActor () throws -> Void
) {}

private func atNextRefresh(
  _: @escaping @Sendable @MainActor () throws -> Void
) {}

private func runOnGraphicsThread(_: @escaping @Sendable () throws
  -> Void) {}

@MainActor private func replaceSession() {}

private func destroySurface() {}

// #region attach

@MainActor private var session: RenderSessionHandle!
// #endregion attach

// #region frame-loop

/// Draws the map if it changed since the last frame.
@MainActor private func demandFrame() throws {
  try session.demandFrame(FrameDemand())
}

@MainActor private func drainFrameResults() throws {
  let results = try session.drainFrameResults()
  guard !results.isEmpty else {
    return // No results yet.
  }

  if results.contains(where: { $0.disposition == .failed }) {
    replaceSession() // Release the session and attach a new one.
  }
}

// #endregion frame-loop

// #region frame-wake

/// Both run on a library thread.
private func onRedrawWake() {
  atNextRefresh(demandFrame)
}

private func onFrameWake() {
  runOnEventLoop(drainFrameResults)
}

// #endregion frame-wake

// #region resize

/// Runs when the window changes size, or moves to a display with
/// another scale factor.
@MainActor private func onWindowResized(
  width: UInt32,
  height: UInt32,
  scaleFactor: Double
) throws {
  // The window's new size in logical pixels, and its scale factor.
  try session.discarding.resize(extent: LogicalExtent(
    width: width,
    height: height,
    scaleFactor: scaleFactor
  ))
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
  try await session.releaseTarget()
  // #endregion lost-surface
}

@MainActor private func surfaceRestored(
  display: NativePointer,
  newSurface: NativePointer
) async throws {
  // #region lost-surface
  try await session.setTarget(.waylandSurface(WaylandSurfaceTarget(
    logicalSize: LogicalSize(width: 800, height: 600),
    display: display,
    surface: newSurface
  )))
  // #endregion lost-surface
}

@MainActor private func renderToWindow(
  map: MapHandle,
  display: NativePointer,
  windowSurface: NativePointer
) async throws {
  // #region attach
  // With no device, the session creates one on the platform's own
  // backend.
  session = try map.attach(
    target: .waylandSurface(WaylandSurfaceTarget(
      // The window's size in logical pixels. The map's scale factor
      // turns it into pixels.
      logicalSize: LogicalSize(width: 800, height: 600),
      // Your wl_display and wl_surface.
      display: display,
      surface: windowSurface
    )),
    options: RenderSessionAttachOptions(
      driver: .coreWorker,
      frameWake: Wake(callback: onFrameWake),
      redrawWake: Wake(callback: onRedrawWake)
    )
  )
  try await session.ready
  // #endregion attach

  try onWindowResized(width: 1024, height: 768, scaleFactor: 2)

  // #region service
  let graphicsThreadOptions = RenderSessionAttachOptions(
    driver: .callerGraphicsThread,
    frameWake: Wake(callback: onFrameWake),
    redrawWake: Wake(callback: onRedrawWake),
    driverWorkWake: Wake(callback: onDriverWorkWake)
  )
  // #endregion service
  _ = graphicsThreadOptions

  try await session.release()
  destroySurface() // For example, with wl_surface_destroy.
}
