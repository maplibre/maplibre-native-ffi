import MaplibreNativeFFI
import Metal
import os
import QuartzCore
import UIKit

/// The display-paced render loop. This view runs on the main thread and owns
/// the layer, gesture decoding, the Metal objects, and the render session. The
/// runtime and the map live on the runtime loop thread it starts, reached
/// through ``Channels``.
@MainActor
final class MetalMapView: UIView {
  static let willTerminateMapViews = Notification
    .Name("SwiftMapIOSWillTerminateMapViews")
  private let log = Logger(
    subsystem: "org.maplibre.nativeffi.examples.swift-map-ios",
    category: "MapView"
  )
  private let channels = Channels()
  private var graphics: MetalGraphicsContext?
  private var renderTarget: MetalRenderTarget?
  private var runtimeLoop: RuntimeLoopThread?
  private var displayLink: CADisplayLink?
  private var currentViewport: Viewport?
  private var didLogStartupStatus = false
  private var viewVisible = false
  private var appForeground = true
  private var isShutDown = false

  /// The recognizers with a gesture still open. Pinch, rotation, and shove
  /// recognize simultaneously and report to the map as one gesture.
  private var openGestures = Set<ObjectIdentifier>()

  override class var layerClass: AnyClass {
    CAMetalLayer.self
  }

  private var metalLayer: CAMetalLayer {
    layer as! CAMetalLayer
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    backgroundColor = .black
    isMultipleTouchEnabled = true
    do {
      graphics = try MetalGraphicsContext(layer: metalLayer)
    } catch {
      showError(error)
    }
    installGestures()
    installLifecycleObservers()
  }

  required init?(coder _: NSCoder) {
    nil
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
    MainActor.assumeIsolated {
      stopHostLoop()
      teardown()
    }
  }

