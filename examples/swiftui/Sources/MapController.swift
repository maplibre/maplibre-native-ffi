// A continuous map on a CAMetalLayer. The view reports its layer, size, and
// scale factor, and the scene reports whether the window is visible. The
// controller attaches a session to the layer, and keeps the map and session
// as these change. Everything else matches the browser example in
// examples/browser.

import MaplibreNativeFFI
import Observation
import QuartzCore

private let styleUrl = "https://tiles.openfreemap.org/styles/liberty"

private let initialCamera = CameraOptions(
  center: LatLng(latitude: 48.8584, longitude: 2.2945),
  zoom: 15.5,
  bearing: -30,
  pitch: 55
)

struct Status: Equatable {
  var message = ""
  var error = false
}

@MainActor @Observable
final class MapController {
  private(set) var status = Status()

  @ObservationIgnored private var started = false
  @ObservationIgnored private var failed = false

  @ObservationIgnored private var runtime: RuntimeHandle?
  @ObservationIgnored private var pacer: FramePacerHandle?
  @ObservationIgnored private(set) var map: MapHandle?
  @ObservationIgnored private var session: RenderSessionHandle?

  // What the view reports.
  @ObservationIgnored private var layer: CAMetalLayer?
  @ObservationIgnored private var size: LogicalSize?
  @ObservationIgnored private var scaleFactor: Double?
  @ObservationIgnored private var visible = true

  // What the session has.
  @ObservationIgnored private var sessionExtent: LogicalExtent?
  @ObservationIgnored private var hasTarget = false

  // Changes to the map and session run one at a time, in order.
  @ObservationIgnored private var changes: Task<Void, Never>?
  @ObservationIgnored private var changeQueued = false

  func start() {
    started = true
    failed = false
    update()
  }

  func dispose() {
    started = false
    // After any change in flight, which can still make a session, release the
    // session and only then the objects it uses.
    let previous = changes
    changes = Task {
      await previous?.value
      let (session, map, runtime, pacer) = (session, map, runtime, pacer)
      self.session = nil
      self.map = nil
      self.runtime = nil
      self.pacer = nil
      sessionExtent = nil
      hasTarget = false
      try? await session?.release()
      try? map?.dispose()
      try? runtime?.dispose()
      pacer?.release()
    }
  }

  func setLayer(_ layer: CAMetalLayer?) {
    self.layer = layer
    update()
  }

  func setSize(_ size: LogicalSize) {
    self.size = size
    update()
  }

  func setScaleFactor(_ scaleFactor: Double) {
    self.scaleFactor = scaleFactor
    update()
  }

  func setVisible(_ visible: Bool) {
    self.visible = visible
    update()
  }

  /// Brings the map and session in line with what the view last reported.
  private func update() {
    guard started, !changeQueued else { return }
    changeQueued = true
    let previous = changes
    changes = Task {
      await previous?.value
      changeQueued = false
      do {
        try await applyChanges()
      } catch {
        fail(String(describing: error))
      }
    }
  }

  private func applyChanges() async throws {
    guard started, let size, let scaleFactor else { return }

    // The platform takes the surface away, as when the app goes to the
    // background or the view leaves its window. Release it from the session,
    // and keep the session and its tiles for when the view shows again. This
    // runs even after a failure, since the session can still render.
    guard visible, let layer else {
      if let session, hasTarget {
        hasTarget = false
        try await session.releaseTarget()
      }
      return
    }
    guard !failed else { return }

    let extent = LogicalExtent(
      width: size.width,
      height: size.height,
      scaleFactor: scaleFactor
    )
    let map: MapHandle
    if let current = self.map {
      map = current
    } else {
      map = try await createMap(extent: extent)
    }
    guard let session, let sessionExtent else {
      try attach(map: map, layer: layer, extent: extent)
      return
    }
    if !hasTarget {
      try await session.setTarget(.metalLayer(
        MetalLayerTarget(logicalSize: size, layer: layer)
      ))
      hasTarget = true
      self.sessionExtent = LogicalExtent(
        width: size.width,
        height: size.height,
        scaleFactor: sessionExtent.scaleFactor
      )
    }
    // The session resizes the map with it, and takes a new scale factor when
    // the view moves to a display of another density.
    if self.sessionExtent != extent {
      try session.discarding.resize(extent: extent)
      self.sessionExtent = extent
    }
  }

  private func createMap(extent: LogicalExtent) async throws -> MapHandle {
    let runtime = try self.runtime ?? startRuntime()
    let map = try runtime.createMap(options: MapOptions(
      initialExtent: extent,
      mapMode: .continuous,
      eventMask: [.mapStyleLoaded, .mapLoadingFailed, .mapRenderError]
    ))
    self.map = map
    try await map.ready
    try map.discarding.updateCamera(
      CameraUpdate(mode: .jump, camera: initialCamera)
    )
    try map.discarding.setStyleUrl(styleUrl)
    showStatus("Loading the style…")
    return map
  }

  private func startRuntime() throws -> RuntimeHandle {
    let runtime = try RuntimeHandle.create()
    self.runtime = runtime
    // The handler runs on a library thread.
    try runtime.discarding
      .setEventHandler(RuntimeEventHandler { [weak self] events in
        Task { @MainActor [weak self] in self?.handle(events) }
      })
    // The pacer demands each session's frames at the display's refreshes.
    pacer = try FramePacerHandle.create()
    return runtime
  }

  /// Attaches a session to the layer, on a device that the session creates.
  /// The map was created at `extent`, which the session takes.
  private func attach(
    map: MapHandle,
    layer: CAMetalLayer,
    extent: LogicalExtent
  ) throws {
    let session = try map.attach(
      target: .metalLayer(MetalLayerTarget(
        logicalSize: LogicalSize(width: extent.width, height: extent.height),
        layer: layer
      )),
      options: RenderSessionAttachOptions(
        driver: .coreWorker,
        // The wake runs on a library thread.
        frameWake: Wake { [weak self] in
          Task { @MainActor [weak self] in self?.drainFrameResults() }
        }
      )
    )
    self.session = session
    hasTarget = true
    sessionExtent = extent
    try session.setFramePacer(pacer)
  }

  private func handle(_ events: [RuntimeEvent]) {
    for event in events where event.source == map?.id {
      switch event.type {
      case .mapStyleLoaded:
        showStatus("")
      case .mapLoadingFailed, .mapRenderError:
        fail(event.message)
      default:
        break
      }
    }
  }

  private func drainFrameResults() {
    guard let session else { return }
    do {
      let results = try session.drainFrameResults()
      if results.contains(where: { $0.disposition == .failed }) {
        fail((try? session.getFailure()) ?? "The map failed to render a frame.")
      }
    } catch {
      fail(String(describing: error))
    }
  }

  /// Once the controller shows an error, later progress doesn't replace it.
  private func showStatus(_ message: String) {
    guard !failed else { return }
    status = Status(message: message)
  }

  private func fail(_ message: String) {
    print("Map error: \(message)")
    guard !failed else { return }
    failed = true
    status = Status(message: message, error: true)
  }
}
