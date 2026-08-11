internal import CMaplibreNativeC
import Foundation

public enum MapMode: UInt32, Sendable, Hashable {
  case continuous = 0
  case `static` = 1
  case tile = 2
}

public struct MapOptions: Equatable, Sendable {
  /// Initial logical width in UI pixels, replaced by the extent of the first
  /// attached render session.
  public var width: UInt32
  /// Initial logical height in UI pixels, replaced by the extent of the first
  /// attached render session.
  public var height: UInt32
  /// UI-to-device pixel scale, fixed for the lifetime of the map. It selects
  /// sprites, glyphs, and raster tiles; a render session attached or resized
  /// with a different scale factor logs a warning.
  public var scaleFactor: Double
  public var mode: MapMode
  /// Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
  /// encodings, fixed for the lifetime of the map. A map created with this
  /// `false` logs a tile parse warning for such tiles.
  public var fastPFOREnabled: Bool
  /// Map-originated event types this map queues, every type the library
  /// reports unless the host narrows it.
  ///
  /// A map reports the two camera events of its initial sizing whatever this
  /// selects, because MapLibre resizes the map inside its own constructor.
  public var eventMask: RuntimeEventMask

  public init(
    width: UInt32,
    height: UInt32,
    scaleFactor: Double = 1.0,
    mode: MapMode = .continuous,
    fastPFOREnabled: Bool = false,
    eventMask: RuntimeEventMask = .mapOptionsDefault
  ) {
    self.width = width
    self.height = height
    self.scaleFactor = scaleFactor
    self.mode = mode
    self.fastPFOREnabled = fastPFOREnabled
    self.eventMask = eventMask
  }

  var nativeInput: NativeMapOptionsInput {
    NativeMapOptionsInput(
      width: width,
      height: height,
      scaleFactor: scaleFactor,
      mapMode: mode.rawValue,
      fastPFOREnabled: fastPFOREnabled,
      eventMask: eventMask.rawValue
    )
  }
}

public final class MapHandle {
  private let handle: NativeHandleBox<NativeMapHandle>
  private let mapId: MapId

