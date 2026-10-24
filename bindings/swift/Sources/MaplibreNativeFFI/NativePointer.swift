/// The address of a native object. A pointer made with `init(retaining:)`
/// also keeps its object alive for as long as the pointer, or a record that
/// holds it, exists.
public struct NativePointer: @unchecked Sendable, Hashable,
  CustomStringConvertible
{
  public static let null = NativePointer(bitPattern: 0)

  let bitPattern: UInt
  /// Only held, never read, so sharing it across threads is safe.
  private let owner: AnyObject?

  public init(bitPattern: UInt) {
    self.bitPattern = bitPattern
    owner = nil
  }

  /// Points at `object` and keeps it alive while this pointer exists, so a
  /// record that carries it into a native call keeps it alive for the call.
  public init(retaining object: AnyObject) {
    bitPattern = UInt(bitPattern: Unmanaged.passUnretained(object).toOpaque())
    owner = object
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

  public static func == (lhs: NativePointer, rhs: NativePointer) -> Bool {
    lhs.bitPattern == rhs.bitPattern
  }

  public func hash(into hasher: inout Hasher) {
    hasher.combine(bitPattern)
  }

  var unsafeRawPointer: UnsafeRawPointer? {
    UnsafeRawPointer(bitPattern: bitPattern)
  }

  var unsafeMutableRawPointer: UnsafeMutableRawPointer? {
    UnsafeMutableRawPointer(bitPattern: bitPattern)
  }

  /// The object at this address, which native lends without a retain.
  func unretainedObject() -> AnyObject? {
    unsafeRawPointer.map {
      Unmanaged<AnyObject>.fromOpaque($0).takeUnretainedValue()
    }
  }
}
