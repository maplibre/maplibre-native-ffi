import AppKit
import MaplibreNativeFFI
import QuartzCore

/// The display-paced render loop. This view runs on the main thread and owns
/// the window, input decoding, the runtime, the map, the Metal objects, and the
/// render session.
@MainActor
final class MetalMapView: NSView {
  private let metalLayer = CAMetalLayer()
  private let input = InputController()

  private let mode: RenderTargetMode
  private var graphics: MetalGraphicsContext?
  private var mapState: MapState?
  private var renderTarget: MetalRenderTarget?
  private var setupTask: Task<Void, Never>?
  private var timer: Timer?
  private var frameTask: Task<Void, Never>?
  private var shutdownTask: Task<Void, Never>?
  private var pendingUpdates: Task<Void, Never>?
  private var currentViewport: Viewport?
  private var renderRequested = true
  private var pendingResize = false
  private var consecutiveRenderFailures = 0
  private var didLogStartupStatus = false
  private var isShutDown = false
  private var terminateAfterShutdown = false
  private var setupError: Error?
  private var errorLabel: NSTextField?

  override var acceptsFirstResponder: Bool {
    true
  }

  init(mode: RenderTargetMode) {
    self.mode = mode
    super.init(frame: .zero)

    wantsLayer = true
    layer = metalLayer
    do {
      graphics = try MetalGraphicsContext(layer: metalLayer)
    } catch {
      setupError = error
    }
    postsFrameChangedNotifications = true
    if let setupError {
      showError(String(describing: setupError))
    }
  }

  required init?(coder _: NSCoder) {
    return nil
  }

  override func viewDidMoveToWindow() {
    super.viewDidMoveToWindow()
    window?.makeFirstResponder(self)
    startTimerIfNeeded()
    updateViewport()
  }

  override func viewWillMove(toWindow newWindow: NSWindow?) {
    super.viewWillMove(toWindow: newWindow)
    if newWindow == nil {
      Task { @MainActor in await shutdown() }
    }
  }

  /// Stops the host loop, then releases the render session, the map, and the
  /// runtime in that order; a map with an attached session cannot be destroyed.
  /// The caller may await the same teardown from more than one place.
  func shutdown() async {
    beginShutdown()
    await frameTask?.value
    finishShutdown()
    await shutdownTask?.value
  }

  private func beginShutdown(terminate: Bool = false) {
    terminateAfterShutdown = terminateAfterShutdown || terminate
    guard !isShutDown else { return }
    isShutDown = true
    timer?.invalidate()
    timer = nil
    if frameTask == nil {
      finishShutdown()
    }
  }

  private func finishShutdown() {
    guard shutdownTask == nil else { return }
    let target = renderTarget
    renderTarget = nil
    let setup = setupTask
    // A failing update reaches here from inside the update chain, so the chain
    // is dropped rather than awaited; `isShutDown` stops every queued update
    // before it touches the map.
    pendingUpdates = nil
    shutdownTask = Task { @MainActor in
      await setup?.value
      do {
        try await target?.close()
        try await self.mapState?.close()
      } catch {
        print(error)
      }
      self.mapState = nil
      if self.terminateAfterShutdown {
        NSApp.terminate(nil)
      }
    }
  }

  override func layout() {
    super.layout()
    updateViewport()
  }

  override func viewDidChangeBackingProperties() {
    super.viewDidChangeBackingProperties()
    updateViewport()
  }

  override func mouseDown(with event: NSEvent) {
    updateMap { try await self.input.mouseDown(event, mapState: $0) }
  }

  override func rightMouseDown(with event: NSEvent) {
    updateMap { try await self.input.rightMouseDown(event, mapState: $0) }
  }

  override func mouseUp(with event: NSEvent) {
    updateMap { try await self.input.mouseUp(event, mapState: $0) }
  }

  override func rightMouseUp(with event: NSEvent) {
    updateMap { try await self.input.rightMouseUp(event, mapState: $0) }
  }

