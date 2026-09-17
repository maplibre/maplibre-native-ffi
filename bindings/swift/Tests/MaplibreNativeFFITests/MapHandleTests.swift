import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

private struct CameraEventTally {
  var finishedTransitionIds: [UInt64] = []
  var lastDidChangeMode: CameraChangeMode?
  /// The types this batch carried, in queue order.
  var types: [RuntimeEventType] = []
}

private func drainCameraEvents(_ runtime: RuntimeHandle) throws
  -> CameraEventTally
{
  var tally = CameraEventTally()
  for event in try runtime.drainEvents() {
    tally.types.append(event.type)
    switch event.type {
    case .mapCameraTransitionFinished:
      guard case let .cameraTransitionFinished(payload) = event.payload else {
        Issue.record("unexpected transition payload: \(event.payload)")
        continue
      }
      tally.finishedTransitionIds.append(payload.transitionId)
    case .mapCameraDidChange:
      tally.lastDidChangeMode = CameraChangeMode
        .fromNative(UInt32(bitPattern: event.code))
    default:
      break
    }
  }
  return tally
}

@Test func mapOptionsMaterializeInitialExtentAndFastPFOR() throws {
  try MapOptions(
    width: 256,
    height: 128,
    scaleFactor: 2,
    fastPFOREnabled: true
  ).nativeInput.withNativeOptions { native in
    #expect(native.pointee.initial_extent.width == 256)
    #expect(native.pointee.initial_extent.height == 128)
    #expect(native.pointee.initial_extent.scale_factor == 2)
    #expect(native.pointee.fast_pfor_enabled)
  }
}

@Test func mapLifecycleCommandsSnapshotsAndOrderedCameraQuery() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 256, height: 128, scaleFactor: 2)
  )

  let initial = try map.snapshot()
  #expect(initial.logicalExtent == MapLogicalExtent(
    width: 256,
    height: 128,
    scaleFactor: 2
  ))

  // Cross an actor/task suspension before using the any-thread map handle.
  await Task.yield()
  let cameraCommand = try await map.updateCamera(CameraUpdate(
    camera: CameraOptions(
      center: LatLng(latitude: 37.7749, longitude: -122.4194),
      zoom: 12
    )
  ))
  let resizeCommand = try await map.resize(to: MapLogicalExtent(
    width: 512,
    height: 256,
    scaleFactor: 2
  ))
  #expect(cameraCommand.disposition == .committed)
  #expect(resizeCommand.generation >= cameraCommand.generation)

  let ordered = try await map.queryCamera()
  #expect(abs((ordered.camera.center?.latitude ?? 0) - 37.7749) < 0.000001)
  #expect(abs((ordered.camera.center?.longitude ?? 0) - -122.4194) < 0.000001)
  #expect(abs((ordered.camera.zoom ?? 0) - 12) < 0.000001)

  let resized = try map.snapshot()
  #expect(resized.generation >= ordered.generation)
  #expect(resized.logicalExtent.width == 512)
  #expect(resized.logicalExtent.height == 256)

  try await map.close()
  #expect(map.isClosed)
  try await runtime.close()
  #expect(runtime.isClosed)
}

/// The published camera snapshot is a synchronous read of the latest commit,
/// while the ordered query observes every command accepted before it.
@Test func cameraSnapshotIsSynchronousAndOrderedQueryObservesCommands(
) async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )
  defer { try? map.closeBlockingForTests() }

  let before = try map.cameraSnapshot()
  _ = try await map.updateCamera(CameraUpdate(camera: CameraOptions(zoom: 6)))

  await Task.yield()
  let ordered = try await map.queryCamera()
  #expect(ordered.generation > before.generation)
  #expect(abs((ordered.camera.zoom ?? 0) - 6) < 0.000001)

  let after = try map.cameraSnapshot()
  #expect(after.generation >= ordered.generation)
  #expect(abs((after.camera.zoom ?? 0) - 6) < 0.000001)
}

/// A committed mutation's finished event reports the generation its commit
/// published, and a snapshot at or past that generation observes the value.
@Test func snapshotObservesACommittedCommandAtItsGeneration() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )
  defer { try? map.closeBlockingForTests() }

  let result = try await map.setDebugOptions([.tileBorders])
  #expect(result.disposition == .committed)
  #expect(result.generation > 0)

  let snapshot = try map.snapshot()
  #expect(snapshot.generation >= result.generation)
  #expect(snapshot.debugOptions == [.tileBorders])
}

