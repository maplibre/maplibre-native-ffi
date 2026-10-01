import AppKit
import MaplibreNativeFFI
import QuartzCore

/// The map view. It runs on the main thread and owns the window's input
/// decoding, the Metal objects, and the render loop, which owns the map and the
/// render session.
@MainActor
final class MetalMapView: NSView {
  private let metalLayer = CAMetalLayer()
  private let input = InputController()

  private let mode: RenderTargetMode
  private var graphics: MetalGraphicsContext?
  private var loop: RenderLoop?
  private var startup: Task<Void, Never>?
  private var shutdownTask: Task<Void, Never>?
  private var currentViewport: Viewport?
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
      showError(String(describing: error))
    }
    postsFrameChangedNotifications = true
  }

  required init?(coder _: NSCoder) {
    return nil
  }

  override func viewDidMoveToWindow() {
    super.viewDidMoveToWindow()
    window?.makeFirstResponder(self)
    updateViewport()
  }

  override func viewWillMove(toWindow newWindow: NSWindow?) {
    super.viewWillMove(toWindow: newWindow)
    if newWindow == nil {
      Task { @MainActor in await shutdown() }
    }
  }

  /// Closes the render loop once startup settles. The caller may await the
  /// same teardown from more than one place.
  func shutdown() async {
    if shutdownTask == nil {
      let startup = startup
      shutdownTask = Task { @MainActor in
        await startup?.value
        do {
          try await self.loop?.close()
        } catch {
          print(error)
        }
        self.loop = nil
      }
    }
    await shutdownTask?.value
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
    if let mapState { input.mouseDown(event, mapState: mapState) }
  }

  override func rightMouseDown(with event: NSEvent) {
    if let mapState { input.rightMouseDown(event, mapState: mapState) }
  }

  override func mouseUp(with event: NSEvent) {
    if let mapState { input.mouseUp(event, mapState: mapState) }
  }

  override func rightMouseUp(with event: NSEvent) {
    if let mapState { input.rightMouseUp(event, mapState: mapState) }
  }

  override func mouseDragged(with event: NSEvent) {
    if let mapState { input.mouseDragged(event, mapState: mapState) }
  }

  override func rightMouseDragged(with event: NSEvent) {
    if let mapState { input.mouseDragged(event, mapState: mapState) }
  }

  override func scrollWheel(with event: NSEvent) {
    if let mapState { input.scrollWheel(event, in: self, mapState: mapState) }
  }

  override func keyDown(with event: NSEvent) {
    guard let viewport = currentViewport else { return }
    if let mapState { input.keyDown(
      event,
      viewport: viewport,
      mapState: mapState
    ) }
  }

  /// The map that input drives, once the loop is running.
  private var mapState: MapState? {
    shutdownTask == nil ? loop?.mapState : nil
  }

  /// Starts the loop once a non-empty viewport is known, because the map takes
  /// its initial extent from it.
  private func startIfNeeded(viewport: Viewport) {
    guard startup == nil, shutdownTask == nil, let graphics else { return }
    startup = Task { @MainActor in
      do {
        let loop = try await RenderLoop.start(
          mode: mode,
          graphics: graphics,
          viewport: viewport
        )
        loop.onFailure = { [weak self] in self?.fail(String(describing: $0)) }
        self.loop = loop
        logStartupStatus(mode: mode)
        // The viewport can change while startup is in flight.
        if let latest = currentViewport, latest != viewport, !latest.isEmpty {
          loop.resize(latest)
        }
      } catch {
        fail(String(describing: error))
      }
    }
  }

  private func updateViewport() {
    guard shutdownTask == nil, let graphics else { return }
    let viewport = readViewport()
    guard viewport != currentViewport else { return }
    viewport
      .log(currentViewport == nil ? "initial viewport" : "resized viewport")
    currentViewport = viewport
    guard !viewport.isEmpty else { return }
    graphics.resize(viewport)
    if let loop {
      loop.resize(viewport)
    } else {
      startIfNeeded(viewport: viewport)
    }
  }

  private func fail(_ message: String) {
    print(message)
    showError(message)
    Task { @MainActor in
      await shutdown()
      NSApp.terminate(nil)
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
