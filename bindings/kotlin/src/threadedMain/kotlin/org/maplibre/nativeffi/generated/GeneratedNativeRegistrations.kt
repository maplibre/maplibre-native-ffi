// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.callback.NativeRegistration
import org.maplibre.nativeffi.render.NativePointer

/**
 * Creates a `Wake` of the host's native functions, which the binding copies to native code
 * unchanged. Each function has the C type of the record field that it fills, and native code calls
 * each with `userData`. A null pointer leaves its field null.
 *
 * The first call that carries the registration consumes it. Native code calls `releaseUserData`
 * once it accepts the call. When native code does not accept the call, the binding calls
 * `releaseUserData` before the call returns or throws. A later call that carries the registration
 * throws an `IllegalStateException` before it calls native code.
 *
 * Only the JVM, Android, and Kotlin/Native declare this function, because a Kotlin/Wasm page has no
 * native functions of its own.
 *
 * See `mln_wake` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wake_8h.html).
 */
public fun Wake.Companion.native(
  callback: NativePointer,
  userData: NativePointer = NativePointer.NULL_POINTER,
  releaseUserData: NativePointer = NativePointer.NULL_POINTER,
): Wake =
  Wake(
    null,
    NativeRegistration(longArrayOf(callback.address), userData.address, releaseUserData.address),
  )

/**
 * Creates a `QueueLock` of the host's native functions, which the binding copies to native code
 * unchanged. Each function has the C type of the record field that it fills, and native code calls
 * each with `userData`. A null pointer leaves its field null.
 *
 * The first call that carries the registration consumes it. Native code calls `releaseUserData`
 * once it accepts the call. When native code does not accept the call, the binding calls
 * `releaseUserData` before the call returns or throws. A later call that carries the registration
 * throws an `IllegalStateException` before it calls native code.
 *
 * Only the JVM, Android, and Kotlin/Native declare this function, because a Kotlin/Wasm page has no
 * native functions of its own.
 *
 * See `mln_queue_lock` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
public fun QueueLock.Companion.native(
  lock: NativePointer,
  unlock: NativePointer,
  userData: NativePointer = NativePointer.NULL_POINTER,
  releaseUserData: NativePointer = NativePointer.NULL_POINTER,
): QueueLock =
  QueueLock(
    null,
    null,
    NativeRegistration(
      longArrayOf(lock.address, unlock.address),
      userData.address,
      releaseUserData.address,
    ),
  )