  public init(runtime: RuntimeHandle, options: MapOptions) throws {
    let native = try mapNativeFailure {
      try options.nativeInput.withNativeOptions { nativeOptions in
        try NativeMap.create(
          runtime: runtime.requireLiveHandle(),
          options: nativeOptions
        )
      }
    }
    mapId = MapId(value: native.raw)
    handle = try NativeHandleBox(typeName: "MapHandle", handle: native)
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func isSource(of event: RuntimeEvent) -> Bool {
    guard case let .map(source) = event.source else {
      return false
    }
    return source == mapId
  }

  func requireLiveHandle() throws -> NativeMapHandle {
    try handle.requireLive()
  }

  /// Produces a `Sendable` reference to this map for attaching a render
  /// session from a thread other than the map's owner thread.
  public func attachRef() throws -> MapAttachRef {
    // Resolve once so a closed map fails here rather than at the first attach.
    _ = try requireLiveHandle()
    return MapAttachRef(handle: handle)
  }

  /// Selects which map-originated event types this map queues.
  ///
  /// The call reads the bits in ``RuntimeEventMask/allMapEvents`` and ignores
  /// the rest, so ``RuntimeEventMask/all`` selects every one of them. Narrowing
  /// gates later events and keeps queued ones, so a host drains what it already
  /// caused.
  ///
  /// Select every type the host reads: map-render-update-available is the map's
  /// only invalidation report, the two still-image types are the only reports
  /// that a still-image request finished, and map-loading-failed and
  /// map-render-error carry native failure text.
  public func setEventMask(_ mask: RuntimeEventMask) throws {
    try mapNativeFailure {
      try NativeMap.setEventMask(handle.requireLive(), mask: mask.rawValue)
    }
  }

  /// The mask last set. A map that has not been narrowed reports
  /// ``RuntimeEventMask/all``. Read it, change one bit, and write it back to
  /// leave the other bits alone.
  public var eventMask: RuntimeEventMask {
    get throws {
      try mapNativeFailure {
        try RuntimeEventMask(
          rawValue: NativeMap.eventMask(handle.requireLive())
        )
      }
    }
  }

  /// Destroys this map, discarding its undrained events the way the C API
  /// discards the ones it still holds.
  public func close() throws {
    try handle.closeOnce { handle in
      try checkStatus(mln_map_destroy(handle.raw))
    }
  }

  /// Loads a style URL.
  ///
  /// Loading is asynchronous: a missing, unreachable, or malformed style still
  /// returns normally here and reports through a map-loading-failed runtime
  /// event. A style MapLibre rejects semantically, such as one with an unknown
  /// `version`, produces neither an error nor an event.
  public func setStyleURL(_ url: String) throws {
    try mapNativeFailure {
      try NativeString.withCString(url) { url in
        try checkStatus(mln_map_set_style_url(handle.requireLive().raw, url))
      }
    }
  }

  /// Loads inline style JSON.
  ///
  /// Malformed JSON is reported twice: this call throws the parse error, and
  /// the same message arrives as a map-loading-failed runtime event. A style
  /// MapLibre rejects semantically, such as one with an unknown `version`,
  /// produces neither an error nor an event.
  public func setStyleJSON(_ json: Data) throws {
    try mapNativeFailure {
      let arena = NativeInputArena()
      defer { withExtendedLifetime(arena) {} }
      try checkStatus(mln_map_set_style_json(
        handle.requireLive().raw,
        arena.view(json)
      ))
    }
  }

  /// Copies the style document this map's style was last parsed from, byte for
  /// byte, rather than a serialization of the live style. Runtime mutations do
  /// not change it, and a failed parse leaves the previous document in place.
  /// The result is empty when no document has been parsed.
  public func loadedStyleJSON() throws -> Data {
    try mapNativeFailure {
      try NativeStyle.copyMapData(handle.requireLive()) {
        mln_map_copy_loaded_style_json($0, $1, $2, $3)
      }
    }
  }

  /// Copies the URL this map's style was last requested from.
  ///
  /// ``setStyleURL(_:)`` records the URL when the request is made, before the
  /// document parses, so this can disagree with ``loadedStyleJSON()`` while a
  /// load is in flight or after one fails. The result is empty for inline JSON,
  /// for a map that has loaded no style, and for an empty URL alike.
  public func styleURL() throws -> String {
    try mapNativeFailure {
      try NativeStyle
        .copyMapText(handle.requireLive()) { map, text, capacity, size in
          mln_map_copy_style_url(map, text, capacity, size)
        }
    }
  }

  public func requestRepaint() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_request_repaint(handle.requireLive().raw))
    }
  }

  public func requestStillImage() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_request_still_image(handle.requireLive().raw))
    }
  }

  public func camera() throws -> CameraOptions {
    try mapNativeFailure {
      try CameraOptions(native: NativeCameraOptionsInput(NativeMap
          .camera(handle.requireLive())))
    }
  }

  public func jump(to camera: CameraOptions) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try checkStatus(mln_map_jump_to(handle.requireLive().raw, nativeCamera))
      }
    }
  }

  public func ease(
    to camera: CameraOptions,
    animation: AnimationOptions? = nil
  ) throws {
    try mapNativeFailure {
      try camera.nativeInput.withNativeOptions { nativeCamera in
        try (animation?.nativeInput ?? NativeAnimationOptionsInput())
          .withOptionalNativeOptions { nativeAnimation in
            try checkStatus(mln_map_ease_to(
              handle.requireLive().raw,
              nativeCamera,
              nativeAnimation
            ))
          }
      }
    }
  }

  public func moveBy(deltaX: Double, deltaY: Double) throws {
    try mapNativeFailure {
      try checkStatus(mln_map_move_by(handle.requireLive().raw, deltaX, deltaY))
    }
  }

  public func moveBy(
    deltaX: Double,
    deltaY: Double,
    animation: AnimationOptions
  ) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { nativeAnimation in
        try checkStatus(mln_map_move_by_animated(
          handle.requireLive().raw,
          deltaX,
          deltaY,
          nativeAnimation
        ))
      }
    }
  }

  public func scaleBy(_ scale: Double, anchor: ScreenPoint? = nil) throws {
    try mapNativeFailure {
      if var nativeAnchor = anchor?.nativeInput.native {
        try withUnsafePointer(to: &nativeAnchor) { anchor in
          try checkStatus(mln_map_scale_by(
            handle.requireLive().raw,
            scale,
            anchor
          ))
        }
      } else {
        try checkStatus(mln_map_scale_by(handle.requireLive().raw, scale, nil))
      }
    }
  }

  public func scaleBy(
    _ scale: Double,
    anchor: ScreenPoint? = nil,
    animation: AnimationOptions
  ) throws {
    try mapNativeFailure {
      try animation.nativeInput.withOptionalNativeOptions { nativeAnimation in
        if var nativeAnchor = anchor?.nativeInput.native {
          try withUnsafePointer(to: &nativeAnchor) { anchor in
            try checkStatus(mln_map_scale_by_animated(
              handle.requireLive().raw,
              scale,
              anchor,
              nativeAnimation
            ))
          }
        } else {
          try checkStatus(mln_map_scale_by_animated(
            handle.requireLive().raw,
            scale,
            nil,
            nativeAnimation
          ))
        }
      }
    }
  }

  public func cancelTransitions() throws {
    try mapNativeFailure {
      try checkStatus(mln_map_cancel_transitions(handle.requireLive().raw))
    }
  }

  /// Marks whether a host-driven gesture is in progress. The flag stays set
  /// until the host clears it, so pair every `true` with a `false`.
  public func setGestureInProgress(_ inProgress: Bool) throws {
    try mapNativeFailure {
      try checkStatus(mln_map_set_gesture_in_progress(
        handle.requireLive().raw,
        inProgress
      ))
    }
  }

  /// Returns whether a host-driven gesture is currently in progress.
  public func isGestureInProgress() throws -> Bool {
    try mapNativeFailure {
      try NativeMap.isGestureInProgress(handle.requireLive())
    }
  }
}
