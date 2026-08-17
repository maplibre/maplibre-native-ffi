import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

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
  let cameraCommand = try map.updateCamera(CameraUpdate(
    camera: CameraOptions(
      center: LatLng(latitude: 37.7749, longitude: -122.4194),
      zoom: 12
    )
  ))
  let resizeCommand = try map.resize(to: MapLogicalExtent(
    width: 512,
    height: 256,
    scaleFactor: 2
  ))
  #expect(cameraCommand > 0)
  #expect(resizeCommand > cameraCommand)

  let ordered = try await map.queryCamera()
  #expect(abs((ordered.camera.center?.latitude ?? 0) - 37.7749) < 0.000001)
  #expect(abs((ordered.camera.center?.longitude ?? 0) - -122.4194) < 0.000001)
  #expect(abs((ordered.camera.zoom ?? 0) - 12) < 0.000001)

  let resized = try map.snapshot()
  #expect(resized.generation >= ordered.generation)
  #expect(resized.logicalExtent.width == 512)
  #expect(resized.logicalExtent.height == 256)

  try map.close()
  #expect(map.isClosed)
  try runtime.close()
  #expect(runtime.isClosed)
}

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
  _ = try map.updateCamera(CameraUpdate(
    mode: .ease,
    camera: CameraOptions(zoom: 6),
    animation: AnimationOptions(durationMilliseconds: 0, transitionId: 10),
    gesturePhase: .end
  ))

  await Task.yield()
  let ordered = try await map.queryCamera()
  #expect(ordered.generation >= before.generation)
  #expect(abs((ordered.camera.zoom ?? 0) - 6) < 0.000001)
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

  let command = try map.setDebugOptions([.tileBorders])
  let result = try #require(try await commandFinished(
    command, runtime: runtime
  ))
  #expect(result.finished
    .disposition == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)
  #expect(result.finished.generation > 0)

  let snapshot = try map.snapshot()
  #expect(snapshot.generation >= result.finished.generation)
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

  _ = try map.setTileOptions(MapTileOptions(prefetchZoomDelta: 5, lodScale: 2))
  _ = try map.setBounds(BoundOptions(minZoom: 2, maxZoom: 15, maxPitch: 45))
  // The altitude sits inside the bound zoom range, so it is not clamped.
  _ = try map.setFreeCameraOptions(FreeCameraOptions(
    position: Vec3(x: 0.25, y: 0.5, z: 0.01)
  ))
  let statsCommand = try map.setRenderingStatsViewEnabled(true)
  #expect(try await commandDisposition(
    statsCommand, runtime: runtime
  ) == MLN_COMMAND_DISPOSITION_COMMITTED.rawValue)

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

@Test func requestRepaintReturnsACommandId() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64, mode: .continuous)
  )
  defer { try? map.closeBlockingForTests() }

  let command = try map.requestRepaint()
  #expect(command > 0)
  try await runtime.barrier()
}
