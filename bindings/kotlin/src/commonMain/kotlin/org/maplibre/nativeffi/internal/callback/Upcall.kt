package org.maplibre.nativeffi.internal.callback

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
