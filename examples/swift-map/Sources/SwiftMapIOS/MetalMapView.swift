import MaplibreNativeFFI
import Metal
import os
import QuartzCore
import UIKit

/// The display-paced render loop. This view owns the layer, gesture decoding,
/// runtime, map, Metal objects, and render session on the main thread.
@MainActor
final class MetalMapView: UIView {
  static let willTerminateMapViews = Notification
    .Name("SwiftMapIOSWillTerminateMapViews")
  private let log = Logger(
    subsystem: "org.maplibre.nativeffi.examples.swift-map-ios",
    category: "MapView"
  )
  private var graphics: MetalGraphicsContext?
  private var mapState: MapState?
  private var renderTarget: MetalRenderTarget?
  private var setupTask: Task<Void, Never>?
  private var cameraTask: Task<Void, Never>?
  private var displayLink: CADisplayLink?
  private var frameTask: Task<Void, Never>?
  private var shutdownTask: Task<Void, Never>?
  private var currentViewport: Viewport?
  private var renderRequested = true
  private var nextCameraSequence = 0
  private var didLogStartupStatus = false
  private var viewVisible = false
  private var appForeground = true
  private var isShutDown = false
  private var pendingResize = false

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
      renderTarget?.abandon()
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
    beginTeardown()
  }

  /// Closes the session before closing the map; a map with an
  /// attached session cannot be destroyed.
  private func beginTeardown() {
    guard !isShutDown else { return }
    isShutDown = true
    stopHostLoop()
    if frameTask == nil {
      finishTeardown()
    }
  }

  private func finishTeardown() {
    guard shutdownTask == nil else { return }
    let target = renderTarget
    renderTarget = nil
    let setup = setupTask
    let camera = cameraTask
    shutdownTask = Task { @MainActor in
      await setup?.value
      await camera?.value
      do {
        try await target?.close()
        try await self.mapState?.close()
      } catch {
        self.log.error("\(String(describing: error), privacy: .public)")
      }
      self.mapState = nil
    }
  }

  @objc private func displayLinkTick() {
    guard frameTask == nil, !isShutDown else { return }
    frameTask = Task { @MainActor in
      await renderDisplayFrame()
      frameTask = nil
      if isShutDown {
        finishTeardown()
      }
    }
  }

  private func renderDisplayFrame() async {
    guard !isShutDown else { return }
    await attachIfNeeded()
    guard !isShutDown,
          let renderTarget,
          let viewport = currentViewport,
          !viewport.isEmpty
    else { return }

    do {
      if pendingResize {
        try await renderTarget.resize(viewport)
        pendingResize = false
      }
      if renderRequested {
        renderRequested = false
        let rendered = try renderTarget.renderFrame()
        if !rendered {
          renderRequested = true
        }
      }
    } catch {
      showError(error)
      beginTeardown()
    }
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
        map: renderMap,
        graphics: graphics,
        viewport: viewport
      )
      pendingResize = false
      if !didLogStartupStatus {
        log.info("render target: native-surface")
        log.info(
          "render target status: renders directly to the host view surface"
        )
        didLogStartupStatus = true
      }
      renderRequested = true
    } catch {
      showError(error)
      beginTeardown()
    }
  }

  private func refreshAndStartIfNeeded() {
    guard !isShutDown else { return }
    refreshViewport()
    if viewVisible, appForeground {
      renderRequested = true
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
            try await state.apply(.resize(MapLogicalExtent(
              width: latest.logicalWidth,
              height: latest.logicalHeight,
              scaleFactor: latest.scaleFactor
            )))
          }
          self.mapState = state
          state.scheduleEventDrains(
            onRenderRequested: { [weak self] in
              self?.renderRequested = true
            },
            onFailure: { [weak self] error in
              self?.showError(error)
              self?.beginTeardown()
            }
          )
          self.renderRequested = true
        }
      } catch {
        setupFailure = error
      }
      self.setupTask = nil
      if let setupFailure {
        self.showError(setupFailure)
        self.beginTeardown()
      }
    }
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

    graphics.resize(viewport)
    currentViewport = viewport
    pendingResize = renderTarget != nil
    if mapState != nil {
      submit(.resize(MapLogicalExtent(
        width: viewport.logicalWidth,
        height: viewport.logicalHeight,
        scaleFactor: viewport.scaleFactor
      )))
    }
    renderRequested = true
    startMapStateIfNeeded(viewport: viewport)
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

  /// Serializes camera queries without moving map ownership off the render
  /// loop. Each task inherits the main actor and waits for its predecessor.
  private func submit(_ command: CameraCommand) {
    guard !isShutDown else { return }
    renderRequested = true
    nextCameraSequence += 1
    let sequence = nextCameraSequence
    let predecessor = cameraTask
    cameraTask = Task { @MainActor [weak self] in
      await predecessor?.value
      guard let self, !self.isShutDown,
            let state = self.mapState else { return }
      var commandError: Error?
      do {
        try await state.apply(command)
      } catch {
        commandError = error
      }
      if self.nextCameraSequence == sequence {
        self.cameraTask = nil
      }
      if let commandError {
        self.showError(commandError)
        self.beginTeardown()
      }
    }
  }

  /// Decodes a gesture and requests a render when the decoded gesture changed
  /// the camera.
  private func submitGesture(_ decode: ((CameraCommand) -> Void) -> Bool) {
    if decode(submit) {
      renderRequested = true
    }
  }

  /// Opens the gesture bracket for the first recognizer to begin.
  private func beginGesture(
    _ recognizer: UIGestureRecognizer,
    _ submit: (CameraCommand) -> Void
  ) {
    if openGestures.isEmpty {
      submit(.setGestureInProgress(true))
    }
    openGestures.insert(ObjectIdentifier(recognizer))
  }

  /// Closes the bracket once the last recognizer ends or is cancelled, so each
  /// open is paired with a close.
  private func endGesture(
    _ recognizer: UIGestureRecognizer,
    _ submit: (CameraCommand) -> Void
  ) {
    guard openGestures.remove(ObjectIdentifier(recognizer)) != nil else {
      return
    }
    if openGestures.isEmpty {
      submit(.setGestureInProgress(false))
    }
  }

  @objc private func handlePan(_ recognizer: UIPanGestureRecognizer) {
    submitGesture { submit in
      switch recognizer.state {
      case .began:
        self.beginGesture(recognizer, submit)
        recognizer.setTranslation(.zero, in: self)
        return false
      case .changed:
        let translation = recognizer.translation(in: self)
        recognizer.setTranslation(.zero, in: self)
        guard translation != .zero else { return false }
        submit(.moveBy(
          dx: Double(translation.x),
          dy: Double(translation.y)
        ))
        return true
      default:
        self.endGesture(recognizer, submit)
        return false
      }
    }
  }

  @objc private func handlePinch(_ recognizer: UIPinchGestureRecognizer) {
    submitGesture { submit in
      switch recognizer.state {
      case .began:
        self.beginGesture(recognizer, submit)
        recognizer.scale = 1.0
        return false
      case .changed:
        let scale = Double(recognizer.scale)
        recognizer.scale = 1.0
        guard scale.isFinite, scale > 0 else { return false }
        let location = recognizer.location(in: self)
        submit(.scaleBy(
          scale: scale,
          anchor: self.screenPoint(location)
        ))
        return true
      default:
        self.endGesture(recognizer, submit)
        return false
      }
    }
  }

  @objc private func handleRotation(_ recognizer: UIRotationGestureRecognizer) {
    submitGesture { submit in
      switch recognizer.state {
      case .began:
        self.beginGesture(recognizer, submit)
        recognizer.rotation = 0
        return false
      case .changed:
        let deltaRadians = recognizer.rotation
        recognizer.rotation = 0
        guard deltaRadians != 0 else { return false }
        let location = recognizer.location(in: self)
        submit(.adjustBearing(
          delta: -Double(deltaRadians * 180 / .pi),
          anchor: self.screenPoint(location)
        ))
        return true
      default:
        self.endGesture(recognizer, submit)
        return false
      }
    }
  }

  @objc private func handleShove(_ recognizer: UIPanGestureRecognizer) {
    submitGesture { submit in
      switch recognizer.state {
      case .began:
        guard recognizer.numberOfTouches == 2 else { return false }
        self.beginGesture(recognizer, submit)
        recognizer.setTranslation(.zero, in: self)
        return false
      case .changed:
        guard recognizer.numberOfTouches == 2 else { return false }
        let translation = recognizer.translation(in: self)
        recognizer.setTranslation(.zero, in: self)
        guard translation.y != 0 else { return false }
        submit(.adjustPitch(delta: -Double(translation.y) * 0.1))
        return true
      default:
        self.endGesture(recognizer, submit)
        return false
      }
    }
  }

  @objc private func handleDoubleTap(_ recognizer: UITapGestureRecognizer) {
    submitGesture { submit in
      let location = recognizer.location(in: self)
      submit(.zoomToNextStep(
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
