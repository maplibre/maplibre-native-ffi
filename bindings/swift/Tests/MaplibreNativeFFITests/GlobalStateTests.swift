#if canImport(Darwin)
  import Darwin
#elseif canImport(Glibc)
  import Glibc
#endif
import Foundation
import MaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

/// Tests that change the process log callback or standard error run in order.
@Suite(.serialized)
struct GlobalStateTests {
  /// Replacing the log callback releases the registration it replaces, and
  /// clearing it releases the last one.
  @Test func replacingTheLogCallbackReleasesThePreviousOne() async throws {
    let records = LockedBox([LogSeverity]())
    let releases = LockedBox(0)
    defer { try? Maplibre.logClearCallback() }

    try Maplibre.logSetCallback(handler: recordingHandler(
      into: records,
      releasing: releases
    ))
    try await withMapFixture { fixture in
      // MapLibre logs the parse failure of a malformed document.
      _ = try? await fixture.map
        .setStyleJson(json: Data(#"{"version":8,"#.utf8))
      await awaitCondition("a logged parse failure") {
        records.value.contains(.error)
      }
    }
    #expect(releases.value == 0)

    try Maplibre.logSetCallback(handler: recordingHandler(
      into: LockedBox([]),
      releasing: releases
    ))
    await awaitCondition("the replaced callback's release") {
      releases.value == 1
    }
    try Maplibre.logClearCallback()
    await awaitCondition("the cleared callback's release") {
      releases.value == 2
    }
  }

  /// Abandoned handles are disposed on a worker and reported once to standard
  /// error.
  @Test func anAbandonedHandleIsDisposedOffTheCallbackStackAndReported(
  ) async throws {
    let capture = try StandardErrorCapture()
    try abandonInsideACallback()
    let failed =
      "Leaked leaky_handle native handle 0x3 (native disposal failed); "
        + "close handles explicitly."
    let disposed =
      "Leaked leaky_handle native handle 0x4; close handles explicitly."
    await awaitCondition("the leak reports") {
      capture.text.contains(failed) && capture.text.contains(disposed)
    }
    capture.finish()
    #expect(capture.text.components(separatedBy: failed).count == 2)
    #expect(capture.text.components(separatedBy: disposed).count == 2)
    let disposals = RecordedHandle.disposals.value.filter { $0.raw != 0x5 }
    #expect(Set(disposals) == [
      RecordedHandle.Disposal(raw: 0x3, insideACallback: false),
      RecordedHandle.Disposal(raw: 0x4, insideACallback: false),
    ])
    #expect(disposals.count == 2)
  }

  /// A created handle that arrives after its wait is cancelled never reached
  /// a caller, so the binding disposes it without reporting a leak.
  @Test func aHandleArrivingAfterItsWaitIsCancelledIsRetiredWithoutAReport(
  ) async throws {
    let capture = try StandardErrorCapture()
    let held = LockedBox<mln_completion?>(nil)
    let future = try NativeCompletion.start({ completion, _ in
      held.update { $0 = completion.pointee }
      return MLN_STATUS_OK
    }) { result in
      try RecordedOwner(raw: NativeCompletion.value(result, as: UInt64.self))
    }
    let waiting = Task { try await future.value() }
    waiting.cancel()
    await #expect(throws: CancellationError.self) { try await waiting.value }

    let descriptor = try #require(held.value)
    withUnsafePointer(to: UInt64(0x5)) { value in
      var result = mln_completion_result()
      result.size = UInt32(MemoryLayout<mln_completion_result>.size)
      result.status = MLN_STATUS_OK.rawValue
      result.value = UnsafeRawPointer(value)
      result.value_count = 1
      descriptor.callback?(descriptor.user_data, &result)
    }
    descriptor.release_user_data?(descriptor.user_data)
    _ = consume future
    _ = consume waiting
    #expect(RecordedHandle.disposals.value.contains(
      RecordedHandle.Disposal(raw: 0x5, insideACallback: false)
    ))
    capture.finish()
    #expect(!capture.text.contains("late_handle"))
  }
}

