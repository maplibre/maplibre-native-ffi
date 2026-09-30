import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

/// Drains until an event `isMatch` accepts arrives, and returns it.
/// Records an issue naming `subject` and returns `nil` at the deadline.
func drainUntilEvent(
  _ runtime: RuntimeHandle,
  waitingFor subject: String,
  timeout: TimeInterval = 10,
  where isMatch: (RuntimeEvent) -> Bool
) async throws -> RuntimeEvent? {
  let deadline = Date().addingTimeInterval(timeout)
  while Date() < deadline {
    try await runtime.barrier()
    for event in try runtime.drainEventCopies() where isMatch(event) {
      return event
    }
    try await Task<Never, Never>.sleep(nanoseconds: 1_000_000)
  }
  Issue.record("timed out waiting for \(subject)")
  return nil
}

func expectCommandFailure(_ completion: CommandCompletion, status: mln_status) {
  #expect(completion.disposition == .failed)
  #expect(completion.rawStatus == status.rawValue)
  #expect(!completion.diagnostic.isEmpty)
}

/// Polls until `condition` holds while the runtime's own threads make
/// progress. Records an issue naming `subject` and returns false at the
/// deadline.
func waitUntilTrue(
  _ subject: String,
  timeout: TimeInterval = 10,
  condition: () throws -> Bool
) async throws -> Bool {
  let deadline = Date().addingTimeInterval(timeout)
  while Date() < deadline {
    if try condition() { return true }
    try await Task<Never, Never>.sleep(nanoseconds: 1_000_000)
  }
  if try condition() { return true }
  Issue.record("timed out waiting for \(subject)")
  return false
}

/// A style with no sources, so a load finishes without reaching the network.
let emptyStyleJSON = Data(#"{"version":8,"sources":{},"layers":[]}"#.utf8)

/// Waits on `semaphore`. `DispatchSemaphore.wait(timeout:)` is unavailable
/// from an async context, so a test that has to block calls this from a
/// detached task or another thread.
func waitForSemaphore(
  _ semaphore: DispatchSemaphore,
  timeout: DispatchTime
) -> DispatchTimeoutResult {
  semaphore.wait(timeout: timeout)
}

extension RuntimeHandle {
  /// Closes and waits for native teardown without an async context, so a test
  /// leaves no native thread running past its own end.
  func closeBlockingForTests() throws {
    guard let teardown = try startClose() else { return }
    try mapNativeFailure { try teardown.valueBlocking() }
  }
}

extension MapHandle {
  /// Closes and waits for native teardown without an async context.
  func closeBlockingForTests() throws {
    guard let teardown = try startClose() else { return }
    try mapNativeFailure { try teardown.valueBlocking() }
  }
}
