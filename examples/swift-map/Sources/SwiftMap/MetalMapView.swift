import AppKit
import MaplibreNativeFFI
import QuartzCore

/// The display-paced render loop. This view runs on the main thread and owns
/// the window, input decoding, the Metal objects, and the render session. The
/// runtime and the map live on the asynchronous runtime task it starts, reached
/// through ``Channels``.
@MainActor
final class MetalMapView: NSView {
  private let metalLayer = CAMetalLayer()
  private let input = InputController()
  private let mode: RenderTargetMode
  private let channels = Channels()
  private var graphics: MetalGraphicsContext?
  private var renderTarget: MetalRenderTarget?
  private var mapTask: MapTask?
  private var timer: Timer?
  private var frameTask: Task<Void, Never>?
  private var shutdownTask: Task<Void, Never>?
  private var currentViewport: Viewport?
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
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(shutdown),
      name: AppDelegate.willTerminateMapViews,
      object: nil
    )
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
      shutdown()
    }
  }

  /// Closes the session before the map task closes the map; a map with an
  /// attached session cannot be destroyed.
  @objc private func shutdown() {
    beginShutdown()
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
    shutdownTask = Task { @MainActor in
      do {
        try await target?.close()
      } catch {
        print(error)
      }
      if self.mapTask != nil {
        self.channels.requestShutdown()
        if !self.channels.waitForMapTaskExit(timeout: 5.0) {
          print("map task did not finish before the shutdown deadline")
        }
        self.mapTask = nil
      }
      NotificationCenter.default.removeObserver(self)
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
    requestRenderIfCameraChanged(input.mouseDown(event, commands: channels))
  }

  override func rightMouseDown(with event: NSEvent) {
    requestRenderIfCameraChanged(
      input.rightMouseDown(event, commands: channels)
    )
  }

  override func mouseUp(with event: NSEvent) {
    requestRenderIfCameraChanged(input.mouseUp(event, commands: channels))
  }

  override func rightMouseUp(with event: NSEvent) {
    requestRenderIfCameraChanged(input.rightMouseUp(event, commands: channels))
  }

  override func mouseDragged(with event: NSEvent) {
    requestRenderIfCameraChanged(input.mouseDragged(event, commands: channels))
  }

  override func rightMouseDragged(with event: NSEvent) {
    requestRenderIfCameraChanged(input.mouseDragged(event, commands: channels))
  }

  override func scrollWheel(with event: NSEvent) {
    requestRenderIfCameraChanged(
      input.scrollWheel(event, commands: channels, in: self)
    )
  }

  override func keyDown(with event: NSEvent) {
    guard let viewport = currentViewport else { return }
    requestRenderIfCameraChanged(
      input.keyDown(event, commands: channels, viewport: viewport)
    )
  }

  private func requestRenderIfCameraChanged(_ cameraChanged: Bool) {
    if cameraChanged {
      channels.setRenderRequest()
    }
  }

  private func startTimerIfNeeded() {
    guard timer == nil else { return }
    timer = Timer
      .scheduledTimer(withTimeInterval: 1.0 / 60.0,
                      repeats: true)
      { [weak self] _ in
        Task { @MainActor in self?.scheduleFrame() }
      }
    RunLoop.main.add(timer!, forMode: .common)
  }

  /// Starts the map task once a non-empty viewport is known, because the map
  /// takes its initial extent from it.
  private func startMapTaskIfNeeded(viewport: Viewport) {
    guard mapTask == nil, !isShutDown else { return }
    let task = MapTask(channels: channels, viewport: viewport)
    mapTask = task
    task.start()
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
    if mapTask != nil {
      channels.push(.resize(MapLogicalExtent(
        width: viewport.logicalWidth,
        height: viewport.logicalHeight,
        scaleFactor: viewport.scaleFactor
      )))
    }
    channels.setRenderRequest()
    startMapTaskIfNeeded(viewport: viewport)
  }

  /// Attaches the render session on this thread. Attach records the calling
  /// thread as the session's owner, and every later session call runs here.
  private func attachIfNeeded() async {
    guard renderTarget == nil,
          let graphics,
          let viewport = currentViewport,
          !viewport.isEmpty,
          let renderMap = channels.map()
    else { return }

    do {
      renderTarget = try await MetalRenderTarget.attach(
        mode: mode,
        map: renderMap,
        graphics: graphics,
        viewport: viewport
      )
      if !didLogStartupStatus {
        logStartupStatus(mode: mode)
        didLogStartupStatus = true
      }
      channels.setRenderRequest()
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
    if let failureMessage = channels.failureMessage {
      fail(failureMessage)
      return
    }
    await attachIfNeeded()
    guard !isShutDown,
          var renderTarget,
          let graphics,
          let viewport = currentViewport,
          !viewport.isEmpty
    else { return }

    do {
      if try renderTargetNeedsResize(renderTarget, viewport: viewport) {
        try await renderTarget.resize(graphics: graphics, viewport: viewport)
        self.renderTarget = renderTarget
      }
      if channels.consumeRenderRequest() {
        let rendered = try await renderTarget.renderFrame()
        if !rendered {
          channels.setRenderRequest()
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

  private func renderTargetNeedsResize(
    _ target: MetalRenderTarget,
    viewport: Viewport
  ) throws -> Bool {
    switch target {
    case let .ownedTexture(session, _),
         let .borrowedTexture(session, _, _),
         let .nativeSurface(session):
      return try session.snapshot().extent != viewport.extent
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
