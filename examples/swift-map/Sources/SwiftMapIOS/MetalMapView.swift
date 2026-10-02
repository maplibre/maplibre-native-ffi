import MaplibreNativeFFI
import Metal
import os
import QuartzCore
import UIKit

/// The map view. It owns the layer, gesture decoding, the Metal objects, and
/// the render loop, which owns the map and the render session, on the main
/// thread.
@MainActor
final class MetalMapView: UIView {
  static let willTerminateMapViews = Notification
    .Name("SwiftMapIOSWillTerminateMapViews")
  private let log = Logger(
    subsystem: "org.maplibre.nativeffi.examples.swift-map-ios",
    category: "MapView"
  )
  private var graphics: MetalGraphicsContext?
  private var loop: RenderLoop?
  private var startup: Task<Void, Never>?
  private var shutdownTask: Task<Void, Never>?
  private var currentViewport: Viewport?
  private var viewVisible = false
  private var appForeground = true

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

  /// Last resort only: a view released without leaving its window abandons the
  /// session rather than closing it, because deinit cannot await teardown.
  /// ``didMoveToWindow`` is the ordered path.
  deinit {
    NotificationCenter.default.removeObserver(self)
    MainActor.assumeIsolated {
      loop?.abandon()
    }
  }

  override func didMoveToWindow() {
    super.didMoveToWindow()
    viewVisible = window != nil
    if viewVisible {
      updatePresenting()
      refreshViewport()
    } else {
      // Leaving the window ends this view's map: release the session, then the
      // map, then the runtime.
      beginTeardown()
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    refreshViewport()
  }

  @objc private func enterForeground() {
    appForeground = true
    updatePresenting()
    refreshViewport()
  }

  @objc private func enterBackground() {
    appForeground = false
    updatePresenting()
    // The app may not render once it is suspended, so a background task keeps
    // it running until the frames demanded before the pause have their
    // results. The transition itself returns at once, as UIKit expects.
    guard let loop else { return }
    let task = BackgroundTask()
    task.begin()
    Task { @MainActor in
      await loop.awaitRenderBarrier()
      task.end()
    }
  }

  @objc private func closeMap() {
    beginTeardown()
  }

  /// Demands frames only while the view is visible in the foreground. The
  /// native scheduler keeps loading.
  private func updatePresenting() {
    loop?.isPresenting = viewVisible && appForeground
  }

  /// Closes the render loop once startup settles.
  private func beginTeardown() {
    guard shutdownTask == nil else { return }
    let startup = startup
    shutdownTask = Task { @MainActor in
      await startup?.value
      do {
        try await self.loop?.close()
      } catch {
        self.log.error("\(String(describing: error), privacy: .public)")
      }
      self.loop = nil
    }
  }

  /// Starts the loop once a non-empty viewport is known, because the map takes
  /// its initial extent from it.
  private func startIfNeeded(viewport: Viewport) {
    guard startup == nil, shutdownTask == nil, let graphics else { return }
    startup = Task { @MainActor in
      do {
        let loop = try await RenderLoop.start(
          graphics: graphics,
          viewport: viewport
        )
        loop.onFailure = { [weak self] in self?.fail($0) }
        self.loop = loop
        updatePresenting()
        log.info("render target: native-surface")
        log.info(
          "render target status: renders directly to the host window surface"
        )
        log.info(
          "render driver: \(loop.driver == .coreWorker ? "core-worker" : "caller-graphics-thread", privacy: .public)"
        )
        // The viewport can change while startup is in flight.
        if let latest = currentViewport, latest != viewport, !latest.isEmpty {
          loop.resize(latest)
        }
      } catch {
        fail(error)
      }
    }
  }

  private func refreshViewport() {
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

  private func fail(_ error: Error) {
    showError(error)
    beginTeardown()
  }

  private func readViewport() -> Viewport {
    let displayScale = traitCollection.displayScale
    let scale = displayScale > 0 ? displayScale : UIScreen.main.scale
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

  /// The map that gestures drive, once the loop is running.
  private var mapState: MapState? {
    shutdownTask == nil ? loop?.mapState : nil
  }

  /// Opens the gesture bracket for the first recognizer to begin.
  private func beginGesture(_ recognizer: UIGestureRecognizer) {
    if openGestures.isEmpty {
      mapState?.cancelTransitions()
      mapState?.setGestureInProgress(true)
    }
    openGestures.insert(ObjectIdentifier(recognizer))
  }

  /// Closes the bracket once the last recognizer ends or is cancelled, so each
  /// open is paired with a close.
  private func endGesture(_ recognizer: UIGestureRecognizer) {
    guard openGestures.remove(ObjectIdentifier(recognizer)) != nil else {
      return
    }
    if openGestures.isEmpty {
      mapState?.setGestureInProgress(false)
    }
  }

  @objc private func handlePan(_ recognizer: UIPanGestureRecognizer) {
    switch recognizer.state {
    case .began:
      beginGesture(recognizer)
      recognizer.setTranslation(.zero, in: self)
    case .changed:
      let translation = recognizer.translation(in: self)
      recognizer.setTranslation(.zero, in: self)
      guard translation != .zero else { return }
      mapState?.moveBy(dx: Double(translation.x), dy: Double(translation.y))
    default:
      endGesture(recognizer)
    }
  }

  @objc private func handlePinch(_ recognizer: UIPinchGestureRecognizer) {
    switch recognizer.state {
    case .began:
      beginGesture(recognizer)
      recognizer.scale = 1.0
    case .changed:
      let scale = Double(recognizer.scale)
      recognizer.scale = 1.0
      guard scale.isFinite, scale > 0 else { return }
      let anchor = screenPoint(recognizer.location(in: self))
      mapState?.scaleBy(scale, anchor: anchor)
    default:
      endGesture(recognizer)
    }
  }

  @objc private func handleRotation(_ recognizer: UIRotationGestureRecognizer) {
    switch recognizer.state {
    case .began:
      beginGesture(recognizer)
      recognizer.rotation = 0
    case .changed:
      let deltaRadians = recognizer.rotation
      recognizer.rotation = 0
      guard deltaRadians != 0 else { return }
      let anchor = screenPoint(recognizer.location(in: self))
      mapState?.adjustBearing(
        delta: -Double(deltaRadians * 180 / .pi),
        anchor: anchor
      )
    default:
      endGesture(recognizer)
    }
  }

  @objc private func handleShove(_ recognizer: UIPanGestureRecognizer) {
    switch recognizer.state {
    case .began:
      guard recognizer.numberOfTouches == 2 else { return }
      beginGesture(recognizer)
      recognizer.setTranslation(.zero, in: self)
    case .changed:
      guard recognizer.numberOfTouches == 2 else { return }
      let translation = recognizer.translation(in: self)
      recognizer.setTranslation(.zero, in: self)
      guard translation.y != 0 else { return }
      mapState?.adjustPitch(delta: -Double(translation.y) * 0.1)
    default:
      endGesture(recognizer)
    }
  }

  @objc private func handleDoubleTap(_ recognizer: UITapGestureRecognizer) {
    let anchor = screenPoint(recognizer.location(in: self))
    mapState?.zoomToNextStep(
      anchor: anchor,
      animation: MaplibreNativeFFI.AnimationOptions(durationMs: 160)
    )
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

/// A UIKit background task that ends once, when its work finishes or when
/// UIKit expires it.
@MainActor
private final class BackgroundTask {
  private var identifier = UIBackgroundTaskIdentifier.invalid

  func begin() {
    identifier = UIApplication.shared.beginBackgroundTask { [self] in
      MainActor.assumeIsolated { end() }
    }
  }

  func end() {
    guard identifier != .invalid else { return }
    UIApplication.shared.endBackgroundTask(identifier)
    identifier = .invalid
  }
}
