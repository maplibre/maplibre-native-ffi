public struct RenderBackend: OptionSet, Sendable, Hashable {
  public let rawValue: UInt32

  public init(rawValue: UInt32) {
    self.rawValue = rawValue
  }

  public static let metal = Self(rawValue: 1 << 0)
  public static let vulkan = Self(rawValue: 1 << 1)
  public static let openGL = Self(rawValue: 1 << 2)
}

public struct OpenGLContextProvider: OptionSet, Sendable, Hashable {
  public let rawValue: UInt32

  public init(rawValue: UInt32) {
    self.rawValue = rawValue
  }

  public static let wgl = Self(rawValue: 1 << 0)
  public static let egl = Self(rawValue: 1 << 1)
}