  override func didMoveToWindow() {
    super.didMoveToWindow()
    viewVisible = window != nil
    if viewVisible {
      refreshAndStartIfNeeded()
    } else {
      stopHostLoop()
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    refreshViewport()
  }

  @objc private func enterForeground() {
    appForeground = true
    refreshAndStartIfNeeded()
  }

  @objc private func enterBackground() {
    appForeground = false
    stopHostLoop()
  }

  @objc private func closeMap() {
    stopHostLoop()
    teardown()
  }

  /// Closes the session before the runtime loop closes the map; a map with an
  /// attached session cannot be destroyed.
  private func teardown() {
    guard !isShutDown else { return }
    isShutDown = true
    do {
      try renderTarget?.close()
    } catch {
      log.error("\(String(describing: error), privacy: .public)")
    }
    renderTarget = nil
    guard runtimeLoop != nil else { return }
    channels.requestShutdown()
    if !channels.waitForRuntimeLoopExit(timeout: 5.0) {
      log.error("runtime loop did not finish before the shutdown deadline")
    }
    runtimeLoop = nil
  }

  @objc private func displayLinkTick() {
    guard !isShutDown else { return }
    if let failureMessage = channels.failureMessage {
      log.error("\(failureMessage, privacy: .public)")
      // The runtime loop waits for the session to close before destroying the
      // map, so tear down rather than only stopping the display link.
      stopHostLoop()
      teardown()
      return
    }
    attachIfNeeded()
    guard let renderTarget,
          let viewport = currentViewport,
          !viewport.isEmpty
    else { return }

    do {
      // Consume first, so a request published during the render survives.
      if channels.consumeRenderRequest() {
        let rendered = try renderTarget.renderUpdate()
        if !rendered {
          channels.setRenderRequest()
        }
      }
      try renderTarget.finishFrame()
    } catch {
      showError(error)
      // Stopping the display link alone leaves the session attached, and the
      // runtime loop would hold a map it can never destroy.
      stopHostLoop()
      teardown()
    }
  }

  /// Attaches the render session on this thread. Attach records the calling
  /// thread as the session's owner, and every later session call runs here.
  private func attachIfNeeded() {
    guard renderTarget == nil,
          let graphics,
          let viewport = currentViewport,
          !viewport.isEmpty,
          let attachRef = channels.attachRef()
    else { return }

    do {
      renderTarget = try MetalRenderTarget.attach(
        attachRef: attachRef,
        graphics: graphics,
        viewport: viewport
      )
      if !didLogStartupStatus {
        log.info("render target: native-surface")
        log.info(
          "render target status: renders directly to the host view surface"
        )
        didLogStartupStatus = true
      }
      channels.setRenderRequest()
    } catch {
      showError(error)
      // The runtime loop waits for the session to close before destroying the
      // map, so tear down rather than only stopping the display link.
      stopHostLoop()
      teardown()
    }
  }

  private func refreshAndStartIfNeeded() {
    guard !isShutDown else { return }
    refreshViewport()
    if viewVisible, appForeground {
      channels.setRenderRequest()
      startHostLoop()
    }
  }

  private func startHostLoop() {
    guard displayLink == nil else { return }
    let link = CADisplayLink(target: self, selector: #selector(displayLinkTick))
    link.add(to: .main, forMode: .common)
    displayLink = link
  }

  private func stopHostLoop() {
    displayLink?.invalidate()
    displayLink = nil
  }

  /// Starts the runtime loop once a non-empty viewport is known, because the
  /// map takes its initial extent from it.
  private func startRuntimeLoopIfNeeded(viewport: Viewport) {
    guard runtimeLoop == nil else { return }
    let loop = RuntimeLoopThread(channels: channels, viewport: viewport)
    runtimeLoop = loop
    loop.start()
  }

  private func refreshViewport() {
    guard !isShutDown, let graphics else { return }
    let viewport = readViewport()
    guard viewport != currentViewport else { return }
    viewport
      .log(currentViewport == nil ? "initial viewport" : "resized viewport")
    if viewport.isEmpty {
      currentViewport = viewport
      return
    }

    do {
      graphics.resize(viewport)
      try renderTarget?.resize(viewport)
      currentViewport = viewport
      channels.setRenderRequest()
      startRuntimeLoopIfNeeded(viewport: viewport)
    } catch {
      showError(error)
    }
  }

  private func readViewport() -> Viewport {
    let scale = traitCollection.displayScale > 0 ? traitCollection
      .displayScale :
      UIScreen.main.scale
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

  private func installGestures() {
    let pan = UIPanGestureRecognizer(target: self, action: #selector(handlePan))
    pan.maximumNumberOfTouches = 1

    let pinch = UIPinchGestureRecognizer(
      target: self,
      action: #selector(handlePinch)
    )
    let rotate = UIRotationGestureRecognizer(
      target: self,
      action: #selector(handleRotation)
    )
    let shove = UIPanGestureRecognizer(
      target: self,
      action: #selector(handleShove)
    )
    shove.minimumNumberOfTouches = 2
    shove.maximumNumberOfTouches = 2

    let doubleTap = UITapGestureRecognizer(
      target: self,
      action: #selector(handleDoubleTap)
    )
    doubleTap.numberOfTapsRequired = 2

    pinch.delegate = self
    rotate.delegate = self
    shove.delegate = self
    addGestureRecognizer(pan)
    addGestureRecognizer(pinch)
    addGestureRecognizer(rotate)
    addGestureRecognizer(shove)
    addGestureRecognizer(doubleTap)
  }

  private func installLifecycleObservers() {
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(enterForeground),
      name: UIApplication.willEnterForegroundNotification,
      object: nil
    )
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(enterBackground),
      name: UIApplication.didEnterBackgroundNotification,
      object: nil
    )
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(closeMap),
      name: Self.willTerminateMapViews,
      object: nil
    )
  }

  /// Queues decoded camera commands and sets the render request when the
  /// gesture changed the camera.
  private func enqueue(_ decode: (Channels) -> Bool) {
    if decode(channels) {
      channels.setRenderRequest()
    }
  }

  /// Opens the gesture bracket for the first recognizer to begin.
  private func beginGesture(
    _ recognizer: UIGestureRecognizer,
    _ commands: Channels
  ) {
    if openGestures.isEmpty {
      commands.push(.setGestureInProgress(true))
    }
    openGestures.insert(ObjectIdentifier(recognizer))
  }

  /// Closes the bracket once the last recognizer ends or is cancelled, so each
  /// open is paired with a close.
  private func endGesture(
    _ recognizer: UIGestureRecognizer,
    _ commands: Channels
  ) {
    guard openGestures.remove(ObjectIdentifier(recognizer)) != nil else {
      return
    }
    if openGestures.isEmpty {
      commands.push(.setGestureInProgress(false))
    }
  }

  @objc private func handlePan(_ recognizer: UIPanGestureRecognizer) {
    enqueue { commands in
      switch recognizer.state {
      case .began:
        commands.push(.cancelTransitions)
        self.beginGesture(recognizer, commands)
        recognizer.setTranslation(.zero, in: self)
        return false
      case .changed:
        let translation = recognizer.translation(in: self)
        recognizer.setTranslation(.zero, in: self)
        guard translation != .zero else { return false }
        commands.push(.moveBy(
          dx: Double(translation.x),
          dy: Double(translation.y)
        ))
        return true
      default:
        self.endGesture(recognizer, commands)
        return false
      }
    }
  }

  @objc private func handlePinch(_ recognizer: UIPinchGestureRecognizer) {
    enqueue { commands in
      switch recognizer.state {
      case .began:
        commands.push(.cancelTransitions)
        self.beginGesture(recognizer, commands)
        recognizer.scale = 1.0
        return false
      case .changed:
        let scale = Double(recognizer.scale)
        recognizer.scale = 1.0
        guard scale.isFinite, scale > 0 else { return false }
        let location = recognizer.location(in: self)
        commands.push(.scaleBy(
          scale: scale,
          anchor: self.screenPoint(location)
        ))
        return true
      default:
        self.endGesture(recognizer, commands)
        return false
      }
    }
  }

  @objc private func handleRotation(_ recognizer: UIRotationGestureRecognizer) {
    enqueue { commands in
      switch recognizer.state {
      case .began:
        commands.push(.cancelTransitions)
        self.beginGesture(recognizer, commands)
        recognizer.rotation = 0
        return false
      case .changed:
        let deltaRadians = recognizer.rotation
        recognizer.rotation = 0
        guard deltaRadians != 0 else { return false }
        let location = recognizer.location(in: self)
        commands.push(.adjustBearing(
          delta: -Double(deltaRadians * 180 / .pi),
          anchor: self.screenPoint(location)
        ))
        return true
      default:
        self.endGesture(recognizer, commands)
        return false
      }
    }
  }

  @objc private func handleShove(_ recognizer: UIPanGestureRecognizer) {
    enqueue { commands in
      switch recognizer.state {
      case .began:
        guard recognizer.numberOfTouches == 2 else { return false }
        commands.push(.cancelTransitions)
        self.beginGesture(recognizer, commands)
        recognizer.setTranslation(.zero, in: self)
        return false
      case .changed:
        guard recognizer.numberOfTouches == 2 else { return false }
        let translation = recognizer.translation(in: self)
        recognizer.setTranslation(.zero, in: self)
        guard translation.y != 0 else { return false }
        commands.push(.adjustPitch(delta: -Double(translation.y) * 0.1))
        return true
      default:
        self.endGesture(recognizer, commands)
        return false
      }
    }
  }

  @objc private func handleDoubleTap(_ recognizer: UITapGestureRecognizer) {
    enqueue { commands in
      commands.push(.cancelTransitions)
      let location = recognizer.location(in: self)
      commands.push(.zoomToNextStep(
        anchor: screenPoint(location),
        animation: MaplibreNativeFFI.AnimationOptions(durationMilliseconds: 160)
      ))
      return true
    }
  }

  private func screenPoint(_ point: CGPoint) -> ScreenPoint {
    ScreenPoint(x: point.x, y: point.y)
  }

  private func showError(_ error: Error) {
    log.error("\(String(describing: error), privacy: .public)")
    if subviews.contains(where: { $0 is UILabel }) {
      return
    }
    let label = UILabel()
    label.translatesAutoresizingMaskIntoConstraints = false
    label.text = String(describing: error)
    label.textAlignment = .center
    label.textColor = .white
    label.numberOfLines = 0
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
  }
}

extension MetalMapView: UIGestureRecognizerDelegate {
  func gestureRecognizer(
    _ gestureRecognizer: UIGestureRecognizer,
    shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer
  ) -> Bool {
    if gestureRecognizer is UIPinchGestureRecognizer,
       otherGestureRecognizer is UIRotationGestureRecognizer
    {
      return true
    }
    if gestureRecognizer is UIRotationGestureRecognizer,
       otherGestureRecognizer is UIPinchGestureRecognizer
    {
      return true
    }
    return false
  }
}
