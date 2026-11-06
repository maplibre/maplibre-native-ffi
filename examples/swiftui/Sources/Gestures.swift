// Gestures, as camera deltas. On iOS, a drag pans, two fingers pinch, rotate,
// and pan, and a double tap zooms in. On macOS, a drag pans, a right or Control
// drag rotates and pitches, a trackpad scrolls, pinches, and rotates, a mouse
// wheel zooms, and a double click or a two-finger double tap zooms in, or out
// with Shift. Each gesture ends with its release velocities, so the map keeps
// moving until a touch or a click stops it.

import CoreGraphics
import MaplibreNativeFFI

#if os(iOS)
  import UIKit

  @MainActor
  final class TouchGestures: NSObject, UIGestureRecognizerDelegate {
    private unowned let view: UIView
    private let apply: (CameraDelta) -> Void
    // The gesture lasts while any of the view's touches or recognizers do.
    private var touches: Set<UITouch> = []
    private var recognizing = 0
    private var cancelled = false
    private var momentum = GestureMomentum()
    // The pinch's scale and rotation at the last event, and their centroid.
    private var scale = 1.0
    private var rotation = 0.0
    private var anchor: ScreenPoint?

    init(view: UIView, apply: @escaping (CameraDelta) -> Void) {
      self.view = view
      self.apply = apply
      super.init()
      let doubleTap = UITapGestureRecognizer(
        target: self,
        action: #selector(doubleTapped)
      )
      doubleTap.numberOfTapsRequired = 2
      for recognizer in [
        UIPanGestureRecognizer(target: self, action: #selector(panned)),
        UIPinchGestureRecognizer(target: self, action: #selector(pinched)),
        UIRotationGestureRecognizer(target: self, action: #selector(rotated)),
        doubleTap,
      ] {
        recognizer.delegate = self
        view.addGestureRecognizer(recognizer)
      }
    }

    /// A touch down starts a gesture, which stops the last one's momentum.
    func began(_ began: Set<UITouch>) {
      if touches.isEmpty, recognizing == 0 {
        cancelled = false
        momentum = GestureMomentum()
        apply(CameraDelta(gesturePhase: .begin))
      }
      touches.formUnion(began)
    }

    func ended(_ ended: Set<UITouch>) {
      touches.subtract(ended)
      finish()
    }

    func gestureRecognizer(
      _: UIGestureRecognizer,
      shouldRecognizeSimultaneouslyWith _: UIGestureRecognizer
    ) -> Bool {
      true
    }

    @objc private func panned(_ pan: UIPanGestureRecognizer) {
      let offset = pan.translation(in: view)
      pan.setTranslation(.zero, in: view)
      handle(pan, CameraDelta(offset: ScreenPoint(x: offset.x, y: offset.y))) {
        let velocity = pan.velocity(in: view)
        momentum.pan.velocity = ScreenPoint(x: velocity.x, y: velocity.y)
      }
    }

    @objc private func pinched(_ pinch: UIPinchGestureRecognizer) {
      if pinch.state == .began {
        scale = 1
      }
      let change = pinch.scale / scale
      scale = pinch.scale
      anchor = centroid(pinch)
      handle(pinch, CameraDelta(scale: change, anchor: anchor)) {
        // The velocity is in scale factor per second, and each doubling of
        // the scale is a zoom level.
        momentum.zoom = AnchoredMomentum(
          velocity: pinch.velocity / pinch.scale / log(2),
          anchor: anchor
        )
      }
    }

    @objc private func rotated(_ turn: UIRotationGestureRecognizer) {
      if turn.state == .began {
        rotation = 0
      }
      // The rotation runs clockwise, which turns the bearing the other way.
      let change = -(turn.rotation - rotation) * 180 / .pi
      rotation = turn.rotation
      anchor = centroid(turn)
      handle(turn, CameraDelta(bearing: change, anchor: anchor)) {
        momentum.bearing = AnchoredMomentum(
          velocity: -turn.velocity * 180 / .pi,
          anchor: anchor
        )
      }
    }

    @objc private func doubleTapped(_ tap: UITapGestureRecognizer) {
      let point = tap.location(in: view)
      apply(CameraDelta(
        scale: 2,
        anchor: ScreenPoint(x: point.x, y: point.y),
        animation: AnimationOptions(durationMs: 300)
      ))
    }

    /// Applies a recognizer's change, and keeps its velocity at release.
    private func handle(
      _ recognizer: UIGestureRecognizer,
      _ change: CameraDelta,
      release: () -> Void
    ) {
      var change = change
      change.gesturePhase = .update
      switch recognizer.state {
      case .began:
        recognizing += 1
        apply(change)
      case .changed:
        apply(change)
      case .ended:
        release()
        recognizing -= 1
        finish()
      case .cancelled:
        cancelled = true
        recognizing -= 1
        finish()
      default:
        break
      }
    }

    /// Ends the gesture once its touches and recognizers have all ended.
    private func finish() {
      guard touches.isEmpty, recognizing == 0 else { return }
      apply(cancelled
        ? CameraDelta(gesturePhase: .cancel)
        : CameraDelta(momentum: momentum, gesturePhase: .end))
    }

    /// Where a two-finger recognizer's touches meet, while it has both.
    private func centroid(_ recognizer: UIGestureRecognizer) -> ScreenPoint? {
      guard recognizer.numberOfTouches == 2 else { return anchor }
      let point = recognizer.location(in: view)
      return ScreenPoint(x: point.x, y: point.y)
    }
  }

#elseif os(macOS)
  import AppKit

  @MainActor
  final class PointerGestures {
    /// How far a right drag turns the bearing and pitch.
    private static let degreesPerPoint = 0.5

    private unowned let view: NSView
    private let apply: (CameraDelta) -> Void
    private var previous: CGPoint?
    private var rotating = false
    private var drag = VelocityTracker()
    private var scroll = VelocityTracker()
    private var magnification = VelocityTracker()
    private var rotation = VelocityTracker()

    init(view: NSView, apply: @escaping (CameraDelta) -> Void) {
      self.view = view
      self.apply = apply
    }

    func mouseDown(_ event: NSEvent, rotating: Bool) {
      previous = point(event)
      self.rotating = rotating
      drag = VelocityTracker()
      apply(CameraDelta(gesturePhase: .begin))
    }

    func mouseDragged(_ event: NSEvent) {
      guard let previous else { return }
      let current = point(event)
      self.previous = current
      let dx = current.x - previous.x
      let dy = current.y - previous.y
      drag.add(dx, dy, at: event.timestamp)
      if rotating {
        apply(CameraDelta(
          bearing: dx * Self.degreesPerPoint,
          pitch: -dy * Self.degreesPerPoint,
          gesturePhase: .update
        ))
      } else {
        apply(CameraDelta(
          offset: ScreenPoint(x: dx, y: dy),
          gesturePhase: .update
        ))
      }
    }

    func mouseUp(_ event: NSEvent) {
      guard previous != nil else { return }
      previous = nil
      let velocity = drag.velocity(at: event.timestamp)
      let turn = Self.degreesPerPoint
      let momentum = rotating
        ? GestureMomentum(
          bearing: AnchoredMomentum(velocity: velocity.x * turn),
          pitch: AnchoredMomentum(velocity: -velocity.y * turn)
        )
        : GestureMomentum(pan: PanMomentum(velocity: velocity))
      apply(CameraDelta(momentum: momentum, gesturePhase: .end))
      if event.clickCount == 2 {
        zoom(event, in: !event.modifierFlags.contains(.shift))
      }
    }

    func scrollWheel(_ event: NSEvent) {
      guard event.hasPreciseScrollingDeltas else {
        // A mouse wheel zooms about the pointer, by lines.
        let at = point(event)
        apply(CameraDelta(
          scale: pow(2, event.scrollingDeltaY * 40 / 300),
          anchor: ScreenPoint(x: at.x, y: at.y)
        ))
        return
      }
      // A trackpad or Magic Mouse scroll pans. The system's momentum events
      // after release are skipped, as the map makes its own.
      guard event.momentumPhase.isEmpty else { return }
      scroll.add(
        event.scrollingDeltaX,
        event.scrollingDeltaY,
        at: event.timestamp
      )
      let phase = phase(event.phase)
      apply(CameraDelta(
        offset: ScreenPoint(x: event.scrollingDeltaX, y: event.scrollingDeltaY),
        momentum: phase == .end
          ? GestureMomentum(pan: PanMomentum(
            velocity: scroll.velocity(at: event.timestamp)
          ))
          : nil,
        gesturePhase: phase
      ))
    }

    func magnify(_ event: NSEvent) {
      let at = point(event)
      let anchor = ScreenPoint(x: at.x, y: at.y)
      let scale = max(0.01, 1 + event.magnification)
      magnification.add(log2(scale), at: event.timestamp)
      let phase = phase(event.phase)
      apply(CameraDelta(
        scale: scale,
        anchor: anchor,
        momentum: phase == .end
          ? GestureMomentum(zoom: AnchoredMomentum(
            velocity: magnification.velocity(at: event.timestamp).x,
            anchor: anchor
          ))
          : nil,
        gesturePhase: phase
      ))
    }

    func rotate(_ event: NSEvent) {
      // The rotation is in degrees, counterclockwise, which turns the bearing
      // the same way.
      let at = point(event)
      let anchor = ScreenPoint(x: at.x, y: at.y)
      rotation.add(Double(event.rotation), at: event.timestamp)
      let phase = phase(event.phase)
      apply(CameraDelta(
        bearing: Double(event.rotation),
        anchor: anchor,
        momentum: phase == .end
          ? GestureMomentum(bearing: AnchoredMomentum(
            velocity: rotation.velocity(at: event.timestamp).x,
            anchor: anchor
          ))
          : nil,
        gesturePhase: phase
      ))
    }

    func smartMagnify(_ event: NSEvent) {
      zoom(event, in: true)
    }

    private func zoom(_ event: NSEvent, in zoomIn: Bool) {
      let at = point(event)
      apply(CameraDelta(
        scale: zoomIn ? 2 : 0.5,
        anchor: ScreenPoint(x: at.x, y: at.y),
        animation: AnimationOptions(durationMs: 300)
      ))
    }

    private func point(_ event: NSEvent) -> CGPoint {
      view.convert(event.locationInWindow, from: nil)
    }

    /// The gesture phase of a trackpad event. Fingers that rest on the
    /// trackpad may begin a scroll, which stops the last one's momentum.
    private func phase(_ phase: NSEvent.Phase) -> GesturePhase {
      switch phase {
      case .mayBegin, .began: .begin
      case .changed, .stationary: .update
      case .ended: .end
      case .cancelled: .cancel
      default: .none
      }
    }
  }

  /// Estimates how fast a gesture moved as it ended, from its recent events.
  private struct VelocityTracker {
    private static let window: TimeInterval = 0.1

    private var samples: [(time: TimeInterval, x: Double, y: Double)] = []

    mutating func add(_ dx: Double, _ dy: Double = 0, at time: TimeInterval) {
      let x = (samples.last?.x ?? 0) + dx
      let y = (samples.last?.y ?? 0) + dy
      samples.removeAll { time - $0.time > Self.window }
      samples.append((time, x, y))
    }

    /// The velocity, in units per second, over the window that ends at `time`.
    func velocity(at time: TimeInterval) -> ScreenPoint {
      guard let first = samples.first(where: { time - $0.time <= Self.window }),
            let last = samples.last, time > first.time
      else { return ScreenPoint(x: 0, y: 0) }
      return ScreenPoint(
        x: (last.x - first.x) / (time - first.time),
        y: (last.y - first.y) / (time - first.time)
      )
    }
  }
#endif
