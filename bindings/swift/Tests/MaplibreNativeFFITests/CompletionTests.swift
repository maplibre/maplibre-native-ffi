import Foundation
import MaplibreNativeC
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
      result.status = mln_status(rawValue: status.rawValue)
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

@Test func discardedCommandsAreFencedAndSynchronousRejectionThrows(
) async throws {
  try await withMapFixture { fixture in
    try fixture.map.discarding.setDebugFlags(.tileBorders)
    // A later command on the same map completes after the discarded one.
    _ = try await fixture.map.requestRepaint()
    #expect(try fixture.map.getSnapshot().debugFlags == .tileBorders)
    #expect(throws: MaplibreError.self) {
      try fixture.map.discarding.resize(
        logicalSize: LogicalSize(width: 0, height: 8)
      )
    }
  }
}

/// A completion delivers its converted value to its waiter, and when the
/// conversion of a delivered value fails, the waiter gets the conversion's
/// error.
@Test func aCompletionDeliversItsValueAndAFailedConversionThrows(
) async throws {
  let held = HeldCompletion()
  let delivered = try NativeCompletion.start({ completion, _ in
    held.keep(completion)
    return MLN_STATUS_OK
  }) { result in
    try NativeCompletion.value(result, as: Int32.self)
  }
  held.deliver(value: 42)
  held.release()
  #expect(try await delivered.value() == 42)

  let failingHeld = HeldCompletion()
  let failing = try NativeCompletion.start({ completion, _ in
    failingHeld.keep(completion)
    return MLN_STATUS_OK
  }) { _ -> Int32 in
    throw ConversionFailure()
  }
  failingHeld.deliver()
  failingHeld.release()
  await #expect(throws: ConversionFailure.self) { try await failing.value() }
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
    result.status = MLN_STATUS_OK
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

/// A command that native accepts and then fails throws its status, its
/// diagnostic, and the generation it published.
@Test func aFailedCommandThrowsItsStatusAndGeneration() async throws {
  try await withMapFixture { fixture in
    let committed = try await fixture.map.setStyleJson(emptyStyle)
    let error = await expectMaplibreError(.notFound) {
      try await fixture.map.removeStyleSource("missing")
    }
    #expect(error?.diagnostic.isEmpty == false)
    #expect(error.map { $0.generation > committed.generation } == true)
  }
}

/// Cancelling the task that awaits a completion ends the wait with
/// `CancellationError`, and a result native delivers afterwards finds no
/// waiter.
@Test func cancellingAWaitEndsItWithCancellationError() async throws {
  let held = HeldCompletion()
  let future = try NativeCompletion.start({ completion, _ in
    held.keep(completion)
    return MLN_STATUS_OK
  }) { result in
    try NativeCompletion.value(result, as: Int32.self)
  }
  let waiting = Task { try await future.value() }
  waiting.cancel()
  await #expect(throws: CancellationError.self) { try await waiting.value }

  held.deliver(value: 7)
  held.release()
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
      try await map.updateCamera(update)
    }
    if immediately,
       #available(macOS 26, iOS 26, macCatalyst 26, tvOS 26, visionOS 26, *)
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
