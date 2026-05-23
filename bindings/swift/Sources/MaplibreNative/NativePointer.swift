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
