import AppKit
import MaplibreNativeFFI

private let keyboardAnimationDurationMS = 160.0
private let resetAnimationDurationMS = 220.0
private let preciseScrollDeltaDivisor = 10.0
private let maxScrollDeltaPerEvent = 4.0

/// Decodes host input into camera commands on the render loop, converting to
/// logical map coordinates. The runtime loop applies the commands. Every
/// handler returns whether the camera changed.
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

  func mouseDown(_ event: NSEvent, commands: Channels) -> Bool {
    beginDrag(
      .left,
      mode: event.modifierFlags.contains(.control) ? .rotate : .pan,
      at: event.locationInWindow,
      commands: commands
    )
    return false
  }

  func rightMouseDown(_ event: NSEvent, commands: Channels) -> Bool {
    beginDrag(
      .right,
      mode: .rotate,
      at: event.locationInWindow,
      commands: commands
    )
    return false
  }

  func mouseUp(_ event: NSEvent, commands: Channels) -> Bool {
    endDrag(.left, at: event.locationInWindow, commands: commands)
    return false
  }

  func rightMouseUp(_ event: NSEvent, commands: Channels) -> Bool {
    endDrag(.right, at: event.locationInWindow, commands: commands)
    return false
  }

  private func beginDrag(
    _ button: DragButton,
    mode: DragMode,
    at location: CGPoint,
    commands: Channels
  ) {
    // A second button pressed during a live drag joins it, leaving the drag
    // baseline alone.
    guard dragMode == .none else { return }
    lastLocation = location
    dragMode = mode
    dragButton = button
    // Cancel first, so the running transition stops before the first delta.
    commands.push(.cancelTransitions)
    commands.push(.setGestureInProgress(true))
  }

  /// Ends the drag only for the button that started it, so the gesture bracket
  /// stays paired.
  private func endDrag(
    _ button: DragButton,
    at location: CGPoint,
    commands: Channels
  ) {
    guard dragButton == button else { return }
    lastLocation = location
    dragMode = .none
    dragButton = .none
    commands.push(.setGestureInProgress(false))
  }

  func mouseDragged(_ event: NSEvent, commands: Channels) -> Bool {
    let location = event.locationInWindow
    let dx = Double(location.x - lastLocation.x)
    let dy = Double(lastLocation.y - location.y)
    defer { lastLocation = location }

    switch dragMode {
    case .none:
      return false
    case .pan:
      if dx == 0 && dy == 0 { return false }
      commands.push(.moveBy(dx: dx, dy: dy))
    case .rotate:
      if dx == 0 && dy == 0 { return false }
      commands.push(.adjustBearing(delta: dx * 0.5))
      commands.push(.adjustPitch(delta: -dy * 0.5))
    }
    return true
  }

  func scrollWheel(_ event: NSEvent, commands: Channels,
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
    commands.push(.scaleBy(scale: scale, anchor: anchor))
    return true
  }

  func keyDown(_ event: NSEvent, commands: Channels,
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
      commands.push(.moveByAnimated(dx: panStep, dy: 0, animation: animation))
    case 124, 2:
      commands.push(.moveByAnimated(dx: -panStep, dy: 0, animation: animation))
    case 126, 13:
      commands.push(.moveByAnimated(dx: 0, dy: panStep, animation: animation))
    case 125, 1:
      commands.push(.moveByAnimated(dx: 0, dy: -panStep, animation: animation))
    case 24, 69:
      commands.push(.scaleByAnimated(
        scale: zoomStep,
        anchor: center,
        animation: animation
      ))
    case 27, 78:
      commands.push(.scaleByAnimated(
        scale: 1.0 / zoomStep,
        anchor: center,
        animation: animation
      ))
    case 12:
      commands
        .push(.adjustBearingAnimated(
          delta: -bearingStep,
          animation: animation
        ))
    case 14:
      commands
        .push(.adjustBearingAnimated(delta: bearingStep, animation: animation))
    case 30:
      commands
        .push(.adjustPitchAnimated(delta: pitchStep, animation: animation))
    case 33:
      commands
        .push(.adjustPitchAnimated(delta: -pitchStep, animation: animation))
    case 29:
      commands.push(.resetOrientation(
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
