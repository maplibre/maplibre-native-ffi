import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

/// A completion descriptor that a fake native call kept, so a test delivers
/// its result and releases its user data by hand, as native would.
private final class HeldCompletion: @unchecked Sendable {
  private var descriptor: mln_completion?

  func keep(_ completion: UnsafePointer<mln_completion>) {
    descriptor = completion.pointee
  }

  func deliver(status: mln_status = MLN_STATUS_OK, value: Int32 = 0) {
    guard let descriptor else {
      Issue.record("the fake call kept no completion")
      return
    }
    withUnsafePointer(to: value) { value in
      var result = mln_completion_result()
      result.size = UInt32(MemoryLayout<mln_completion_result>.size)
      result.status = status.rawValue
      result.value = UnsafeRawPointer(value)
      result.value_count = 1
      descriptor.callback?(descriptor.user_data, &result)
    }
  }

  func release() {
    guard let descriptor else { return }
    self.descriptor = nil
    descriptor.release_user_data?(descriptor.user_data)
  }
}

private struct ConversionFailure: Error {}

/// A completion delivers its value to the one waiter exactly once, and when
/// the conversion of a delivered value fails, the waiter gets the conversion's
/// error and the bridge state, with everything the converter holds, is freed
/// once native releases it.
@Test func aCompletionDeliversOnceAndAFailedConversionIsDisposed() async throws {
  let held = HeldCompletion()
  let converted = LockedBox(0)
  let delivered = try NativeCompletion.start({ completion, _ in
    held.keep(completion)
    return MLN_STATUS_OK
  }) { result in
    converted.update { $0 += 1 }
    return try NativeCompletion.value(result, as: Int32.self)
  }
  held.deliver(value: 42)
  held.release()
  #expect(try await delivered.value() == 42)
  #expect(converted.value == 1)

  let failingHeld = HeldCompletion()
  let releases = LockedBox(0)
  let failing = try NativeCompletion.start({ completion, _ in
    failingHeld.keep(completion)
    return MLN_STATUS_OK
  }) { [sentinel = ReleaseProbe(releases)] _ -> Int32 in
    withExtendedLifetime(sentinel) {}
    throw ConversionFailure()
  }
  failingHeld.deliver()
  failingHeld.release()
  await #expect(throws: ConversionFailure.self) { try await failing.value() }
  // The future is the last reference to the state and the converter.
  _ = consume failing
  await awaitCondition("the failed conversion's state to be freed") {
    releases.value == 1
  }
}

/// An array result is read at the completion's value_size, which a native
/// build whose element grew reports wider than this binding's element. A
/// stride narrower than the element cannot hold one, so the read throws.
@Test func anArrayResultIsReadAtItsValueSize() throws {
  // Two coordinates, each followed by a member this binding does not know.
  let wide: [Double] = [1, 2, -1, 3, 4, -1]
  try wide.withUnsafeBytes { bytes in
    var result = mln_completion_result()
    result.size = UInt32(MemoryLayout<mln_completion_result>.size)
    result.status = MLN_STATUS_OK.rawValue
    result.value = bytes.baseAddress
    result.value_count = 2
    result.value_size = UInt32(3 * MemoryLayout<Double>.size)
    let points = try withUnsafePointer(to: result) {
      try NativeCompletion.values($0, as: mln_lat_lng.self)
    }
    #expect(points.map(\.latitude) == [1, 3])
    #expect(points.map(\.longitude) == [2, 4])

    result.value_size = UInt32(MemoryLayout<mln_lat_lng>.size - 1)
    #expect(throws: (any Error).self) {
      try withUnsafePointer(to: result) {
        try NativeCompletion.values($0, as: mln_lat_lng.self)
      }
    }
  }
}

/// A submission native rejects throws its status, and the bridge state it
/// never handed over, with the converter's captures, is freed before the
/// throw reaches the caller.
@Test func aRejectedSubmissionFreesItsCompletionState() throws {
  let releases = LockedBox(0)
  #expect(throws: NativeStatusFailure.self) {
    try NativeCompletion.start({ _, _ in MLN_STATUS_INVALID_ARGUMENT }) {
      [sentinel = ReleaseProbe(releases)] _ in
      withExtendedLifetime(sentinel) {}
    }
  }
  #expect(releases.value == 1)
}