/// The snapshot's tile, bound, and free-camera fields observe their set
/// commands.
@Test func snapshotFieldsRoundTripThroughTheirSetCommands() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )
  defer { try? map.closeBlockingForTests() }

  _ = try await map.setTileOptions(MapTileOptions(
    prefetchZoomDelta: 5,
    lodScale: 2
  ))
  _ = try await map.setBounds(BoundOptions(
    minZoom: 2,
    maxZoom: 15,
    maxPitch: 45
  ))
  // The altitude sits inside the bound zoom range, so it is not clamped.
  _ = try await map.setFreeCameraOptions(FreeCameraOptions(
    position: Vec3(x: 0.25, y: 0.5, z: 0.01)
  ))
  let statsCommand = try await map.setRenderingStatsViewEnabled(true)
  #expect(statsCommand.disposition == .committed)

  let snapshot = try map.snapshot()
  #expect(snapshot.tileOptions.prefetchZoomDelta == 5)
  #expect(snapshot.tileOptions.lodScale == 2)
  #expect(snapshot.bounds.minZoom == 2)
  #expect(snapshot.bounds.maxZoom == 15)
  #expect(snapshot.bounds.maxPitch == 45)
  let position = try #require(snapshot.freeCameraOptions.position)
  #expect(abs(position.x - 0.25) < 0.000001)
  #expect(abs(position.y - 0.5) < 0.000001)
  #expect(abs(position.z - 0.01) < 0.000001)
  #expect(snapshot.renderingStatsViewEnabled)
}

@Test func requestRepaintReturnsACommittedCompletion() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64, mode: .continuous)
  )
  defer { try? map.closeBlockingForTests() }

  let command = try await map.requestRepaint()
  #expect(command.disposition == .committed)
}

/// The terminal outcomes a headless map's camera transitions reach: a
/// zero-duration ease finishes inside its own command, a later camera command
/// supersedes a running one, and cancelling ends the one still running.
@Test func cameraTransitionIdReportsTerminalOutcomesOnce() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 256, height: 256)
  )
  defer { try? map.closeBlockingForTests() }

  var camera = CameraOptions(
    center: LatLng(latitude: 37.7749, longitude: -122.4194),
    zoom: 11,
    bearing: 12,
    pitch: 30
  )

  // MapLibre resizes the map inside its own constructor, so drop the camera
  // events of the initial sizing before the batch that matters.
  try await runtime.barrier()
  _ = try runtime.drainEvents()

  // A zero-duration ease resolves inside the command, so its event lands ahead
  // of the immediate did-change event.
  _ = try await map.updateCamera(CameraUpdate(
    mode: .ease,
    camera: camera,
    animation: AnimationOptions(durationMilliseconds: 0, transitionId: 7)
  ))
  try await runtime.barrier()
  var tally = try drainCameraEvents(runtime)
  #expect(tally.finishedTransitionIds == [7])
  #expect(tally.lastDidChangeMode == .immediate)

  // One batch keeps queue order, so a host reads the transition's own event
  // before the camera change that ended it.
  let finishedIndex = try #require(
    tally.types.firstIndex(of: .mapCameraTransitionFinished)
  )
  let didChangeIndex = try #require(
    tally.types.firstIndex(of: .mapCameraDidChange)
  )
  #expect(finishedIndex < didChangeIndex)

  // A running transition stays silent until it releases the camera.
  camera.zoom = 12
  _ = try await map.updateCamera(CameraUpdate(
    mode: .ease,
    camera: camera,
    animation: AnimationOptions(durationMilliseconds: 5000, transitionId: 11)
  ))
  try await runtime.barrier()
  tally = try drainCameraEvents(runtime)
  #expect(tally.finishedTransitionIds.isEmpty)

  // A later camera command supersedes it and reports the superseded identity.
  camera.zoom = 13
  _ = try await map.updateCamera(CameraUpdate(
    mode: .ease,
    camera: camera,
    animation: AnimationOptions(durationMilliseconds: 5000, transitionId: 12)
  ))
  try await runtime.barrier()
  tally = try drainCameraEvents(runtime)
  #expect(tally.finishedTransitionIds == [11])
  #expect(tally.lastDidChangeMode == .animated)

  let cancelled = try await map.cancelTransitions()
  #expect(cancelled.disposition == .committed)
  try await runtime.barrier()
  tally = try drainCameraEvents(runtime)
  #expect(tally.finishedTransitionIds == [12])

  // A transition started without an identity reports nothing when cancelled.
  camera.zoom = 14
  _ = try await map.updateCamera(CameraUpdate(
    mode: .ease,
    camera: camera,
    animation: AnimationOptions(durationMilliseconds: 5000)
  ))
  _ = try await map.cancelTransitions()
  try await runtime.barrier()
  tally = try drainCameraEvents(runtime)
  #expect(tally.finishedTransitionIds.isEmpty)

  // Cancelling with nothing running commits and changes nothing.
  let idle = try await map.cancelTransitions()
  #expect(idle.disposition == .committed)
}

