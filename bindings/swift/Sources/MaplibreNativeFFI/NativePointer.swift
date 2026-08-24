import Foundation

public struct NativePointer: Sendable, Hashable, CustomStringConvertible {
  public static let null = NativePointer(bitPattern: 0)

  let bitPattern: UInt

  public init(bitPattern: UInt) {
    self.bitPattern = bitPattern
  }

  public var addressBitPattern: UInt {
    bitPattern
  }

  public var isNull: Bool {
    bitPattern == 0
  }

  public var description: String {
    "NativePointer(address: 0x\(String(bitPattern, radix: 16)))"
  }

  var unsafeRawPointer: UnsafeRawPointer? {
    UnsafeRawPointer(bitPattern: bitPattern)
  }

  var unsafeMutableRawPointer: UnsafeMutableRawPointer? {
    UnsafeMutableRawPointer(bitPattern: bitPattern)
  }
}

public struct VulkanHandle: Sendable, Hashable, CustomStringConvertible {
  public static let null = VulkanHandle(bitPattern: 0)

  public let bitPattern: UInt64

  public init(bitPattern: UInt64) {
    self.bitPattern = bitPattern
  }

  public var isNull: Bool {
    bitPattern == 0
  }

  public var description: String {
    "VulkanHandle(bits: 0x\(String(bitPattern, radix: 16)))"
  }
}

final class NativeFrameScope: @unchecked Sendable {
  private let lock = NSLock()
  private let isFrameLive: () -> Bool
  private var active = true

  init(isFrameLive: @escaping () -> Bool) {
    self.isFrameLive = isFrameLive
  }

  func close() {
    lock.withLock {
      active = false
    }
  }

  func requireActive(_ diagnostic: String) throws {
    let isActive = lock.withLock { active }
    guard isActive else {
      throw MaplibreError(
        kind: .invalidState,
        rawStatus: nil,
        diagnostic: "\(diagnostic) scope has ended"
      )
    }
    guard isFrameLive() else {
      throw MaplibreError(
        kind: .invalidState,
        rawStatus: nil,
        diagnostic: "\(diagnostic) frame is closed"
      )
    }
  }
}

public struct FrameNativePointer: Sendable {
  let bitPattern: UInt
  private let scope: NativeFrameScope
  private let diagnosticName: String

  init(bitPattern: UInt, scope: NativeFrameScope, diagnosticName: String) {
    self.bitPattern = bitPattern
    self.scope = scope
    self.diagnosticName = diagnosticName
  }

  public var addressBitPattern: UInt {
    get throws {
      try scope.requireActive(diagnosticName)
      return bitPattern
    }
  }

  public var isNull: Bool {
    get throws {
      try scope.requireActive(diagnosticName)
      return bitPattern == 0
    }
  }
}

public struct FrameVulkanHandle: Sendable {
  private let bitPattern: UInt64
  private let scope: NativeFrameScope
  private let diagnosticName: String

  init(bitPattern: UInt64, scope: NativeFrameScope, diagnosticName: String) {
    self.bitPattern = bitPattern
    self.scope = scope
    self.diagnosticName = diagnosticName
  }

  public var bits: UInt64 {
    get throws {
      try scope.requireActive(diagnosticName)
      return bitPattern
    }
  }

  public var isNull: Bool {
    get throws {
      try scope.requireActive(diagnosticName)
      return bitPattern == 0
    }
  }
}

public struct FrameOpenGLTextureName: Sendable {
  private let name: UInt32
  private let scope: NativeFrameScope

  init(_ name: UInt32, scope: NativeFrameScope) {
    self.name = name
    self.scope = scope
  }

  public var value: UInt32 {
    get throws {
      try scope.requireActive("OpenGL texture")
      return name
    }
  }

  public var isZero: Bool {
    get throws {
      try scope.requireActive("OpenGL texture")
      return name == 0
    }
  }
}
