/// Callback registrations that native code in the host supplies.
///
/// A wake or a registration with synchronous callbacks has a `native`
/// constructor, which records its function pointers, context, and release
/// here. The first call that carries the registration copies them into its C
/// descriptor unchanged.
library;

import 'dart:ffi';

import '../../render/native_pointer.dart';

/// The function pointers, context, and release of one native registration.
final class NativeRegistration {
  NativeRegistration(this.callbacks, this.userData, this.releaseUserData);

  /// One function pointer per callback field, in field order.
  final List<NativePointer> callbacks;
  final NativePointer userData;
  final NativePointer releaseUserData;

  /// Whether a call carried this registration. Only the first call may.
  bool consumed = false;

  /// Releases the context of a call that native code did not accept, as
  /// native code releases the context of a call that it accepts.
  void release() {
    if (releaseUserData.isNull) return;
    Pointer<NativeFunction<Void Function(Pointer<Void>)>>.fromAddress(
      releaseUserData.address,
    ).asFunction<void Function(Pointer<Void>)>()(
      Pointer.fromAddress(userData.address),
    );
  }
}

/// The native registration that a public registration value carries, if any.
final nativeRegistrations = Expando<NativeRegistration>('native registration');
