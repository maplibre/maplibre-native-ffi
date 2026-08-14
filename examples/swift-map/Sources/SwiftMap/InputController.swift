import AppKit
import MaplibreNativeFFI

private let keyboardAnimationDurationMS = 160.0
private let resetAnimationDurationMS = 220.0
private let preciseScrollDeltaDivisor = 10.0
private let maxScrollDeltaPerEvent = 4.0

/// Decodes host input into camera commands on the render loop, converting to
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

  func mouseDown(_ event: NSEvent, submit: (CameraCommand) -> Void) -> Bool {
    beginDrag(
      .left,
      mode: event.modifierFlags.contains(.control) ? .rotate : .pan,
      at: event.locationInWindow,
      submit: submit
    )
    return false
  }

  func rightMouseDown(_ event: NSEvent,
                      submit: (CameraCommand) -> Void) -> Bool
  {
    beginDrag(
      .right,
      mode: .rotate,
      at: event.locationInWindow,
      submit: submit
    )
    return false
  }

  func mouseUp(_ event: NSEvent, submit: (CameraCommand) -> Void) -> Bool {
    endDrag(.left, at: event.locationInWindow, submit: submit)
    return false
  }

  func rightMouseUp(_ event: NSEvent, submit: (CameraCommand) -> Void) -> Bool {
    endDrag(.right, at: event.locationInWindow, submit: submit)
    return false
  }

  private func beginDrag(
    _ button: DragButton,
    mode: DragMode,
    at location: CGPoint,
    submit: (CameraCommand) -> Void
  ) {
    // A second button pressed during a live drag joins it, leaving the drag
    // baseline alone.
    guard dragMode == .none else { return }
    lastLocation = location
    dragMode = mode
    dragButton = button
    submit(.setGestureInProgress(true))
  }

  /// Ends the drag only for the button that started it, so the gesture bracket
  /// stays paired.
  private func endDrag(
    _ button: DragButton,
    at location: CGPoint,
    submit: (CameraCommand) -> Void
  ) {
    guard dragButton == button else { return }
    lastLocation = location
    dragMode = .none
    dragButton = .none
    submit(.setGestureInProgress(false))
  }

  func mouseDragged(_ event: NSEvent, submit: (CameraCommand) -> Void) -> Bool {
    let location = event.locationInWindow
    let dx = Double(location.x - lastLocation.x)
    let dy = Double(lastLocation.y - location.y)
    defer { lastLocation = location }

    switch dragMode {
    case .none:
      return false
    case .pan:
      if dx == 0 && dy == 0 { return false }
      submit(.moveBy(dx: dx, dy: dy))
    case .rotate:
      if dx == 0 && dy == 0 { return false }
      submit(.adjustBearing(delta: dx * 0.5))
      submit(.adjustPitch(delta: -dy * 0.5))
    }
    return true
  }

  func scrollWheel(_ event: NSEvent, submit: (CameraCommand) -> Void,
                   in view: NSView) -> Bool
  {
    let delta = scrollDelta(event)
    if delta == 0 { return false }

    let location = view.convert(event.locationInWindow, from: nil)
    let anchor = ScreenPoint(
      x: Double(location.x),
      y: Double(view.bounds.height - location.y)
    )
    let scale = pow(2.0, delta * 0.25)
    submit(.scaleBy(scale: scale, anchor: anchor))
    return true
  }

  func keyDown(_ event: NSEvent, submit: (CameraCommand) -> Void,
               viewport: Viewport) -> Bool
  {
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
      submit(.moveByAnimated(dx: panStep, dy: 0, animation: animation))
    case 124, 2:
      submit(.moveByAnimated(dx: -panStep, dy: 0, animation: animation))
    case 126, 13:
      submit(.moveByAnimated(dx: 0, dy: panStep, animation: animation))
    case 125, 1:
      submit(.moveByAnimated(dx: 0, dy: -panStep, animation: animation))
    case 24, 69:
      submit(.scaleByAnimated(
        scale: zoomStep,
        anchor: center,
        animation: animation
      ))
    case 27, 78:
      submit(.scaleByAnimated(
        scale: 1.0 / zoomStep,
        anchor: center,
        animation: animation
      ))
    case 12:
      submit(.adjustBearingAnimated(
        delta: -bearingStep,
        animation: animation
      ))
    case 14:
      submit(.adjustBearingAnimated(
        delta: bearingStep,
        animation: animation
      ))
    case 30:
      submit(.adjustPitchAnimated(
        delta: pitchStep,
        animation: animation
      ))
    case 33:
      submit(.adjustPitchAnimated(
        delta: -pitchStep,
        animation: animation
      ))
    case 29:
      submit(.resetOrientation(
        animation: AnimationOptions(
          durationMilliseconds: resetAnimationDurationMS
        )
      ))
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
