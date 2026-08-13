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
  let runtime = try await RuntimeHandle(
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

  try await map.close()
  #expect(map.isClosed)
  try await runtime.close()
  #expect(runtime.isClosed)
}

@Test func cameraSnapshotIsSynchronousAndOrderedQueryObservesCommands(
) async throws {
  let runtime = try await RuntimeHandle(
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
    animation: AnimationOptions(durationMilliseconds: 0),
    gesturePhase: .end,
    gestureId: 9,
    animationId: 10
  ))

  await Task.yield()
  let ordered = try await map.queryCamera()
  #expect(ordered.generation >= before.generation)
  #expect(abs((ordered.camera.zoom ?? 0) - 6) < 0.000001)
}

@Test func requestRepaintReturnsACommandId() async throws {
  let runtime = try await RuntimeHandle(
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
