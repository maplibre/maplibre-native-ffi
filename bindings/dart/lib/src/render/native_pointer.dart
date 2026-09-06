/// Opaque borrowed native address value used for backend interop handles.
///
/// The value does not own, retain, dereference, or validate the pointed-to
/// object. Passing it to MapLibre Native transfers no ownership and grants the
/// Dart binding no memory access.
final class NativePointer {
  /// Creates an opaque borrowed pointer value from an address bit pattern.
  const NativePointer(this.address);

  /// Null native pointer value.
  static const nullPointer = NativePointer(0);

  /// Address bit pattern for this borrowed native pointer.
  final int address;

  /// Returns true when this pointer represents a null backend handle.
  bool get isNull => address == 0;

  @override
  bool operator ==(Object other) =>
      other is NativePointer && other.address == address;

  @override
  int get hashCode => address.hashCode;

  @override
  String toString() => 'NativePointer[address=0x${address.toRadixString(16)}]';
}

/// Opaque borrowed Vulkan non-dispatchable handle with a stable 64-bit width.
final class VulkanHandle {
  /// Creates a Vulkan handle from its unsigned 64-bit representation.
  VulkanHandle(this.bits) {
    if (bits.isNegative || bits.bitLength > 64) {
      throw ArgumentError.value(
        bits,
        'bits',
        'must fit in an unsigned 64-bit value',
      );
    }
  }

  /// Null Vulkan handle.
  static final nullHandle = VulkanHandle(BigInt.zero);

  /// Unsigned 64-bit Vulkan handle representation.
  final BigInt bits;

  /// Returns true when this is a null Vulkan handle.
  bool get isNull => bits == BigInt.zero;

  @override
  bool operator ==(Object other) => other is VulkanHandle && other.bits == bits;

  @override
  int get hashCode => bits.hashCode;

  @override
  String toString() => 'VulkanHandle[bits=0x${bits.toRadixString(16)}]';
}

/// Native pointer value that is valid only while an owner-defined scope is live.
final class ScopedNativePointer {
  /// Creates a scoped native pointer.
  const ScopedNativePointer(
    this._address, {
    required void Function() checkValid,
    required String debugName,
  }) : _checkValid = checkValid,
       _debugName = debugName;

  final int _address;
  final void Function() _checkValid;
  final String _debugName;

  /// Pointer address after validating that the scope is still live.
  int get address {
    _checkValid();
    return _address;
  }

  /// Returns true when this pointer represents a null backend handle.
  bool get isNull => address == 0;

  /// Copies the current address into an unscoped [NativePointer].
  NativePointer toNativePointer() => NativePointer(address);

  @override
  String toString() =>
      'ScopedNativePointer[$_debugName,address=0x${address.toRadixString(16)}]';
}

/// Vulkan handle value that is valid only while an owner-defined scope is live.
final class ScopedVulkanHandle {
  /// Creates a scoped Vulkan handle.
  ScopedVulkanHandle(
    this._bits, {
    required void Function() checkValid,
    required String debugName,
  }) : _checkValid = checkValid,
       _debugName = debugName;

  final BigInt _bits;
  final void Function() _checkValid;
  final String _debugName;

  /// Unsigned 64-bit handle representation after validating the scope.
  BigInt get bits {
    _checkValid();
    return _bits;
  }

  /// Returns true when this is a null Vulkan handle.
  bool get isNull => bits == BigInt.zero;

  /// Copies the current bits into an unscoped [VulkanHandle].
  VulkanHandle toVulkanHandle() => VulkanHandle(bits);

  @override
  String toString() =>
      'ScopedVulkanHandle[$_debugName,bits=0x${bits.toRadixString(16)}]';
}

/// Native integer value that is valid only while an owner-defined scope is live.
final class ScopedNativeInt {
  /// Creates a scoped native integer.
  const ScopedNativeInt(
    this._value, {
    required void Function() checkValid,
    required String debugName,
  }) : _checkValid = checkValid,
       _debugName = debugName;

  final int _value;
  final void Function() _checkValid;
  final String _debugName;

  /// Integer value after validating that the scope is still live.
  int get value {
    _checkValid();
    return _value;
  }

  @override
  String toString() => 'ScopedNativeInt[$_debugName,value=$value]';
}
