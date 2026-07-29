import '../../error/maplibre_exception.dart';

final BigInt _uint64Modulus = BigInt.one << 64;
final BigInt _maxSignedInt64 = (BigInt.one << 63) - BigInt.one;
final BigInt _maxUnsignedInt64 = _uint64Modulus - BigInt.one;

/// Copies a native `uint64_t` bit pattern into the full Dart integer domain.
BigInt uint64FromNative(int value) {
  final bits = BigInt.from(value);
  return value < 0 ? bits + _uint64Modulus : bits;
}

/// Materializes a full-range Dart unsigned integer as native `uint64_t` bits.
int uint64ToNative(BigInt value, String name) {
  if (value < BigInt.zero || value > _maxUnsignedInt64) {
    throw MaplibreException.invalidArgument(
      '$name must be between 0 and $_maxUnsignedInt64',
    );
  }
  final signedBits = value > _maxSignedInt64 ? value - _uint64Modulus : value;
  return signedBits.toInt();
}
