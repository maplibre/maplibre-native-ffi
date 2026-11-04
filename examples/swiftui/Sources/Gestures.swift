// Gestures, as camera deltas. On iOS, a drag pans, two fingers pinch, rotate,
// and pan, and a double tap zooms in. On macOS, a drag pans, a right or Control
// drag rotates and pitches, a trackpad scrolls, pinches, and rotates, a mouse
// wheel zooms, and a double click or a two-finger double tap zooms in, or out
// with Shift.

import CoreGraphics
import MaplibreNativeFFI

#if os(iOS)
  import UIKit

  @MainActor
  final class TouchGestures {
    private static let tapSlop = 10.0

    private unowned let view: UIView
    private let apply: (CameraDelta) -> Void
    // The touches at the last event, in the order they began.
    private var touches: [(touch: UITouch, point: CGPoint)] = []
    private var start: CGPoint?
    private var moved = false

    init(view: UIView, apply: @escaping (CameraDelta) -> Void) {
      self.view = view
      self.apply = apply
    }

    func began(_ began: Set<UITouch>) {
      if touches.isEmpty {
        start = began.first.map { $0.location(in: view) }
        moved = false
        apply(CameraDelta(gesturePhase: .begin))
      }
      for touch in began {
        touches.append((touch, touch.location(in: view)))
      }
    }

    func moved(_: Set<UITouch>) {
      let pairs = touches.map { ($0.point, $0.touch.location(in: view)) }
      touches = touches.map { ($0.touch, $0.touch.location(in: view)) }
      guard let (before, after) = pairs.first else { return }
      if let start, distance(start, after) > Self.tapSlop {
        moved = true
      }

      guard pairs.count > 1 else {
        apply(CameraDelta(
          offset: ScreenPoint(x: after.x - before.x, y: after.y - before.y),
          gesturePhase: .update
        ))
        return
      }
      // The span between the touches scales and turns about their midpoint,
      // which moves with them.
      moved = true
      let (otherBefore, otherAfter) = pairs[1]
      let midBefore = midpoint(before, otherBefore)
      let midAfter = midpoint(after, otherAfter)
      let spanBefore = span(before, otherBefore)
      let spanAfter = span(after, otherAfter)
      let scale = length(spanAfter) / length(spanBefore)
      let turn = (angle(spanAfter) - angle(spanBefore)) * 180 / .pi
      apply(CameraDelta(
        offset: ScreenPoint(
          x: midAfter.x - midBefore.x,
          y: midAfter.y - midBefore.y
        ),
        scale: scale.isFinite && scale > 0 ? scale : 1,
        bearing: -wrapDegrees(turn),
        anchor: ScreenPoint(x: midAfter.x, y: midAfter.y),
        gesturePhase: .update
      ))
    }

    func ended(_ ended: Set<UITouch>, cancelled: Bool) {
      touches.removeAll { ended.contains($0.touch) }
      guard touches.isEmpty else { return }
      apply(CameraDelta(gesturePhase: cancelled ? .cancel : .end))
      if !cancelled, !moved, let touch = ended.first, touch.tapCount == 2 {
        let point = touch.location(in: view)
        apply(CameraDelta(
          scale: 2,
          anchor: ScreenPoint(x: point.x, y: point.y),
          animation: AnimationOptions(durationMs: 300)
        ))
      }
    }
  }

#elseif os(macOS)
  import AppKit

  @MainActor
  final class PointerGestures {
    private unowned let view: NSView
    private let apply: (CameraDelta) -> Void
    private var previous: CGPoint?
    private var rotating = false

    init(view: NSView, apply: @escaping (CameraDelta) -> Void) {
      self.view = view
      self.apply = apply
    }

    func mouseDown(_ event: NSEvent, rotating: Bool) {
      previous = point(event)
      self.rotating = rotating
      apply(CameraDelta(gesturePhase: .begin))
    }

    func mouseDragged(_ event: NSEvent) {
      guard let previous else { return }
      let current = point(event)
      self.previous = current
      let dx = current.x - previous.x
      let dy = current.y - previous.y
      if rotating {
        apply(CameraDelta(
          bearing: dx * 0.5,
          pitch: -dy * 0.5,
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
      apply(CameraDelta(gesturePhase: .end))
      if event.clickCount == 2 {
        zoom(event, in: !event.modifierFlags.contains(.shift))
      }
    }

    func scrollWheel(_ event: NSEvent) {
      if event.hasPreciseScrollingDeltas {
        // A trackpad or Magic Mouse scroll pans, and its momentum carries on
        // after the fingers lift.
        apply(CameraDelta(
          offset: ScreenPoint(
            x: event.scrollingDeltaX,
            y: event.scrollingDeltaY
          ),
          gesturePhase: phase(event.phase)
        ))
      } else {
        // A mouse wheel zooms about the pointer, by lines.
        let at = point(event)
        apply(CameraDelta(
          scale: pow(2, event.scrollingDeltaY * 40 / 300),
          anchor: ScreenPoint(x: at.x, y: at.y)
        ))
      }
    }

    func magnify(_ event: NSEvent) {
      let at = point(event)
      apply(CameraDelta(
        scale: max(0.01, 1 + event.magnification),
        anchor: ScreenPoint(x: at.x, y: at.y),
        gesturePhase: phase(event.phase)
      ))
    }

    func rotate(_ event: NSEvent) {
      // The rotation is in degrees, counterclockwise, which turns the bearing
      // the same way.
      let at = point(event)
      apply(CameraDelta(
        bearing: Double(event.rotation),
        anchor: ScreenPoint(x: at.x, y: at.y),
        gesturePhase: phase(event.phase)
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

    /// The gesture phase of a trackpad event. Momentum scrolling has none.
    private func phase(_ phase: NSEvent.Phase) -> GesturePhase {
      switch phase {
      case .began: .begin
      case .changed, .stationary: .update
      case .ended: .end
      case .cancelled: .cancel
      default: .none
      }
    }
  }
#endif

private func midpoint(_ a: CGPoint, _ b: CGPoint) -> CGPoint {
  CGPoint(x: (a.x + b.x) / 2, y: (a.y + b.y) / 2)
}

private func span(_ a: CGPoint, _ b: CGPoint) -> CGVector {
  CGVector(dx: b.x - a.x, dy: b.y - a.y)
}

private func length(_ v: CGVector) -> Double {
  hypot(v.dx, v.dy)
}

private func angle(_ v: CGVector) -> Double {
  atan2(v.dy, v.dx)
}

private func distance(_ a: CGPoint, _ b: CGPoint) -> Double {
  length(span(a, b))
}

/// An angle in degrees, wrapped to [-180, 180).
private func wrapDegrees(_ degrees: Double) -> Double {
  ((degrees + 180).truncatingRemainder(dividingBy: 360) + 360)
    .truncatingRemainder(dividingBy: 360) - 180
}
