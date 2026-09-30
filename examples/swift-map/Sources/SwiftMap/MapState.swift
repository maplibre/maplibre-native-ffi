import Foundation
import MaplibreNativeFFI

struct Viewport: Equatable {
  var logicalWidth: UInt32
  var logicalHeight: UInt32
  var physicalWidth: UInt32
  var physicalHeight: UInt32
  var scaleFactor: Double
  var isEmpty: Bool

  var extent: RenderTargetExtent {
    RenderTargetExtent(
      width: logicalWidth,
      height: logicalHeight,
      scaleFactor: scaleFactor
    )
  }

  func log(_ label: String) {
    let scale = String(format: "%.2f", scaleFactor)
    let emptyLabel = isEmpty ? " empty=true" : ""
    print(
      "\(label): logical=\(logicalWidth)x\(logicalHeight) physical=\(physicalWidth)x\(physicalHeight) scale=\(scale)\(emptyLabel)"
    )
  }
}

/// Runtime and map state owned by the main render loop.
@MainActor
final class MapState {
  private let runtime: RuntimeHandle
  private let map: MapHandle
  private var isClosed = false
  private let eventRelay: EventRelay

  /// Creates the map and loads `styleJSON`, or the example's network style
  /// when it is nil.
  init(viewport: Viewport, styleJSON: Data? = nil) async throws {
    precondition(
      !viewport.isEmpty,
      "cannot create MapState with an empty viewport"
    )
    let eventRelay = EventRelay()
    self.eventRelay = eventRelay
    let runtime = try Maplibre.runtimeCreate(
      options: RuntimeOptions(
        cachePath: ":memory:",
        eventWake: Wake(callback: { Task { @MainActor in eventRelay.fire() } })
      )
    )
    let map: MapHandle
    do {
      map = try await runtime.mapCreate(options: MapOptions(
        initialExtent: LogicalExtent(
          width: viewport.logicalWidth,
          height: viewport.logicalHeight,
          scaleFactor: viewport.scaleFactor
        ),
        mapMode: .continuous
      ))
    } catch {
      try? await runtime.close()
      throw error
    }

    self.runtime = runtime
    self.map = map
    try await map.setEventMask(mask: [.mapRenderUpdateAvailable])
    if let styleJSON {
      _ = try await map.setStyleJson(json: styleJSON)
    } else {
      _ = try await map.setStyleUrl(url:
        "https://tiles.openfreemap.org/styles/bright")
    }
    _ = try await map.updateCamera(update: CameraUpdate(camera: CameraOptions(
      center: LatLng(latitude: 37.7749, longitude: -122.4194),
      zoom: 13.0,
      bearing: 12.0,
      pitch: 30.0
    )))
    _ = try await map.requestRepaint()
  }

  var mapHandle: MapHandle {
    map
  }

  func scheduleEventDrains(
    onRenderRequested: @escaping @MainActor @Sendable () -> Void,
    onFailure: @escaping @MainActor @Sendable (Error) -> Void
  ) {
    eventRelay.callback = { [weak self] in
      guard let self, !self.isClosed else { return }
      do { if try self.drainEvents() { onRenderRequested() } }
      catch { onFailure(error) }
    }
    eventRelay.fire()
  }

  func close() async throws {
    guard !isClosed else { return }
    isClosed = true
    eventRelay.callback = nil
    // Awaiting both release completions keeps process exit ordered after native
    // teardown.
    try await map.close()
    try await runtime.close()
  }

  private func drainEvents() throws -> Bool {
    var renderPending = false
    let batch: EventBatchHandle
    do { batch = try runtime.drainEvents() }
    catch let error as MaplibreError
      where error.kind == .notReady { return false }
    defer { try? batch.close() }
    for event in try batch.get().events
      where event.sourceType == .map && event.source == map.id
    {
      if event.type == .mapRenderUpdateAvailable {
        renderPending = true
      }
    }
    return renderPending
  }

  func resize(_ extent: LogicalExtent) async throws {
    _ = try await map.resize(extent: extent)
  }

  func setGestureInProgress(_ inProgress: Bool) async throws {
    _ = try await map.updateCamera(update: CameraUpdate(
      camera: CameraOptions(),
      gesturePhase: inProgress ? .begin : .end
    ))
  }

  func cancelTransitions() async throws {
    _ = try await map.cancelTransitions()
  }

  func moveBy(
    dx: Double,
    dy: Double,
    animation: AnimationOptions? = nil
  ) async throws {
    _ = try await map.applyCameraDelta(delta: CameraDelta(
      offset: ScreenPoint(x: dx, y: dy),
      animation: animation ?? AnimationOptions()
    ))
  }

  func scaleBy(
    _ scale: Double,
    anchor: ScreenPoint,
    animation: AnimationOptions? = nil
  ) async throws {
    _ = try await map.applyCameraDelta(delta: CameraDelta(
      kind: .scale,
      amount: scale,
      anchor: anchor,
      animation: animation ?? AnimationOptions()
    ))
  }

  func adjustBearing(
    delta: Double,
    animation: AnimationOptions? = nil
  ) async throws {
    _ = try await map.applyCameraDelta(delta: CameraDelta(
      kind: .bearing,
      amount: delta,
      animation: animation ?? AnimationOptions()
    ))
  }

  func adjustPitch(
    delta: Double,
    animation: AnimationOptions? = nil
  ) async throws {
    _ = try await map.applyCameraDelta(delta: CameraDelta(
      kind: .pitch,
      amount: delta,
      animation: animation ?? AnimationOptions()
    ))
  }

  func resetOrientation(animation: AnimationOptions) async throws {
    _ = try await map.updateCamera(update: CameraUpdate(
      mode: .ease,
      camera: CameraOptions(bearing: 0, pitch: 0),
      animation: animation
    ))
  }
}

@MainActor
private final class EventRelay {
  var callback: (() -> Void)?
  func fire() {
    callback?()
  }
}
