/// The generations that a committed command published, which are zero unless
/// it is a map command. A command throws `MaplibreError` instead when it fails
/// or is cancelled.
public struct CommandCompletion: Sendable, Hashable {
  public let generation: UInt64
  /// The first map render update that includes the command.
  public let renderUpdateGeneration: UInt64
}
