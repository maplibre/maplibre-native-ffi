import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

private func makeRuntime() throws -> RuntimeHandle {
  try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
}

private func makeMap(_ runtime: RuntimeHandle) async throws -> MapHandle {
  try await runtime.mapCreate(options: MapOptions(initialExtent: LogicalExtent(
    width: 64,
    height: 64,
    scaleFactor: 1
  )))
}

/// A map-sourced event names its map by the id the C API delivered,
/// whether or not this process still holds a ``MapHandle`` for it, so a host
/// keeps the identity it needs to route or forward the event.
@Test func aMapSourcedEventKeepsAnIdNoLiveMapHandleClaims() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  let batch = try withSynthesizedEventBatch(
    events: [rawRuntimeEvent(type: 4, source: 0xDEAD_BEEF)]
  ) { synthesized in
    try RuntimeEventBatchView(raw: synthesized.batch)
  }

  let decoded = try #require(batch.events.first)
  #expect(decoded.sourceType == .map && decoded.source == 0xDEAD_BEEF)
  #expect(decoded.sourceType != .map || decoded.source != map.id)
}

// MARK: - The mask type

/// The default parameter takes the C API's own default,
/// bits this build does not name included, and a mask the host writes reaches
/// the struct as it stands.
@Test func mapAndRuntimeOptionsAlwaysEncodeAnEventMask() throws {
  let mapMask: RuntimeEventMask = [.mapStyleLoaded, .mapIdle]
  try withUnsafePointer(to: MapOptions(
    initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1),
    eventMask: mapMask
  ).nativeValue()) { native in
    #expect(native.pointee.event_mask == mapMask.rawValue)
  }
  try withUnsafePointer(to: MapOptions(
    initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1),
    eventMask: []
  ).nativeValue()) { native in
    #expect(native.pointee.event_mask == 0)
  }

  let arena = NativeInputArena()
  let native = try RuntimeOptions(eventMask: .allRuntimeEvents)
    .nativeValue(arena: arena)
  #expect(native.event_mask == RuntimeEventMask.allRuntimeEvents.rawValue)
  #expect(native.event_wake.callback == nil)
}

// MARK: - Draining a live runtime

/// One drain reports every event a style load produced, from the map
/// that produced them.
@Test func oneDrainReportsEveryEventAStyleLoadProduced() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }
  _ = try runtime.drainEventCopies()

  try await map.setStyleJson(json: emptyStyleJSON)
  try await runtime.barrier()
  let batch = try runtime.drainEventCopies()

  #expect(batch.count > 1)
  #expect(batch.map(\.type).contains(.mapStyleLoaded))
  #expect(batch.allSatisfy { $0.sourceType == .map && $0.source == map.id })
}

/// An event copied out of a batch keeps its message and
/// payload after the drain that ends the batch's window.
@Test func anEventTakenOutOfABatchOutlivesTheNextDrain() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }
  _ = try runtime.drainEventCopies()

  _ = try? await map.setStyleJson(json: Data(#"{"version":8,"#.utf8))
  try await runtime.barrier()
  let failure = try #require(
    try runtime.drainEventCopies().first { $0.type == .mapLoadingFailed }
  )
  let message = failure.message
  #expect(!message.isEmpty)

  try await map.setStyleJson(json: emptyStyleJSON)
  try await runtime.barrier()
  let second = try runtime.drainEventCopies()

  #expect(second.map(\.type).contains(.mapStyleLoaded))
  // The drain that ends the first batch's window reuses its arena, so the
  // message has to have been copied out rather than borrowed.
  #expect(failure.message == message)
  #expect(failure.payload == .none)
}

/// A map and a runtime built from the default event mask select every
/// event type and deliver each type the test drives, while a bit outside `all`
/// names no event type and is rejected rather than silently kept.
@Test func theDefaultMaskSelectsEveryEventTypeAndAnUnknownBitIsRejected(
) async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  #expect(try runtime.getEventMask() == .all)
  #expect(try map.snapshotGet().eventMask == .all)

  _ = try runtime.drainEventCopies()
  try await map.setStyleJson(json: emptyStyleJSON)
  _ = try await map
    .updateCamera(update: CameraUpdate(camera: CameraOptions(zoom: 4)))
  try await runtime.barrier()
  let types = try Set(runtime.drainEventCopies().map(\.type))

  #expect(types.contains(.mapStyleLoaded))
  #expect(types.contains(.mapCameraDidChange))
  #expect(types.contains(.mapRenderUpdateAvailable))

  let unnamed = RuntimeEventMask(rawValue: 1 << 63)
  do {
    try runtime.setEventMask(mask: unnamed)
    Issue.record("a runtime mask bit outside all should be rejected")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
  }
  do {
    _ = try await map.setEventMask(mask: unnamed)
    Issue.record("a map mask bit outside all should be rejected")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
  }
  do {
    _ = try await runtime.mapCreate(options: MapOptions(
      initialExtent: LogicalExtent(width: 64, height: 64, scaleFactor: 1),
      eventMask: unnamed
    ))
    Issue.record("a mask bit outside all should be rejected on creation")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
  }
}

/// Once an event is queued, a later mask command does not remove it.
@Test func aQueuedEventSurvivesANarrowedMask() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }
  _ = try runtime.drainEventCopies()

  _ = try await map.setStyleJson(json: emptyStyleJSON)
  try await runtime.barrier()
  _ = try await map
    .setEventMask(mask: RuntimeEventMask.all.subtracting(.mapStyleLoaded))
  try await runtime.barrier()
  #expect(try runtime.drainEventCopies().map(\.type).contains(.mapStyleLoaded))
}

/// A read-modify-write cycle keeps every bit it did not touch, on both handles.
@Test func anEventMaskRoundTripsThroughBothHandles() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  try runtime.setEventMask(mask: .all)
  _ = try await map.setEventMask(mask: .all)
  try await runtime.barrier()
  #expect(try runtime.getEventMask() == .all)
  #expect(try map.snapshotGet().eventMask == .all)

  var mapMask = try map.snapshotGet().eventMask
  mapMask.remove(.mapRenderUpdateAvailable)
  _ = try await map.setEventMask(mask: mapMask)
  try await runtime.barrier()
  let readBackMapMask = try map.snapshotGet().eventMask
  #expect(readBackMapMask == mapMask)
  #expect(readBackMapMask.contains(.mapStyleLoaded))
  #expect(!readBackMapMask.contains(.mapRenderUpdateAvailable))

  var runtimeMask = try runtime.getEventMask()
  runtimeMask.remove(.offlineRegionStatusChanged)
  try runtime.setEventMask(mask: runtimeMask)
  let readBackRuntimeMask = try runtime.getEventMask()
  #expect(readBackRuntimeMask == runtimeMask)
  #expect(!readBackRuntimeMask.contains(.offlineRegionStatusChanged))
}

/// A runtime and a map are usable from a thread other than the one that
/// created them, so a host may hand either between execution contexts.
@Test func runtimeAndMapAreUsableFromAnotherThread() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  let zoom = try await Task.detached {
    try runtime.setEventMask(mask: .all)
    _ = try await map.setEventMask(mask: .all)
    _ = try await map
      .updateCamera(update: CameraUpdate(camera: CameraOptions(zoom: 3)))
    return try await map.cameraQuery().camera.zoom
  }.value
  #expect(zoom == 3)
  #expect(try map.snapshotGet().eventMask == .all)
}
