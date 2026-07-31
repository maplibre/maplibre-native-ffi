import AppKit
import MaplibreNative

private let keyboardAnimationDurationMS = 160.0
private let resetAnimationDurationMS = 220.0
private let preciseScrollDeltaDivisor = 10.0
private let maxScrollDeltaPerEvent = 4.0

/// Decodes host input into camera commands.
///
/// This runs on the render loop, which does not own the map, so it only
/// produces commands; the runtime loop applies them on the map's owner thread.
/// Anything needing the current viewport is converted to logical map
/// coordinates here, where the viewport lives.
///
/// Every handler returns whether the camera changed, so the render loop can set
/// the render request.
@MainActor
final class InputController {
  enum DragMode {
    case none
    case pan
    case rotate
  }

  /// The button that started the live drag. A drag belongs to one button, so a
  /// second button pressed during it neither restarts it nor ends it early.
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
    // A drag already owns the pointer, so a second button joins it rather than
    // starting a drag of its own. Its position leaves the live drag's baseline
    // alone, so the next delta still measures from where the owning button
    // last was.
    guard dragMode == .none else { return }
    lastLocation = location
    dragMode = mode
    dragButton = button
    // Queued ahead of the drag's own commands, so the transition stops before
    // the first delta lands.
    commands.push(.cancelTransitions)
    // The deltas that follow belong to one live gesture, so the map hears
    // about the gesture rather than a stream of unrelated camera commands.
    commands.push(.setGestureInProgress(true))
  }

  /// Every path that ends a drag runs through here, so the gesture mark the
  /// drag set is always paired with a clear. Releasing a button that joined the
  /// drag leaves it running, so the drag ends once, when the button that
  /// started it comes up.
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
