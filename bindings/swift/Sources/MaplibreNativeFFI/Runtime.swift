public struct CommandCompletion: Sendable, Hashable {
  public let disposition: CommandDisposition
  public let generation: UInt64
  /// The first map render update that includes the command.
  public let renderUpdateGeneration: UInt64
  public let rawStatus: Int32
  public let diagnostic: String
}
