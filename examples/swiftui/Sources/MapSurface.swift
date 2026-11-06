// A view backed by a CAMetalLayer, which reports its layer, its size in
// logical pixels, and its scale factor to the controller, and passes its input
// to the gestures.

import MaplibreNativeFFI
import QuartzCore
import SwiftUI

#if os(iOS)
  import UIKit

  struct MapSurface: UIViewRepresentable {
    let controller: MapController

    func makeUIView(context _: Context) -> MetalView {
      MetalView(controller: controller)
    }

    func updateUIView(_: MetalView, context _: Context) {}
  }

  final class MetalView: UIView {
    override static var layerClass: AnyClass {
      CAMetalLayer.self
    }

    private let controller: MapController
    private var gestures: TouchGestures?

    init(controller: MapController) {
      self.controller = controller
      super.init(frame: .zero)
      isMultipleTouchEnabled = true
      gestures = TouchGestures(view: self) { [weak self] delta in
        try? self?.controller.map?.discarding.applyCameraDelta(delta)
      }
      registerForTraitChanges([UITraitDisplayScale.self]) { (
        view: MetalView,
        _
      ) in
        view.controller.setScaleFactor(view.traitCollection.displayScale)
      }
    }

    @available(*, unavailable)
    required init?(coder _: NSCoder) {
      fatalError("init(coder:) is unavailable")
    }

    override func didMoveToWindow() {
      super.didMoveToWindow()
      controller.setScaleFactor(traitCollection.displayScale)
      controller.setLayer(window == nil ? nil : layer as? CAMetalLayer)
    }

    override func layoutSubviews() {
      super.layoutSubviews()
      controller.setSize(logicalSize(bounds.size))
    }

    override func touchesBegan(_ touches: Set<UITouch>, with _: UIEvent?) {
      gestures?.began(touches)
    }

    override func touchesEnded(_ touches: Set<UITouch>, with _: UIEvent?) {
      gestures?.ended(touches)
    }

    /// A recognizer that takes over the touches cancels them here.
    override func touchesCancelled(_ touches: Set<UITouch>, with _: UIEvent?) {
      gestures?.ended(touches)
    }
  }

#elseif os(macOS)
  import AppKit

  struct MapSurface: NSViewRepresentable {
    let controller: MapController

    func makeNSView(context _: Context) -> MetalView {
      MetalView(controller: controller)
    }

    func updateNSView(_: MetalView, context _: Context) {}
  }

  final class MetalView: NSView {
    private let controller: MapController
    private lazy var gestures =
      PointerGestures(view: self) { [weak self] delta in
        try? self?.controller.map?.discarding.applyCameraDelta(delta)
      }

    init(controller: MapController) {
      self.controller = controller
      super.init(frame: .zero)
      wantsLayer = true
    }

    @available(*, unavailable)
    required init?(coder _: NSCoder) {
      fatalError("init(coder:) is unavailable")
    }

    override func makeBackingLayer() -> CALayer {
      CAMetalLayer()
    }

    /// Screen points run down from the top left, as the map's do.
    override var isFlipped: Bool {
      true
    }

    override var acceptsFirstResponder: Bool {
      true
    }

    override func acceptsFirstMouse(for _: NSEvent?) -> Bool {
      true
    }

    override func viewDidMoveToWindow() {
      super.viewDidMoveToWindow()
      viewDidChangeBackingProperties()
      controller.setLayer(window == nil ? nil : layer as? CAMetalLayer)
    }

    /// The window moved to a display of another density.
    override func viewDidChangeBackingProperties() {
      super.viewDidChangeBackingProperties()
      if let window {
        controller.setScaleFactor(window.backingScaleFactor)
      }
    }

    override func setFrameSize(_ newSize: NSSize) {
      super.setFrameSize(newSize)
      controller.setSize(logicalSize(newSize))
    }

    override func mouseDown(with event: NSEvent) {
      gestures.mouseDown(
        event,
        rotating: event.modifierFlags.contains(.control)
      )
    }

    override func mouseDragged(with event: NSEvent) {
      gestures.mouseDragged(event)
    }

    override func mouseUp(with event: NSEvent) {
      gestures.mouseUp(event)
    }

    override func rightMouseDown(with event: NSEvent) {
      gestures.mouseDown(event, rotating: true)
    }

    override func rightMouseDragged(with event: NSEvent) {
      gestures.mouseDragged(event)
    }

    override func rightMouseUp(with event: NSEvent) {
      gestures.mouseUp(event)
    }

    override func scrollWheel(with event: NSEvent) {
      gestures.scrollWheel(event)
    }

    override func magnify(with event: NSEvent) {
      gestures.magnify(event)
    }

    override func rotate(with event: NSEvent) {
      gestures.rotate(event)
    }

    override func smartMagnify(with event: NSEvent) {
      gestures.smartMagnify(event)
    }
  }
#endif

/// A view's size in points, which the map takes as logical pixels.
private func logicalSize(_ size: CGSize) -> LogicalSize {
  LogicalSize(
    width: UInt32(max(1, size.width.rounded())),
    height: UInt32(max(1, size.height.rounded()))
  )
}
