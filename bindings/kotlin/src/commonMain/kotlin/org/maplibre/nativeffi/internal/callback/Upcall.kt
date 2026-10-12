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
 * type.
 *
 * The callback runs in a [CallbackScope] that admits the [allowed] operations on the handle that
 * [owner] picks, or every operation when [allowed] is null. Native receives [failure] when the
 * registration is gone, holds another type than [V], or the callback throws, and the binding
 * reports what the callback threw.
 */
internal inline fun <reified V : Any, T> upcall(
  callback: String,
  token: Long,
  failure: T,
  allowed: Set<String>?,
  owner: (CallbackRoot) -> Long? = { null },
  block: (V, CallbackScope) -> T,
): T =
  contain(callback, failure) {
    val root = CallbackRoots.get(token) ?: return failure
    val value = root.value as? V ?: return failure
    CallbackAdmission.scope(owner(root), allowed).use { scope -> block(value, scope) }
  }
