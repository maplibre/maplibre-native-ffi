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

/// The runtime and map, driven by the native scheduler thread the runtime
/// owns. Camera and resize calls submit commands without waiting for them.
@MainActor
final class MapState {
  private let runtime: RuntimeHandle
  let map: MapHandle
  private var isClosed = false
  /// Reports a command that failed after native code accepted it.
  var onFailure: (@MainActor (Error) -> Void)?

  /// Creates the map and loads `styleJSON`, or the example's network style
  /// when it is nil. `eventWake` reports queued runtime events.
  init(
    viewport: Viewport,
    eventWake: Wake,
    styleJSON: Data? = nil
  ) async throws {
    precondition(
      !viewport.isEmpty,
      "cannot create MapState with an empty viewport"
    )
    let runtime = try Maplibre.runtimeCreate(
      options: RuntimeOptions(cachePath: ":memory:", eventWake: eventWake)
    )
    let map: MapHandle
    do {
      map = try await runtime.mapCreate(options: MapOptions(
        initialExtent: LogicalExtent(
          width: viewport.logicalWidth,
          height: viewport.logicalHeight,
          scaleFactor: viewport.scaleFactor
        ),
        mapMode: .continuous,
        eventMask: [.mapRenderUpdateAvailable]
      ))
    } catch {
      try? await runtime.close()
      throw error
    }

    self.runtime = runtime
    self.map = map
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
  }

  func close() async throws {
    guard !isClosed else { return }
    isClosed = true
    // Awaiting both release completions lets native teardown finish before the
    // app tears down state that the callbacks use.
    try await map.close()
    try await runtime.close()
  }

  /// Drains every queued runtime event and reports whether the map published
  /// a render update.
  func drainEvents() throws -> Bool {
    guard !isClosed else { return false }
    let batch = try runtime.drainEvents()
    defer { try? batch.close() }
    return try batch.get().events.contains {
      $0.sourceType == .map && $0.source == map.id &&
        $0.type == .mapRenderUpdateAvailable
    }
  }

  func resize(_ viewport: Viewport) {
    submit { _ = try await $0.resize(extent: LogicalExtent(
      width: viewport.logicalWidth,
      height: viewport.logicalHeight,
      scaleFactor: viewport.scaleFactor
    )) }
  }

  func setGestureInProgress(_ inProgress: Bool) {
    submit { _ = try await $0.updateCamera(update: CameraUpdate(
      camera: CameraOptions(),
      gesturePhase: inProgress ? .begin : .end
    )) }
  }

  func cancelTransitions() {
    submit { _ = try await $0.cancelTransitions() }
  }

  func moveBy(dx: Double, dy: Double, animation: AnimationOptions? = nil) {
    submit { _ = try await $0.applyCameraDelta(delta: CameraDelta(
      offset: ScreenPoint(x: dx, y: dy),
      animation: animation ?? AnimationOptions()
    )) }
  }

  func scaleBy(
    _ scale: Double,
    anchor: ScreenPoint,
    animation: AnimationOptions? = nil
  ) {
    submit { _ = try await $0.applyCameraDelta(delta: CameraDelta(
      kind: .scale,
      amount: scale,
      anchor: anchor,
      animation: animation ?? AnimationOptions()
    )) }
  }

  func adjustBearing(delta: Double, animation: AnimationOptions? = nil) {
    submit { _ = try await $0.applyCameraDelta(delta: CameraDelta(
      kind: .bearing,
      amount: delta,
      animation: animation ?? AnimationOptions()
    )) }
  }

  func adjustPitch(delta: Double, animation: AnimationOptions? = nil) {
    submit { _ = try await $0.applyCameraDelta(delta: CameraDelta(
      kind: .pitch,
      amount: delta,
      animation: animation ?? AnimationOptions()
    )) }
  }

  func resetOrientation(animation: AnimationOptions) {
    submit { _ = try await $0.updateCamera(update: CameraUpdate(
      mode: .ease,
      camera: CameraOptions(bearing: 0, pitch: 0),
      animation: animation
    )) }
  }

  /// Submits one command. The command reaches native code when its task
  /// starts, before the task first suspends, and tasks that the main actor
  /// creates for itself start in creation order, so commands keep input order.
  private func submit(
    _ command: @escaping @MainActor @Sendable (MapHandle) async throws -> Void
  ) {
    guard !isClosed else { return }
    let map = map
    Task { @MainActor [weak self] in
      do { try await command(map) }
      catch { self?.onFailure?(error) }
    }
  }
}
