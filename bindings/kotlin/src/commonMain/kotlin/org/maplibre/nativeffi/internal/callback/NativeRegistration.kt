package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicBoolean
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.internal.status.invalidState

/**
 * A callback registration of the host's native functions: one function pointer for each callback
 * field of the C record, in field order, then its `user_data` and `release_user_data`. The binding
 * copies them into the record unchanged.
 *
 * The first call that carries the registration consumes it. Native code releases the registration
 * of a call that it accepts, and the call's [CallbackRegistrationScope] releases it otherwise.
 */
@OptIn(ExperimentalAtomicApi::class)
internal class NativeRegistration(
  val functions: LongArray,
  val userData: Long,
  val releaseUserData: Long,
) {
  private val consumed = AtomicBoolean(false)

  /** Marks the registration taken by a call, or throws when an earlier call took it. */
  fun consume() {
    if (!consumed.compareAndSet(expectedValue = false, newValue = true)) {
      throw invalidState("a native registration belongs to the first call that carries it")
    }
  }

  /** Calls `release_user_data(user_data)`, for a call that native code did not accept. */
  fun release() {
    if (releaseUserData != 0L) callUserDataRelease(releaseUserData, userData)
  }
}

/** Calls [release], an `mln_user_data_release` function pointer, with [userData]. */
internal expect fun callUserDataRelease(release: Long, userData: Long)