/// Cancelling a task that awaits a map creation abandons the creation: the
/// map native creates anyway is disposed with the bridge state, so the runtime
/// is left with no child and closes.
@Test func cancellingACreationWaitRetiresTheCreatedMap() async throws {
  let runtime = try MapFixture.makeRuntime()
  let creation = Task {
    try await runtime.createMap(options: MapOptions(
      initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1)
    ))
  }
  creation.cancel()
  // A creation that finished before the cancellation reached it returns its
  // map, which this test then disposes.
  if let map = try? await creation.value { try map.dispose() }

  var teardown: NativeFuture<Void>?
  try await awaitCondition("the abandoned map to retire") {
    do {
      teardown = try runtime.startClose()
      return true
    } catch let error as MaplibreError where error.kind == .invalidState {
      return false
    }
  }
  try await teardown?.value()
  #expect(runtime.isClosed)
}

/// A command that native accepts and then fails reports the failure as its
/// completion's disposition, status, and diagnostic, instead of throwing.
@Test func aFailedCommandDispositionArrivesAsData() async throws {
  try await withMapFixture { fixture in
    try await fixture.map.setStyleJson(json: emptyStyle)
    let removal = try await fixture.map.removeStyleSource(sourceId: "missing")
    #expect(removal.disposition == .failed)
    #expect(removal.rawStatus == MLN_STATUS_NOT_FOUND.rawValue)
    #expect(!removal.diagnostic.isEmpty)
  }
}

/// Cancelling the task that awaits a completion ends the wait with
/// `CancellationError`, and a result native delivers afterwards is disposed
/// with the bridge state.
@Test func cancellingAWaitEndsItAndDisposesTheLateResult() async throws {
  let held = HeldCompletion()
  let releases = LockedBox(0)
  let future = try NativeCompletion.start({ completion, _ in
    held.keep(completion)
    return MLN_STATUS_OK
  }) { [sentinel = ReleaseProbe(releases)] result in
    withExtendedLifetime(sentinel) {}
    return try NativeCompletion.value(result, as: Int32.self)
  }
  let waiting = Task { try await future.value() }
  waiting.cancel()
  await #expect(throws: CancellationError.self) { try await waiting.value }

  held.deliver(value: 7)
  held.release()
  _ = consume future
  _ = consume waiting
  await awaitCondition("the late result's state to be freed") {
    releases.value == 1
  }
}

/// Commands that tasks on one actor start in order reach native in that order,
/// because an operation submits on its caller's executor before it first
/// suspends. Each committed command publishes the next map generation, so the
/// generations rise with the order the tasks started in. A task that starts
/// immediately submits before its start returns; an enqueued task submits when
/// the main actor runs it, which is in the order the tasks were enqueued.
@Test func commandsStartedInOrderOnOneActorSubmitInThatOrder() async throws {
  try await withMapFixture { fixture in
    let enqueued = try await generationsOfCommandsStarted(
      on: fixture.map,
      immediately: false
    )
    #expect(isStrictlyIncreasing(enqueued), "\(enqueued)")
    let immediate = try await generationsOfCommandsStarted(
      on: fixture.map,
      immediately: true
    )
    #expect(isStrictlyIncreasing(immediate), "\(immediate)")
  }
}

/// Starts one camera command per task on the main actor, in order, and returns
/// each command's published generation in that order.
@MainActor
private func generationsOfCommandsStarted(
  on map: MapHandle,
  immediately: Bool
) async throws -> [UInt64] {
  var tasks: [Task<CommandCompletion, Error>] = []
  for index in 0 ..< 32 {
    let update = CameraUpdate(camera: CameraOptions(zoom: Double(index % 16)))
    let command: @MainActor () async throws -> CommandCompletion = {
      try await map.updateCamera(update: update)
    }
    if immediately,
       #available(macOS 26, iOS 26, macCatalyst 26, tvOS 26, *)
    {
      tasks.append(Task.immediate(operation: command))
    } else {
      tasks.append(Task(operation: command))
    }
  }
  var generations: [UInt64] = []
  for task in tasks {
    try await generations.append(task.value.generation)
  }
  return generations
}

private func isStrictlyIncreasing(_ values: [UInt64]) -> Bool {
  zip(values, values.dropFirst()).allSatisfy { $0 < $1 }
}