  override func mouseDragged(with event: NSEvent) {
    updateMap { try await self.input.mouseDragged(event, mapState: $0) }
  }

  override func rightMouseDragged(with event: NSEvent) {
    updateMap { try await self.input.mouseDragged(event, mapState: $0) }
  }

  override func scrollWheel(with event: NSEvent) {
    updateMap {
      try await self.input.scrollWheel(event, in: self, mapState: $0)
    }
  }

  override func keyDown(with event: NSEvent) {
    guard let viewport = currentViewport else { return }
    updateMap {
      try await self.input.keyDown(event, viewport: viewport, mapState: $0)
    }
  }

  /// Submits one camera command. Each task awaits the one before it, so a
  /// gesture-begin submission always reaches the map ahead of the deltas the
  /// same gesture produces; main-actor isolation alone gives exclusion, not
  /// order.
  private func updateMap(
    _ update: @escaping @MainActor (MapState) async throws -> Bool
  ) {
    guard !isShutDown, mapState != nil else { return }
    let previous = pendingUpdates
    pendingUpdates = Task { @MainActor in
      await previous?.value
      guard !self.isShutDown, let state = self.mapState else { return }
      do {
        if try await update(state) {
          self.renderRequested = true
        }
      } catch {
        self.fail(String(describing: error))
      }
    }
  }

  private func startTimerIfNeeded() {
    guard timer == nil else { return }
    // TODO(map-example-spec): Replace the fixed interval with a display-paced
    // host loop. See Frame loop.
    timer = Timer
      .scheduledTimer(withTimeInterval: 1.0 / 60.0,
                      repeats: true)
      { [weak self] _ in
        Task { @MainActor in self?.scheduleFrame() }
      }
    RunLoop.main.add(timer!, forMode: .common)
  }

  /// Creates the map once a non-empty viewport is known, because the map takes
  /// its initial extent from it.
  private func startMapStateIfNeeded(viewport: Viewport) {
    guard mapState == nil, setupTask == nil, !isShutDown else { return }
    setupTask = Task { @MainActor [weak self] in
      guard let self else { return }
      var setupFailure: Error?
      do {
        let state = try await MapState(viewport: viewport)
        if self.isShutDown {
          try await state.close()
        } else {
          if let latest = self.currentViewport, !latest.isEmpty,
             latest != viewport
          {
            try await state.resize(MapLogicalExtent(
              width: latest.logicalWidth,
              height: latest.logicalHeight,
              scaleFactor: latest.scaleFactor
            ))
          }
          self.mapState = state
          state.scheduleEventDrains(
            onRenderRequested: { [weak self] in
              self?.renderRequested = true
            },
            onFailure: { [weak self] error in
              self?.fail(String(describing: error))
            }
          )
          self.renderRequested = true
        }
      } catch {
        setupFailure = error
      }
      self.setupTask = nil
      if let setupFailure {
        self.fail(String(describing: setupFailure))
      }
    }
  }

  private func updateViewport() {
    guard !isShutDown, setupError == nil else { return }
    guard let graphics else { return }
    let viewport = readViewport()

    guard viewport != currentViewport else { return }
    let label = currentViewport == nil ? "initial viewport" : "resized viewport"
    viewport.log(label)
    if viewport.isEmpty {
      currentViewport = viewport
      return
    }

    graphics.resize(viewport)
    currentViewport = viewport
    if renderTarget == nil {
      // With no session attached the map is the only extent authority; a live
      // session carries the extent through its own resize on the next tick.
      updateMap { state in
        try await state.resize(MapLogicalExtent(
          width: viewport.logicalWidth,
          height: viewport.logicalHeight,
          scaleFactor: viewport.scaleFactor
        ))
        return true
      }
    } else {
      pendingResize = true
    }
    renderRequested = true
    startMapStateIfNeeded(viewport: viewport)
  }