private func recordingHandler(
  into records: LockedBox<[LogSeverity]>,
  releasing releases: LockedBox<Int>
) -> LogHandler {
  let sentinel = ReleaseProbe(releases)
  return LogHandler { severity, _, _, _ in
    withExtendedLifetime(sentinel) {}
    records.update { $0.append(severity) }
    return 1
  }
}

/// A handle whose disposal fails for 0x3, as one native refuses would, and
/// succeeds otherwise. It records each disposal and whether it ran inside a
/// callback.
private struct RecordedHandle: NativeHandle {
  struct Disposal: Hashable {
    let raw: UInt64
    let insideACallback: Bool
  }

  static let disposals = LockedBox([Disposal]())

  let raw: UInt64

  func disposeAbandoned() -> Bool {
    let inside = Thread.current
      .threadDictionary["MaplibreNativeFFITests.callback"] != nil
    Self.disposals.update {
      $0.append(Disposal(raw: raw, insideACallback: inside))
    }
    return raw != 0x3
  }
}

/// An owner the binding creates from a completion, as a generated creation
/// does.
private final class RecordedOwner: NativeReceiver, @unchecked Sendable {
  let handle: NativeHandleState<RecordedHandle>

  init(raw: UInt64) throws {
    handle = try NativeHandleState(
      typeName: "late_handle",
      handle: RecordedHandle(raw: raw)
    )
  }
}

/// Drops handles whose disposal fails or succeeds.
private func abandonInsideACallback() throws {
  Thread.current.threadDictionary["MaplibreNativeFFITests.callback"] = true
  defer {
    Thread.current.threadDictionary
      .removeObject(forKey: "MaplibreNativeFFITests.callback")
  }
  for raw: UInt64 in [0x3, 0x4] {
    _ = try NativeHandleState(
      typeName: "leaky_handle",
      handle: RecordedHandle(raw: raw)
    )
  }
}

/// Redirects the process's standard error into a pipe until ``finish()``,
/// which restores it and writes everything captured back to it.
private final class StandardErrorCapture: @unchecked Sendable {
  private let saved: Int32
  private let pipe = Pipe()
  private let captured = LockedBox(Data())
  private let drained = DispatchSemaphore(value: 0)

  init() throws {
    // Flushing every stream reaches standard error without naming Glibc's
    // global `stderr`, which strict concurrency rejects as shared state.
    fflush(nil)
    saved = dup(STDERR_FILENO)
    guard saved >= 0,
          dup2(pipe.fileHandleForWriting.fileDescriptor, STDERR_FILENO) >= 0
    else { throw FixtureError("standard error could not be redirected") }
    let reader = pipe.fileHandleForReading
    Thread { [captured, drained] in
      while true {
        let chunk = reader.availableData
        if chunk.isEmpty {
          break
        }
        captured.update { $0.append(chunk) }
      }
      drained.signal()
    }.start()
  }

  var text: String {
    String(decoding: captured.value, as: UTF8.self)
  }

  func finish() {
    fflush(nil)
    dup2(saved, STDERR_FILENO)
    close(saved)
    try? pipe.fileHandleForWriting.close()
    _ = isSignalled(drained)
    FileHandle.standardError.write(captured.value)
  }
}

/// Borrowed copies share the batch, and release fails until the last copy ends.
@Test func borrowedCopiesAllowReadersAndRefuseConcurrentRelease() throws {
  let handle = try NativeHandleState(
    typeName: "test batch", handle: RecordedHandle(raw: 1), protectsCopies: true
  )
  let entered = DispatchSemaphore(value: 0)
  let leave = DispatchSemaphore(value: 0)
  let done = DispatchSemaphore(value: 0)
  DispatchQueue.global().async {
    defer { done.signal() }
    do {
      try handle.withHandle { _ in
        entered.signal()
        #expect(leave.wait(timeout: TestTimeout.deadline()) == .success)
      }
    } catch { Issue.record("reading the batch failed: \(error)") }
  }
  #expect(entered.wait(timeout: TestTimeout.deadline()) == .success)
  defer { leave.signal() }
  #expect(throws: MaplibreError.self) { try handle.closeOnce { _ in } }
  try handle.withHandle { #expect($0.raw == 1) }
  leave.signal()
  #expect(done.wait(timeout: TestTimeout.deadline()) == .success)
  try handle.closeOnce { _ in }
}
