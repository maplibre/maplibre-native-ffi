internal import CMaplibreNativeC
import Foundation

extension RuntimeHandle {
  /// Closes and waits for native teardown without an async context, so a test
  /// leaves no native thread running past its own end.
  func closeBlockingForTests() throws {
    guard let teardown = try startClose() else { return }
    try mapNativeFailure { try teardown.valueBlocking() }
  }
}

public struct CommandCompletion: Sendable, Hashable {
  public let disposition: CommandDisposition
  public let generation: UInt64
  public let rawStatus: Int32
  public let diagnostic: String
}
