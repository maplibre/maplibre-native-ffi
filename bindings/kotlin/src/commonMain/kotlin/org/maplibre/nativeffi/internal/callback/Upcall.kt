package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.error.MaplibreException

/**
 * Runs [block] for an upcall of the C [callback] type, which native cannot receive an exception
 * from. Returns [failure] and reports the exception if [block] throws.
 */
internal inline fun <T> contain(callback: String, failure: T, block: () -> T): T =
  try {
    block()
  } catch (error: Throwable) {
    reportCallbackFailure(callback, error)
    failure
  }

/**
 * Runs the host callback of the registration that [token] roots, for an upcall of the C [callback]
 * type. Native receives [failure] when the registration is gone, holds another type than [V], or
 * the callback throws, and the binding reports what the callback threw.
 */
internal inline fun <reified V : Any, T> upcall(
  callback: String,
  token: Long,
  failure: T,
  block: (V) -> T,
): T =
  contain(callback, failure) {
    val root = CallbackRoots.get(token) ?: return failure
    val value = root.value as? V ?: return failure
    block(value)
  }

/**
 * Runs the host callback of the registration that [token] roots, for an upcall of the C [callback]
 * type that returns an `mln_status`. The host callback returns nothing and throws to fail, so
 * [block] returns `MLN_STATUS_OK` after it. Native receives [failure] when the registration is gone
 * or holds another type than [V]. A thrown [MaplibreException] that carries a failure status is the
 * host's answer: native receives that status, and the binding reports nothing. Any other throwable
 * is a bug: native receives [failure], and the binding reports what the callback threw.
 */
internal inline fun <reified V : Any> statusUpcall(
  callback: String,
  token: Long,
  failure: Int,
  block: (V) -> Int,
): Int =
  try {
    val root = CallbackRoots.get(token) ?: return failure
    val value = root.value as? V ?: return failure
    block(value)
  } catch (error: Throwable) {
    val status = (error as? MaplibreException)?.status?.rawValue?.takeIf { it != 0 }
    if (status == null) reportCallbackFailure(callback, error)
    status ?: failure
  }
