import AppKit
import MaplibreNativeFFI

private let keyboardAnimationDurationMS = 160.0
private let resetAnimationDurationMS = 220.0
private let preciseScrollDeltaDivisor = 10.0
private let maxScrollDeltaPerEvent = 4.0

/// Decodes host input into camera updates on the render loop, converting to
/// logical map coordinates. Every handler returns whether the camera changed.
@MainActor
final class InputController {
  enum DragMode {
    case none
    case pan
    case rotate
  }

  private enum DragButton {
    case none
    case left
    case right
  }

  private var dragMode = DragMode.none
  private var dragButton = DragButton.none
  private var lastLocation = CGPoint.zero

  func mouseDown(_ event: NSEvent, mapState: MapState) throws -> Bool {
    try beginDrag(
      .left,
      mode: event.modifierFlags.contains(.control) ? .rotate : .pan,
      at: event.locationInWindow,
      mapState: mapState
    )
  }

  func rightMouseDown(_ event: NSEvent, mapState: MapState) throws -> Bool {
    try beginDrag(
      .right,
      mode: .rotate,
      at: event.locationInWindow,
      mapState: mapState
    )
  }

  func mouseUp(_ event: NSEvent, mapState: MapState) throws -> Bool {
    try endDrag(.left, at: event.locationInWindow, mapState: mapState)
  }

  func rightMouseUp(_ event: NSEvent, mapState: MapState) throws -> Bool {
    try endDrag(.right, at: event.locationInWindow, mapState: mapState)
  }

  private func beginDrag(
    _ button: DragButton,
    mode: DragMode,
    at location: CGPoint,
    mapState: MapState
  ) throws -> Bool {
    // A second button pressed during a live drag joins it, leaving the drag
    // baseline alone.
    guard dragMode == .none else { return false }
    lastLocation = location
    dragMode = mode
    dragButton = button
    try mapState.cancelTransitions()
    try mapState.setGestureInProgress(true)
    return true
  }

  /// Ends the drag only for the button that started it, so the gesture bracket
  /// stays paired.
  private func endDrag(
    _ button: DragButton,
    at location: CGPoint,
    mapState: MapState
  ) throws -> Bool {
    guard dragButton == button else { return false }
    lastLocation = location
    dragMode = .none
    dragButton = .none
    try mapState.setGestureInProgress(false)
    return true
  }

  func mouseDragged(_ event: NSEvent, mapState: MapState) throws -> Bool {
    let location = event.locationInWindow
    let dx = Double(location.x - lastLocation.x)
    let dy = Double(lastLocation.y - location.y)
    defer { lastLocation = location }

    switch dragMode {
    case .none:
      return false
    case .pan:
      if dx == 0 && dy == 0 { return false }
      try mapState.moveBy(dx: dx, dy: dy)
    case .rotate:
      if dx == 0 && dy == 0 { return false }
      try mapState.adjustBearing(delta: dx * 0.5)
      try mapState.adjustPitch(delta: dy * 0.5)
    }
    return true
  }

  func scrollWheel(
    _ event: NSEvent,
    in view: NSView,
    mapState: MapState
  ) throws -> Bool {
    let delta = scrollDelta(event)
    if delta == 0 { return false }

    let location = view.convert(event.locationInWindow, from: nil)
    let anchor = ScreenPoint(
      x: Double(location.x),
      y: Double(view.bounds.height - location.y)
    )
    let scale = pow(2.0, delta * 0.25)
    try mapState.scaleBy(scale, anchor: anchor)
    return true
  }

  func keyDown(
    _ event: NSEvent,
    viewport: Viewport,
    mapState: MapState
  ) throws -> Bool {
    let panStep = 120.0
    let zoomStep = 1.25
    let bearingStep = 10.0
    let pitchStep = 5.0
    let animation =
      AnimationOptions(durationMilliseconds: keyboardAnimationDurationMS)
    let center = ScreenPoint(
      x: Double(viewport.logicalWidth) / 2.0,
      y: Double(viewport.logicalHeight) / 2.0
    )

    switch event.keyCode {
    case 123, 0:
      try mapState.moveBy(dx: panStep, dy: 0, animation: animation)
    case 124, 2:
      try mapState.moveBy(dx: -panStep, dy: 0, animation: animation)
    case 126, 13:
      try mapState.moveBy(dx: 0, dy: panStep, animation: animation)
    case 125, 1:
      try mapState.moveBy(dx: 0, dy: -panStep, animation: animation)
    case 24, 69:
      try mapState.scaleBy(
        zoomStep,
        anchor: center,
        animation: animation
      )
    case 27, 78:
      try mapState.scaleBy(
        1.0 / zoomStep,
        anchor: center,
        animation: animation
      )
    case 12:
      try mapState.adjustBearing(
        delta: -bearingStep,
        animation: animation
      )
    case 14:
      try mapState.adjustBearing(
        delta: bearingStep,
        animation: animation
      )
    case 30:
      try mapState.adjustPitch(
        delta: pitchStep,
        animation: animation
      )
    case 33:
      try mapState.adjustPitch(
        delta: -pitchStep,
        animation: animation
      )
    case 29:
      try mapState.resetOrientation(
        animation: AnimationOptions(
          durationMilliseconds: resetAnimationDurationMS
        )
      )
    default:
      return false
    }
    return true
  }

  private func scrollDelta(_ event: NSEvent) -> Double {
    let rawDelta = Double(event.scrollingDeltaY)
    let wheelDelta = event
      .hasPreciseScrollingDeltas ? rawDelta / preciseScrollDeltaDivisor :
      rawDelta
    return min(max(wheelDelta, -maxScrollDeltaPerEvent), maxScrollDeltaPerEvent)
  }
}
