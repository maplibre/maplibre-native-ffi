// Gestures. The map's gesture recognizer turns the view's raw input into camera
// movement: a drag pans, two fingers pinch, twist, and tilt, a double tap zooms
// in, a two-finger tap zooms out, and a flick glides on. On macOS, a right or
// Control drag rotates and pitches, a trackpad pans, pinches, and rotates, a
// mouse wheel zooms, a double click zooms in, or out with Shift, the arrow keys
// pan, and plus and minus zoom. A notch, such as north on the bearing, plays a
// haptic tick. This file translates the platform's events, and ticks the
// recognizer when it asks.

import MaplibreNativeFFI
import QuartzCore

#if os(iOS)
  import UIKit
#elseif os(macOS)
  import AppKit
#endif

@MainActor
final class MapGestures: NSObject {
  private weak var view: MetalView?
  private let recognizer: () -> GestureRecognizerHandle?
  /// Ticks the recognizer each frame, while it asks.
  private var link: CADisplayLink?
  /// Ticks the recognizer once, at the time it asks for.
  private var timer: Timer?
  /// The last time that the recognizer took, in milliseconds since boot.
  private var lastMs = 0.0

  init(
    view: MetalView,
    recognizer: @escaping () -> GestureRecognizerHandle?
  ) {
    self.view = view
    self.recognizer = recognizer
  }

  /// Stops ticking, once the view leaves its window.
  func stop() {
    link?.invalidate()
    link = nil
    timer?.invalidate()
    timer = nil
  }

  /// Passes one input to the recognizer at `time`, in seconds since boot, and
  /// returns whether it used the input.
  @discardableResult
  private func handle(_ input: GestureInput, at time: TimeInterval) -> Bool {
    guard let result = try? recognizer()?.handle(
      input: input,
      timeMs: milliseconds(time)
    ) else { return false }
    follow(result)
    return result.handled
  }

  @objc private func tick() {
    guard let result = try? recognizer()?.tick(
      timeMs: milliseconds(CACurrentMediaTime())
    ) else { return stop() }
    follow(result)
  }

  /// Plays the result's notches, and ticks the recognizer when it asks: each
  /// frame while the camera glides or eases, or once at a time, as when a
  /// press may become a long press.
  private func follow(_ result: GestureResult) {
    for case .notch in result.events {
      playNotch()
      break
    }
    if result.nextTick != .frame {
      link?.invalidate()
      link = nil
    } else if link == nil {
      link = makeDisplayLink()
      link?.add(to: .main, forMode: .common)
    }
    timer?.invalidate()
    timer = nil
    if result.nextTick == .at {
      let delay = result.nextTickMs / 1000 - CACurrentMediaTime()
      timer = Timer.scheduledTimer(
        withTimeInterval: max(0, delay),
        repeats: false
      ) { [weak self] _ in
        MainActor.assumeIsolated { self?.tick() }
      }
    }
  }

  /// Events and display links count seconds since boot, as
  /// CACurrentMediaTime() does. An event can carry an earlier time than a
  /// tick that ran before it, so the recognizer's clock keeps the later one.
  private func milliseconds(_ time: TimeInterval) -> Double {
    lastMs = max(lastMs, time * 1000)
    return lastMs
  }

  #if os(iOS)
    private let notchFeedback = UISelectionFeedbackGenerator()
    // The recognizer's ID for each touch that is down.
    private var touchIds: [ObjectIdentifier: UInt32] = [:]
    private var nextTouchId: UInt32 = 0

    private func makeDisplayLink() -> CADisplayLink {
      CADisplayLink(target: self, selector: #selector(tick))
    }

    private func playNotch() {
      notchFeedback.selectionChanged()
    }

    func touchesBegan(_ touches: Set<UITouch>) {
      for touch in touches {
        nextTouchId &+= 1
        touchIds[ObjectIdentifier(touch)] = nextTouchId
        let kind: PointerKind = switch touch.type {
        case .pencil: .pen
        case .indirectPointer: .mouse
        default: .touch
        }
        handle(.pointerDown(GesturePointerDown(
          id: nextTouchId,
          kind: kind,
          position: point(touch)
        )), at: touch.timestamp)
      }
    }

    func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
      for touch in touches {
        guard let id = touchIds[ObjectIdentifier(touch)] else { continue }
        // The samples that UIKit coalesced into this event, so the recognizer
        // measures a flick's speed from each.
        for sample in event?.coalescedTouches(for: touch) ?? [touch] {
          handle(
            .pointerMove(GesturePointer(id: id, position: point(sample))),
            at: sample.timestamp
          )
        }
      }
    }

    func touchesEnded(_ touches: Set<UITouch>) {
      for touch in touches {
        guard let id = touchIds.removeValue(forKey: ObjectIdentifier(touch))
        else { continue }
        handle(
          .pointerUp(GesturePointer(id: id, position: point(touch))),
          at: touch.timestamp
        )
      }
    }

    /// The system took the touches away, as for a system gesture: no tap and
    /// no glide.
    func touchesCancelled(_ touches: Set<UITouch>) {
      for touch in touches {
        guard let id = touchIds.removeValue(forKey: ObjectIdentifier(touch))
        else { continue }
        handle(.pointerCancel(GesturePointer(id: id)), at: touch.timestamp)
      }
    }

    private func point(_ touch: UITouch) -> ScreenPoint {
      let point = touch.location(in: view)
      return ScreenPoint(x: point.x, y: point.y)
    }

  #elseif os(macOS)
    /// The recognizer's ID for the mouse.
    private static let mouse: UInt32 = 0

    /// The mouse button that started the mouse's drag.
    private var dragButton: Int?
    // Whether each of the trackpad's two pinch streams is under way.
    private var magnifying = false
    private var rotating = false