  /// Attaches the render session on the graphics thread that services it.
  private func attachIfNeeded() async {
    guard renderTarget == nil,
          let graphics,
          let viewport = currentViewport,
          !viewport.isEmpty,
          let renderMap = mapState?.mapHandle
    else { return }

    do {
      renderTarget = try await MetalRenderTarget.attach(
        mode: mode,
        map: renderMap,
        graphics: graphics,
        viewport: viewport
      )
      // The viewport can change while the attach is in flight; the session
      // was attached at the captured extent, so carry any later change into
      // the next tick's session resize.
      pendingResize = currentViewport != viewport
      if !didLogStartupStatus {
        logStartupStatus(mode: mode)
        didLogStartupStatus = true
      }
      renderRequested = true
    } catch {
      fail(String(describing: error))
    }
  }

  private func scheduleFrame() {
    guard frameTask == nil, !isShutDown else { return }
    frameTask = Task { @MainActor in
      await tick()
      frameTask = nil
      if isShutDown {
        finishShutdown()
      }
    }
  }

  private func tick() async {
    guard !isShutDown else { return }
    await attachIfNeeded()
    guard !isShutDown,
          var renderTarget,
          let graphics,
          let viewport = currentViewport,
          !viewport.isEmpty
    else { return }

    do {
      if pendingResize {
        try await renderTarget.resize(graphics: graphics, viewport: viewport)
        self.renderTarget = renderTarget
        pendingResize = false
      }
      if renderRequested {
        renderRequested = false
        let rendered = try await renderTarget.renderFrame()
        if !rendered {
          renderRequested = true
        }
      }
      consecutiveRenderFailures = 0
    } catch {
      print(error)
      consecutiveRenderFailures += 1
      if consecutiveRenderFailures >= 3 {
        fail(String(describing: error))
      }
    }
  }

  private func fail(_ message: String) {
    print(message)
    showError(message)
    beginShutdown(terminate: true)
  }

  private func readViewport() -> Viewport {
    let rawScale = window?.backingScaleFactor ?? NSScreen.main?
      .backingScaleFactor ?? 1.0
    let scale = rawScale.isFinite && rawScale > 0 ? rawScale : 1.0
    let rawLogicalWidth = bounds.width
    let rawLogicalHeight = bounds.height
    let rawPhysicalWidth = rawLogicalWidth * scale
    let rawPhysicalHeight = rawLogicalHeight * scale
    let empty = rawLogicalWidth <= 0 ||
      rawLogicalHeight <= 0 ||
      rawPhysicalWidth <= 0 ||
      rawPhysicalHeight <= 0
    return Viewport(
      logicalWidth: empty ? 0 : max(UInt32(ceil(rawLogicalWidth)), 1),
      logicalHeight: empty ? 0 : max(UInt32(ceil(rawLogicalHeight)), 1),
      physicalWidth: empty ? 0 : max(UInt32(ceil(rawPhysicalWidth)), 1),
      physicalHeight: empty ? 0 : max(UInt32(ceil(rawPhysicalHeight)), 1),
      scaleFactor: scale,
      isEmpty: empty
    )
  }

  private func showError(_ message: String) {
    if errorLabel == nil {
      let label = NSTextField(labelWithString: "")
      label.translatesAutoresizingMaskIntoConstraints = false
      label.maximumNumberOfLines = 0
      label.alignment = .center
      addSubview(label)
      NSLayoutConstraint.activate([
        label.leadingAnchor.constraint(
          greaterThanOrEqualTo: leadingAnchor,
          constant: 24
        ),
        label.trailingAnchor.constraint(
          lessThanOrEqualTo: trailingAnchor,
          constant: -24
        ),
        label.centerXAnchor.constraint(equalTo: centerXAnchor),
        label.centerYAnchor.constraint(equalTo: centerYAnchor),
      ])
      errorLabel = label
    }
    errorLabel?.stringValue = message
  }
}
