import AppKit
import MaplibreNativeFFI

private let keyboardAnimationDurationMS = 160.0
private let resetAnimationDurationMS = 220.0
private let preciseScrollDeltaDivisor = 10.0
private let maxScrollDeltaPerEvent = 4.0

/// Decodes host input into camera commands in logical map coordinates. The map
/// reports the resulting render update, which drives the next frame demand.
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

  func mouseDown(_ event: NSEvent, mapState: MapState) {
    beginDrag(
      .left,
      mode: event.modifierFlags.contains(.control) ? .rotate : .pan,
      at: event.locationInWindow,
      mapState: mapState
    )
  }

  func rightMouseDown(_ event: NSEvent,
                      mapState: MapState)
  {
    beginDrag(
      .right,
      mode: .rotate,
      at: event.locationInWindow,
      mapState: mapState
    )
  }

  func mouseUp(_ event: NSEvent, mapState: MapState) {
    endDrag(.left, at: event.locationInWindow, mapState: mapState)
  }

  func rightMouseUp(_ event: NSEvent, mapState: MapState) {
    endDrag(.right, at: event.locationInWindow, mapState: mapState)
  }

  private func beginDrag(
    _ button: DragButton,
    mode: DragMode,
    at location: CGPoint,
    mapState: MapState
  ) {
    // A second button pressed during a live drag joins it, leaving the drag
    // baseline alone.
    guard dragMode == .none else { return }
    lastLocation = location
    dragMode = mode
    dragButton = button
    mapState.cancelTransitions()
    mapState.setGestureInProgress(true)
  }

  /// Ends the drag only for the button that started it, so the gesture bracket
  /// stays paired.
  private func endDrag(
    _ button: DragButton,
    at location: CGPoint,
    mapState: MapState
  ) {
    guard dragButton == button else { return }
    lastLocation = location
    dragMode = .none
    dragButton = .none
    mapState.setGestureInProgress(false)
  }

  func mouseDragged(_ event: NSEvent, mapState: MapState) {
    let location = event.locationInWindow
    let dx = Double(location.x - lastLocation.x)
    let dy = Double(lastLocation.y - location.y)
    defer { lastLocation = location }

    switch dragMode {
    case .none:
      return
    case .pan:
      if dx == 0, dy == 0 { return }
      mapState.moveBy(dx: dx, dy: dy)
    case .rotate:
      if dx == 0, dy == 0 { return }
      mapState.adjustBearing(delta: dx * 0.5)
      mapState.adjustPitch(delta: dy * 0.5)
    }
  }

  func scrollWheel(
    _ event: NSEvent,
    in view: NSView,
    mapState: MapState
  ) {
    let delta = scrollDelta(event)
    if delta == 0 { return }

    let location = view.convert(event.locationInWindow, from: nil)
    let anchor = ScreenPoint(
      x: Double(location.x),
      y: Double(view.bounds.height - location.y)
    )
    let scale = pow(2.0, delta * 0.25)
    mapState.scaleBy(scale, anchor: anchor)
  }

  func keyDown(
    _ event: NSEvent,
    viewport: Viewport,
    mapState: MapState
  ) {
    let panStep = 120.0
    let zoomStep = 1.25
    let bearingStep = 10.0
    let pitchStep = 5.0
    let animation =
      AnimationOptions(durationMs: keyboardAnimationDurationMS)
    let center = ScreenPoint(
      x: Double(viewport.logicalWidth) / 2.0,
      y: Double(viewport.logicalHeight) / 2.0
    )

    switch event.keyCode {
    case 123, 0:
      mapState.moveBy(dx: panStep, dy: 0, animation: animation)
    case 124, 2:
      mapState.moveBy(dx: -panStep, dy: 0, animation: animation)
    case 126, 13:
      mapState.moveBy(dx: 0, dy: panStep, animation: animation)
    case 125, 1:
      mapState.moveBy(dx: 0, dy: -panStep, animation: animation)
    case 24, 69:
      mapState.scaleBy(
        zoomStep,
        anchor: center,
        animation: animation
      )
    case 27, 78:
      mapState.scaleBy(
        1.0 / zoomStep,
        anchor: center,
        animation: animation
      )
    case 12:
      mapState.adjustBearing(
        delta: -bearingStep,
        animation: animation
      )
    case 14:
      mapState.adjustBearing(
        delta: bearingStep,
        animation: animation
      )
    case 30:
      mapState.adjustPitch(
        delta: pitchStep,
        animation: animation
      )
    case 33:
      mapState.adjustPitch(
        delta: -pitchStep,
        animation: animation
      )
    case 29:
      mapState.resetOrientation(
        animation: AnimationOptions(
          durationMs: resetAnimationDurationMS
        )
      )
    default:
      break
    }
  }

  private func scrollDelta(_ event: NSEvent) -> Double {
    let rawDelta = Double(event.scrollingDeltaY)
    let wheelDelta = event
      .hasPreciseScrollingDeltas ? rawDelta / preciseScrollDeltaDivisor :
      rawDelta
    return min(max(wheelDelta, -maxScrollDeltaPerEvent), maxScrollDeltaPerEvent)
  }
}