/// A gesture phase publishes the map's gesture flag around the camera write.
@Test func gesturePhaseReportsThroughTheSnapshot() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 256, height: 256)
  )
  defer { try? map.closeBlockingForTests() }

  #expect(try !map.snapshot().gestureInProgress)

  _ = try await map.updateCamera(CameraUpdate(
    camera: CameraOptions(zoom: 9),
    gesturePhase: .begin
  ))
  #expect(try map.snapshot().gestureInProgress)
  #expect(try map.cameraSnapshot().camera.zoom == 9)

  _ = try await map.updateCamera(CameraUpdate(
    camera: CameraOptions(),
    gesturePhase: .end
  ))
  #expect(try !map.snapshot().gestureInProgress)
}

/// A still-image request a static map never gets to serve reports its
/// cancellation when the map is closed.
@Test func closingAMapCancelsItsPendingStillImageRequest() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64, mode: .static)
  )
  try await map.setStyleJSON(emptyStyleJSON)

  // Submitting synchronously puts the request on the map before the close,
  // which is what makes the outcome deterministic. No render session is
  // attached, so the map never gets to serve it.
  let pending = try NativeMap.requestStillImage(map.requireLiveHandle())
  try await map.close()

  do {
    try await awaitNative { pending }
    Issue.record("a discarded still-image request should report cancellation")
  } catch let error as MaplibreError {
    #expect(error.kind == .cancelled)
    #expect(error.rawStatus == MLN_STATUS_CANCELLED.rawValue)
  }
}

/// An unbounded map wraps across the antimeridian; world bounds clamp there.
@Test func cameraBoundsDistinguishUnboundedFromWorldBounds() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 256, height: 256)
  )
  defer { try? map.closeBlockingForTests() }

  func longitudeAfterJump(to longitude: Double) async throws -> Double {
    _ = try await map.updateCamera(CameraUpdate(camera: CameraOptions(
      center: LatLng(latitude: 0, longitude: longitude),
      zoom: 2
    )))
    return try await map.queryCamera().camera.center?.longitude ?? .nan
  }

  #expect(try map.snapshot().bounds.bounds == .unbounded)
  #expect(try await abs(longitudeAfterJump(to: 200) - -160) < 0.000001)

  _ = try await map.setBounds(BoundOptions(bounds: .bounded(LatLngBounds(
    southwest: LatLng(latitude: -90, longitude: -180),
    northeast: LatLng(latitude: 90, longitude: 180)
  ))))

  if case let .bounded(reported) = try map.snapshot().bounds.bounds {
    #expect(abs(reported.northeast.longitude - 180) < 0.000001)
  } else {
    Issue.record("world bounds should report a bounded constraint")
  }
  #expect(try await abs(longitudeAfterJump(to: 200) - 180) < 0.000001)
}

/// The runtime's event-ready wake schedules the host while it is installed and
/// stops once it is cleared.
@Test func eventReadyHandlerRunsUntilItIsCleared() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )
  defer { try? map.closeBlockingForTests() }

  // The wake fires when the queue stops being empty, so start from an empty
  // one: map creation has already queued its own events.
  try await runtime.barrier()
  _ = try runtime.drainEvents()

  let wakes = LockedBox(0)
  runtime.setEventReadyHandler { wakes.update { $0 += 1 } }
  try await map.setStyleJSON(emptyStyleJSON)
  #expect(try await waitUntilTrue("the event wake") { wakes.value > 0 })

  // The style load's own tail can queue one more event, and each newly
  // non-empty queue wakes again, so drain to empty before reading the count
  // the cleared handler has to hold at.
  try await runtime.barrier()
  _ = try runtime.drainEvents()
  runtime.setEventReadyHandler(nil)
  let afterFirstDrain = wakes.value
  _ = try await map.updateCamera(CameraUpdate(camera: CameraOptions(zoom: 5)))
  try await runtime.barrier()
  #expect(try !runtime.drainEvents().isEmpty)
  #expect(wakes.value == afterFirstDrain)
}
