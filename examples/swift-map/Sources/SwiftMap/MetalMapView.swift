import AppKit
import MaplibreNative
import QuartzCore

@MainActor
final class MetalMapView: NSView {
  private let metalLayer = CAMetalLayer()
  private let input = InputController()
  private let mode: RenderTargetMode
  private var graphics: MetalGraphicsContext?
  private var mapState: MapState?
  private var timer: Timer?
  private var currentViewport: Viewport?
  private var renderPending = true
  private var consecutiveRenderFailures = 0
  private var didLogStartupStatus = false
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
      showError(setupError)
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

  @objc private func shutdown() {
    timer?.invalidate()
    timer = nil
    do {
      try mapState?.close()
    } catch {
      print(error)
    }
    mapState = nil
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
    handleInput { map in try input.mouseDown(event, map: map) }
  }

  override func rightMouseDown(with event: NSEvent) {
    handleInput { map in try input.rightMouseDown(event, map: map) }
  }

  override func mouseUp(with event: NSEvent) {
    if input.mouseUp(event) { renderPending = true }
  }

  override func rightMouseUp(with event: NSEvent) {
    if input.mouseUp(event) { renderPending = true }
  }

  override func mouseDragged(with event: NSEvent) {
    handleInput { map in try input.mouseDragged(event, map: map) }
  }

  override func rightMouseDragged(with event: NSEvent) {
    handleInput { map in try input.mouseDragged(event, map: map) }
  }

  override func scrollWheel(with event: NSEvent) {
    handleInput { map in try input.scrollWheel(event, map: map, in: self) }
  }

  override func keyDown(with event: NSEvent) {
    guard let viewport = currentViewport else { return }
    handleInput { map in
      try input.keyDown(event, map: map, viewport: viewport)
    }
  }

  private func handleInput(_ action: (MapHandle) throws -> Bool) {
    guard let map = mapState?.map else { return }
    do {
      if try action(map) { renderPending = true }
    } catch {
      print(error)
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

  private func updateViewport() {
    guard setupError == nil else { return }
    guard let graphics else { return }
    let viewport = readViewport()

    guard viewport != currentViewport else { return }
    let label = currentViewport == nil ? "initial viewport" : "resized viewport"
    viewport.log(label)
    if viewport.isEmpty {
      currentViewport = viewport
      renderPending = false
      return
    }

    do {
      graphics.resize(viewport)
      if mapState == nil {
        mapState = try MapState(
          mode: mode,
          viewport: viewport,
          graphics: graphics
        )
        if !didLogStartupStatus {
          logStartupStatus(mode: mode)
          didLogStartupStatus = true
        }
      } else {
        try mapState?.resize(viewport, graphics: graphics)
      }
      currentViewport = viewport
      renderPending = true
    } catch {
      print(error)
      showError(error)
    }
  }

  private func tick() {
    guard let mapState else { return }
    do {
      try mapState.runOnce()
      renderPending = try mapState.drainEvents() || renderPending
      try mapState.finishFrame()
      guard let viewport = currentViewport, !viewport.isEmpty else { return }
      guard renderPending else { return }
      if try mapState.render() {
        renderPending = false
        consecutiveRenderFailures = 0
      }
    } catch {
      print(error)
      consecutiveRenderFailures += 1
      if consecutiveRenderFailures >= 3 {
        timer?.invalidate()
        timer = nil
        showError(error)
        shutdown()
        NSApp.terminate(nil)
      }
    }
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

  private func showError(_ error: Error) {
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
    errorLabel?.stringValue = String(describing: error)
  }
}