    private func makeDisplayLink() -> CADisplayLink? {
      view?.displayLink(target: self, selector: #selector(tick))
    }

    private func playNotch() {
      NSHapticFeedbackManager.defaultPerformer
        .perform(.alignment, performanceTime: .now)
    }

    /// `modes` are what the button does: the right one rotates and pitches.
    func mouseDown(_ event: NSEvent, modes: GestureMode) {
      guard dragButton == nil else { return }
      dragButton = event.buttonNumber
      flagsChanged(event)
      handle(.pointerDown(GesturePointerDown(
        id: Self.mouse,
        kind: .mouse,
        position: point(event),
        modes: modes
      )), at: event.timestamp)
    }

    func mouseDragged(_ event: NSEvent) {
      guard dragButton != nil else { return }
      handle(
        .pointerMove(GesturePointer(id: Self.mouse, position: point(event))),
        at: event.timestamp
      )
    }

    func mouseUp(_ event: NSEvent) {
      guard event.buttonNumber == dragButton else { return }
      dragButton = nil
      handle(
        .pointerUp(GesturePointer(id: Self.mouse, position: point(event))),
        at: event.timestamp
      )
    }

    /// What held keys do: Control rotates and pitches on a drag, and Shift
    /// zooms out on a double click.
    func flagsChanged(_ event: NSEvent) {
      var modes: GestureMode = []
      if event.modifierFlags.contains(.control) {
        modes.insert(.rotate)
      }
      if event.modifierFlags.contains(.shift) {
        modes.insert(.zoomOut)
      }
      handle(.modes(modes), at: event.timestamp)
    }

    /// A trackpad or a Magic Mouse pans, along with the system's glide after
    /// it. A mouse wheel zooms a third of a level a line, eased.
    func scrollWheel(_ event: NSEvent) {
      let position = point(event)
      let delta = event.hasPreciseScrollingDeltas
        ? GestureDelta(
          position: position,
          pan: ScreenPoint(x: event.scrollingDeltaX, y: event.scrollingDeltaY)
        )
        : GestureDelta(
          position: position,
          discrete: true,
          zoom: event.scrollingDeltaY / 3
        )
      handle(.delta(delta), at: event.timestamp)
    }

    func magnify(_ event: NSEvent) {
      pinch(event, stream: \.magnifying, scale: 1 + event.magnification)
    }

    /// The event turns counterclockwise, and the pinch clockwise.
    func rotate(_ event: NSEvent) {
      pinch(event, stream: \.rotating, rotation: -Double(event.rotation))
    }

    /// A two-finger double tap zooms in a level.
    func smartMagnify(_ event: NSEvent) {
      handle(
        .delta(GestureDelta(position: point(event), discrete: true, zoom: 1)),
        at: event.timestamp
      )
    }

    /// The arrow keys pan 100 points, and plus and minus zoom a level, about
    /// the view's center. Escape drops a drag. Returns whether the map used
    /// the key.
    func keyDown(_ event: NSEvent) -> Bool {
      let bounds = view?.bounds ?? .zero
      var delta = GestureDelta(
        position: ScreenPoint(x: bounds.midX, y: bounds.midY),
        discrete: true
      )
      switch event.specialKey {
      case .leftArrow?: delta.pan.x = 100
      case .rightArrow?: delta.pan.x = -100
      case .upArrow?: delta.pan.y = 100
      case .downArrow?: delta.pan.y = -100
      default:
        switch event.charactersIgnoringModifiers ?? "" {
        case "+", "=": delta.zoom = 1
        case "-": delta.zoom = -1
        case "\u{1b}" where dragButton != nil:
          dragButton = nil
          return handle(
            .pointerCancel(GesturePointer(id: Self.mouse)),
            at: event.timestamp
          )
        default: return false
        }
      }
      return handle(.delta(delta), at: event.timestamp)
    }

    /// Drops the drag and the pinch, and lets go of the held keys, once the
    /// view can't see their ends: when it or its window loses focus.
    func cancel() {
      let now = CACurrentMediaTime()
      if dragButton != nil {
        dragButton = nil
        handle(.pointerCancel(GesturePointer(id: Self.mouse)), at: now)
      }
      if magnifying || rotating {
        magnifying = false
        rotating = false
        handle(
          .pinch(GesturePinch(center: ScreenPoint(), scale: 1, phase: .cancel)),
          at: now
        )
      }
      handle(.modes([]), at: now)
    }

    /// One of the trackpad's two streams, as part of one pinch: the first to
    /// begin begins it, and the last to end ends it.
    private func pinch(
      _ event: NSEvent,
      stream: ReferenceWritableKeyPath<MapGestures, Bool>,
      scale: Double = 1,
      rotation: Double = 0
    ) {
      let phase: PinchPhase
      switch event.phase {
      case .began:
        let started = magnifying || rotating
        self[keyPath: stream] = true
        guard !started else { return }
        phase = .begin
      case .changed where self[keyPath: stream]:
        phase = .update
      case .ended, .cancelled:
        guard self[keyPath: stream] else { return }
        self[keyPath: stream] = false
        guard !magnifying, !rotating else { return }
        phase = event.phase == .ended ? .end : .cancel
      default:
        return
      }
      handle(.pinch(GesturePinch(
        center: point(event),
        scale: scale,
        rotation: rotation,
        phase: phase
      )), at: event.timestamp)
    }

    private func point(_ event: NSEvent) -> ScreenPoint {
      let point = view?.convert(event.locationInWindow, from: nil) ?? .zero
      return ScreenPoint(x: point.x, y: point.y)
    }
  #endif
}
