import AppKit
import MaplibreNative
import QuartzCore

/// The display-paced render loop.
///
/// This view runs on the main thread, which is the render loop thread: it owns
/// the window, input decoding, the Metal objects, and the render session for
/// the session's whole lifetime. The runtime and the map live on the runtime
/// loop thread this view starts, and ``Channels`` is the only state that
/// crosses between the two.
///
/// Input handlers never touch the map. They decode events into camera commands,
/// because a read-modify-write camera change has to run whole on the map's own
/// thread.
@MainActor
final class MetalMapView: NSView {
  private let metalLayer = CAMetalLayer()
  private let input = InputController()
  private let mode: RenderTargetMode
  private let channels = Channels()
  private var graphics: MetalGraphicsContext?
  private var renderTarget: MetalRenderTarget?
  private var runtimeLoop: RuntimeLoopThread?
  private var timer: Timer?
  private var currentViewport: Viewport?
  private var consecutiveRenderFailures = 0
  private var didLogStartupStatus = false
  private var isShutDown = false
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

  /// Closes the session before the runtime loop closes the map, because native
  /// refuses to destroy a map that still has a session attached.
  @objc private func shutdown() {
    guard !isShutDown else { return }
    isShutDown = true
    timer?.invalidate()
    timer = nil
    do {
      try renderTarget?.close()
    } catch {
      print(error)
    }
    renderTarget = nil
    if runtimeLoop != nil {
      channels.requestShutdown()
      if !channels.waitForRuntimeLoopExit(timeout: 5.0) {
        print("runtime loop did not finish before the shutdown deadline")
      }
      runtimeLoop = nil
    }
    NotificationCenter.default.removeObserver(self)
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
    // TODO(map-example-spec): Replace fixed NSTimer with a display-paced host loop. See Frame loop.
    timer = Timer
      .scheduledTimer(withTimeInterval: 1.0 / 60.0,
                      repeats: true)
      { [weak self] _ in
        Task { @MainActor in self?.tick() }
      }
    RunLoop.main.add(timer!, forMode: .common)
  }

  /// Starts the runtime loop once a non-empty viewport is known, because the
  /// map takes its initial extent from it.
  ///
  /// The runtime loop needs a native thread whose identity is stable for its
  /// whole life, so it is a `Thread` rather than a `DispatchQueue`, an `actor`,
  /// or a `Task`.
  private func startRuntimeLoopIfNeeded(viewport: Viewport) {
    guard runtimeLoop == nil, !isShutDown else { return }
    let loop = RuntimeLoopThread(channels: channels, viewport: viewport)
    runtimeLoop = loop
    loop.start()
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

    do {
      graphics.resize(viewport)
      if let renderTarget {
        if renderTarget.needsReattachOnResize {
          // The host texture is fixed to the viewport size, so close the
          // session and let the next tick attach a replacement against a fresh
          // texture. Both halves run here, on the thread that owns the session
          // either way.
          try renderTarget.close()
          self.renderTarget = nil
        } else {
          try renderTarget.resize(viewport)
        }
      }
      currentViewport = viewport
      channels.setRenderRequest()
      startRuntimeLoopIfNeeded(viewport: viewport)
    } catch {
      print(error)
      showError(String(describing: error))
    }
  }

  /// Attaches the render session on this thread.
  ///
  /// Attach records the calling thread as the session's owner, so it happens
  /// here, where the Metal objects live and where every later session call
  /// runs.
  private func attachIfNeeded() {
    guard renderTarget == nil,
          let graphics,
          let viewport = currentViewport,
          !viewport.isEmpty,
          let attachRef = channels.attachRef()
    else { return }

    do {
      renderTarget = try MetalRenderTarget.attach(
        mode: mode,
        attachRef: attachRef,
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

  private func tick() {
    guard !isShutDown else { return }
    if let failureMessage = channels.failureMessage {
      fail(failureMessage)
      return
    }
    attachIfNeeded()
    guard let renderTarget,
          let viewport = currentViewport,
          !viewport.isEmpty
    else { return }

    do {
      // Consume the request before rendering, so one the runtime loop publishes
      // during the render call is not discarded, and set it again when nothing
      // was rendered.
      if channels.consumeRenderRequest() {
        let rendered = try renderTarget.renderUpdate()
        if !rendered {
          channels.setRenderRequest()
        }
      }
      try renderTarget.finishFrame()
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
    shutdown()
    NSApp.terminate(nil)
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
