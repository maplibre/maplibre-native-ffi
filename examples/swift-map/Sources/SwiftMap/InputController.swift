import AppKit
import MaplibreNative

private let keyboardAnimationDurationMS = 160.0
private let resetAnimationDurationMS = 220.0
private let preciseScrollDeltaPerWheelStep = 10.0

@MainActor
final class InputController {
  enum DragMode {
    case none
    case pan
    case rotate
  }

  private var dragMode = DragMode.none
  private var lastLocation = CGPoint.zero

  func mouseDown(_ event: NSEvent, map: MapHandle) throws -> Bool {
    lastLocation = event.locationInWindow
    dragMode = event.modifierFlags.contains(.control) ? .rotate : .pan
    try map.cancelTransitions()
    return true
  }

  func rightMouseDown(_ event: NSEvent, map: MapHandle) throws -> Bool {
    lastLocation = event.locationInWindow
    dragMode = .rotate
    try map.cancelTransitions()
    return true
  }

  func mouseUp(_ event: NSEvent) -> Bool {
    lastLocation = event.locationInWindow
    dragMode = .none
    return true
  }

  func mouseDragged(_ event: NSEvent, map: MapHandle) throws -> Bool {
    let location = event.locationInWindow
    let dx = Double(location.x - lastLocation.x)
    let dy = Double(lastLocation.y - location.y)
    defer { lastLocation = location }

    switch dragMode {
    case .none:
      return false
    case .pan:
      if dx == 0 && dy == 0 { return true }
      try map.moveBy(deltaX: dx, deltaY: dy)
    case .rotate:
      if dx == 0 && dy == 0 { return true }
      try adjustBearing(map, dx * 0.5)
      try adjustPitch(map, -dy / 2.0)
    }
    return true
  }

  func scrollWheel(_ event: NSEvent, map: MapHandle, in view: NSView) throws -> Bool {
    let rawDelta = -Double(event.scrollingDeltaY)
    let delta = event.hasPreciseScrollingDeltas ? rawDelta / preciseScrollDeltaPerWheelStep : rawDelta
    if delta == 0 { return true }

    let location = view.convert(event.locationInWindow, from: nil)
    let anchor = ScreenPoint(x: Double(location.x), y: Double(view.bounds.height - location.y))
    let scale = pow(2.0, delta * 0.25)
    try map.scaleBy(scale, anchor: anchor)
    return true
  }

  func keyDown(_ event: NSEvent, map: MapHandle, viewport: Viewport) throws -> Bool {
    let panStep = 120.0
    let zoomStep = 1.25
    let bearingStep = 10.0
    let pitchStep = 5.0
    let animation = AnimationOptions(durationMilliseconds: keyboardAnimationDurationMS)
    let center = ScreenPoint(
      x: Double(viewport.logicalWidth) / 2.0,
      y: Double(viewport.logicalHeight) / 2.0
    )

    switch event.keyCode {
    case 123, 0:
      try map.moveBy(deltaX: panStep, deltaY: 0, animation: animation)
    case 124, 2:
      try map.moveBy(deltaX: -panStep, deltaY: 0, animation: animation)
    case 126, 13:
      try map.moveBy(deltaX: 0, deltaY: panStep, animation: animation)
    case 125, 1:
      try map.moveBy(deltaX: 0, deltaY: -panStep, animation: animation)
    case 24, 69:
      try map.scaleBy(zoomStep, anchor: center, animation: animation)
    case 27, 78:
      try map.scaleBy(1.0 / zoomStep, anchor: center, animation: animation)
    case 12:
      try adjustBearingAnimated(map, -bearingStep, animation: animation)
    case 14:
      try adjustBearingAnimated(map, bearingStep, animation: animation)
    case 116, 30:
      try adjustPitchAnimated(map, pitchStep, animation: animation)
    case 121, 33:
      try adjustPitchAnimated(map, -pitchStep, animation: animation)
    case 29:
      try resetPitchAndBearingAnimated(map, animation: AnimationOptions(durationMilliseconds: resetAnimationDurationMS))
    default:
      return false
    }
    return true
  }

  private func adjustBearing(_ map: MapHandle, _ delta: Double) throws {
    let current = try map.camera()
    try map.jump(to: CameraOptions(bearing: (current.bearing ?? 0) + delta))
  }

  private func adjustBearingAnimated(_ map: MapHandle, _ delta: Double, animation: AnimationOptions) throws {
    let current = try map.camera()
    try map.ease(to: CameraOptions(bearing: (current.bearing ?? 0) + delta), animation: animation)
  }

  private func adjustPitch(_ map: MapHandle, _ delta: Double) throws {
    let current = try map.camera()
    try map.jump(to: CameraOptions(pitch: min(max((current.pitch ?? 0) + delta, 0.0), 60.0)))
  }

  private func adjustPitchAnimated(_ map: MapHandle, _ delta: Double, animation: AnimationOptions) throws {
    let current = try map.camera()
    try map.ease(to: CameraOptions(pitch: min(max((current.pitch ?? 0) + delta, 0.0), 60.0)), animation: animation)
  }

  private func resetPitchAndBearingAnimated(_ map: MapHandle, animation: AnimationOptions) throws {
    try map.ease(to: CameraOptions(bearing: 0, pitch: 0), animation: animation)
  }
}
