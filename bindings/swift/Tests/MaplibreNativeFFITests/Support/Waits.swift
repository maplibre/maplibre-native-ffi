import Dispatch
import Foundation
import Testing

/// Every wait gives up at a deadline: 10 seconds times
/// `MLN_TEST_TIMEOUT_SCALE`, which the runners set above 1 on simulators and
/// software renderers, as they do for the C suite.
enum TestTimeout {
  static let scale: Double = {
    guard let text = ProcessInfo.processInfo
      .environment["MLN_TEST_TIMEOUT_SCALE"],
      let value = Double(text), value > 0 else { return 1 }
    return value
  }()

  static func deadline(seconds: Double = 10) -> DispatchTime {
    .now() + seconds * scale
  }
}

/// A process-wide counter that every wake the fixtures install bumps: the
/// runtime's event wake, a session's frame and driver-work wakes, and the
/// flags a test's callbacks set. A waiter blocks until the counter moves, so it
/// wakes as soon as the library or a callback publishes.
final class Pulse: @unchecked Sendable {
  static let shared = Pulse()

  private let condition = NSCondition()
  private var count: UInt64 = 0
  private var nextWaiter: UInt64 = 0
  private var waiters: [UInt64: CheckedContinuation<Void, Never>] = [:]

  var generation: UInt64 {
    condition.withLock { count }
  }

  func signal() {
    let resumed = condition.withLock {
      count &+= 1
      condition.broadcast()
      let resumed = Array(waiters.values)
      waiters.removeAll()
      return resumed
    }
    for waiter in resumed {
      waiter.resume()
    }
  }

  /// Suspends until a signal after `seen`, or until `limit`.
  func wait(past seen: UInt64, until limit: DispatchTime) async {
    await withCheckedContinuation { continuation in
      let id: UInt64? = condition.withLock {
        guard count == seen else { return nil }
        nextWaiter += 1
        waiters[nextWaiter] = continuation
        return nextWaiter
      }
      guard let id else {
        continuation.resume()
        return
      }
      DispatchQueue.global().asyncAfter(deadline: limit) { [self] in
        let waiter = condition.withLock { waiters.removeValue(forKey: id) }
        waiter?.resume()
      }
    }
  }
}

/// How often a waiter re-checks its condition without a signal, which covers
/// state the library publishes without a wake, such as a finalizer that ran on
/// another thread.
private let recheckInterval = DispatchTimeInterval.milliseconds(20)

/// Waits until `condition` holds, re-checking it on every pulse. Records an
/// issue naming `subject` and returns false at the deadline.
@discardableResult
func awaitCondition(
  _ subject: String,
  seconds: Double = 10,
  sourceLocation: SourceLocation = #_sourceLocation,
  _ condition: () throws -> Bool
) async rethrows -> Bool {
  let deadline = TestTimeout.deadline(seconds: seconds)
  while true {
    let seen = Pulse.shared.generation
    if try condition() { return true }
    let now = DispatchTime.now()
    guard now < deadline else {
      Issue.record(
        "timed out waiting for \(subject)",
        sourceLocation: sourceLocation
      )
      return false
    }
    await Pulse.shared.wait(
      past: seen,
      until: min(deadline, now + recheckInterval)
    )
  }
}

/// A lock-guarded value that a callback writes on a native thread and the test
/// reads on its own. Every write pulses, so a test awaits the value it expects.
final class LockedBox<Value>: @unchecked Sendable {
  private let lock = NSLock()
  private var stored: Value

  init(_ initial: Value) {
    stored = initial
  }

  var value: Value {
    lock.withLock { stored }
  }

  func update(_ body: (inout Value) -> Void) {
    lock.withLock { body(&stored) }
    Pulse.shared.signal()
  }
}

/// Counts its own deallocation, so a test observes when the closure that
/// captured it was released.
final class ReleaseProbe: Sendable {
  private let releases: LockedBox<Int>

  init(_ releases: LockedBox<Int>) {
    self.releases = releases
  }

  deinit {
    releases.update { $0 += 1 }
  }
}

/// Waits for `semaphore` until the deadline and reports whether it was
/// signalled, for a test that hands work to a thread it owns.
/// `DispatchSemaphore.wait(timeout:)` is unavailable from an async context, so
/// this synchronous wrapper carries the wait.
func isSignalled(_ semaphore: DispatchSemaphore) -> Bool {
  semaphore.wait(timeout: TestTimeout.deadline()) == .success
}
