import MaplibreNative
import Metal
import os
import QuartzCore
import UIKit

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
  private var displayLink: CADisplayLink?
  private var currentViewport: Viewport?
  private var renderPending = true
  private var viewVisible = false
  private var appForeground = true

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
      do {
        try mapState?.close()
      } catch {
        log.error("\(String(describing: error), privacy: .public)")
      }
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
    do {
      try mapState?.close()
    } catch {
      showError(error)
    }
    mapState = nil
  }

  @objc private func displayLinkTick() {
    guard let mapState else { return }
    do {
      try mapState.runOnce()
      renderPending = try mapState.drainEvents() || renderPending
      guard let viewport = currentViewport, !viewport.isEmpty else { return }
      guard renderPending else { return }
      if try mapState.render() {
        renderPending = false
      }
    } catch {
      showError(error)
      stopHostLoop()
    }
  }

  private func refreshAndStartIfNeeded() {
    refreshViewport()
    if viewVisible, appForeground {
      renderPending = true
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

  private func refreshViewport() {
    guard let graphics else { return }
    let viewport = readViewport()
    guard viewport != currentViewport else { return }
    viewport
      .log(currentViewport == nil ? "initial viewport" : "resized viewport")
    if viewport.isEmpty {
      currentViewport = viewport
      renderPending = false
      return
    }

    do {
      graphics.resize(viewport)
      if let mapState {
        try mapState.resize(viewport, graphics: graphics)
      } else {
        mapState = try MapState(viewport: viewport, graphics: graphics)
        log.info("render target: native-surface")
        log
          .info(
            "render target status: renders directly to the host view surface"
          )
      }
      currentViewport = viewport
      renderPending = true
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

  private func withMap(_ action: (MapHandle) throws -> Bool) {
    guard let map = mapState?.map else { return }
    do {
      if try action(map) {
        renderPending = true
      }
    } catch {
      showError(error)
    }
  }

  @objc private func handlePan(_ recognizer: UIPanGestureRecognizer) {
    withMap { map in
      if recognizer.state == .began {
        try map.cancelTransitions()
        recognizer.setTranslation(.zero, in: self)
        return false
      }
      guard recognizer.state == .changed else { return false }
      let translation = recognizer.translation(in: self)
      recognizer.setTranslation(.zero, in: self)
      guard translation != .zero else { return false }
      try map.moveBy(deltaX: translation.x, deltaY: translation.y)
      return true
    }
  }

  @objc private func handlePinch(_ recognizer: UIPinchGestureRecognizer) {
    withMap { map in
      if recognizer.state == .began {
        try map.cancelTransitions()
        recognizer.scale = 1.0
        return false
      }
      guard recognizer.state == .changed else { return false }
      let scale = Double(recognizer.scale)
      recognizer.scale = 1.0
      guard scale.isFinite, scale > 0 else { return false }
      let location = recognizer.location(in: self)
      try map.scaleBy(scale, anchor: screenPoint(location))
      return true
    }
  }

  @objc private func handleRotation(_ recognizer: UIRotationGestureRecognizer) {
    withMap { map in
      if recognizer.state == .began {
        try map.cancelTransitions()
        recognizer.rotation = 0
        return false
      }
      guard recognizer.state == .changed else { return false }
      let deltaRadians = recognizer.rotation
      recognizer.rotation = 0
      guard deltaRadians != 0 else { return false }
      let location = recognizer.location(in: self)
      let camera = try map.camera()
      try map.jump(to: CameraOptions(
        bearing: (camera.bearing ?? 0) - Double(deltaRadians * 180 / .pi),
        anchor: screenPoint(location)
      ))
      return true
    }
  }

  @objc private func handleShove(_ recognizer: UIPanGestureRecognizer) {
    guard recognizer.numberOfTouches == 2 else { return }
    withMap { map in
      if recognizer.state == .began {
        try map.cancelTransitions()
        recognizer.setTranslation(.zero, in: self)
        return false
      }
      guard recognizer.state == .changed else { return false }
      let translation = recognizer.translation(in: self)
      recognizer.setTranslation(.zero, in: self)
      guard translation.y != 0 else { return false }
      let camera = try map.camera()
      let pitch = min(
        max((camera.pitch ?? 0) - Double(translation.y) * 0.1, 0),
        60
      )
      try map.jump(to: CameraOptions(pitch: pitch))
      return true
    }
  }

  @objc private func handleDoubleTap(_ recognizer: UITapGestureRecognizer) {
    withMap { map in
      try map.cancelTransitions()
      let location = recognizer.location(in: self)
      let camera = try map.camera()
      let zoom = camera.zoom ?? 0
      let targetZoom = round(zoom) + 1.0
      try map.scaleBy(
        pow(2.0, targetZoom - zoom),
        anchor: screenPoint(location),
        animation: MaplibreNative.AnimationOptions(
          durationMilliseconds: 160
        )
      )
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
