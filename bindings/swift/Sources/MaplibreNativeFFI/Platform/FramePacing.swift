// Typed forms of the view that frame pacing follows. The options keep the view
// alive while they exist, which covers the call that takes them.

#if os(macOS)
  import AppKit

  public extension FramePacingOptions {
    /// Paces the session's frames to the display of `view`'s window. Pass
    /// these options to ``RenderSessionHandle/setFramePacing(options:)`` on
    /// the main thread.
    init(maximumFramesPerSecond: UInt32? = nil, view: NSView) {
      self.init(
        maximumFramesPerSecond: maximumFramesPerSecond,
        view: NativePointer(retaining: view)
      )
    }
  }

#elseif os(iOS) || os(tvOS)
  import UIKit

  public extension FramePacingOptions {
    /// Paces the session's frames to the display of `view`'s window. Pass
    /// these options to ``RenderSessionHandle/setFramePacing(options:)`` on
    /// the main thread, while the view is in a window, and again when its
    /// scene moves to another screen.
    init(maximumFramesPerSecond: UInt32? = nil, view: UIView) {
      self.init(
        maximumFramesPerSecond: maximumFramesPerSecond,
        view: NativePointer(retaining: view)
      )
    }
  }
#endif
