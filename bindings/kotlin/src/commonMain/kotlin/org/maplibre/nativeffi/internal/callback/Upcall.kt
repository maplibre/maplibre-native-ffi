package org.maplibre.nativeffi.internal.callback

/**
 * Runs [block] for an upcall that native cannot receive an exception from, returning [failure] if
 * it throws.
 */
internal inline fun <T> contain(failure: T, block: () -> T): T =
  try {
    block()
  } catch (_: Throwable) {
    failure
  }

/**
 * Runs the host callback of the registration that [token] roots.
 *
 * The callback runs in a [CallbackScope] that admits the [allowed] operations on the handle that
 * [owner] picks, or every operation when [allowed] is null. Native receives [failure] when the
 * registration is gone, holds another type than [V], or the callback throws.
 */
internal inline fun <reified V : Any, T> upcall(
  token: Long,
  failure: T,
  allowed: Set<String>?,
  owner: (CallbackRoot) -> Long? = { null },
  block: (V, CallbackScope) -> T,
): T =
  contain(failure) {
    val root = CallbackRoots.get(token) ?: return failure
    val value = root.value as? V ?: return failure
    CallbackAdmission.scope(owner(root), allowed).use { scope -> block(value, scope) }
  }
