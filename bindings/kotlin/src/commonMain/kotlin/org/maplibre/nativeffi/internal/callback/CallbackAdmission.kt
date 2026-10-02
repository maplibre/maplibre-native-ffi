package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.internal.status.Status

internal expect object CallbackContext {
  var current: CallbackScope?
}

internal class CallbackScope
internal constructor(
  internal val owner: Long?,
  internal val allowed: Set<String>?,
  internal val parent: CallbackScope?,
) : AutoCloseable {
  private var active = true

  fun ensureActive() {
    var current = CallbackContext.current
    while (current != null && current !== this) current = current.parent
    if (!active || current == null)
      throw Status.invalidState("callback value is outside its invoking thread or scope")
  }

  override fun close() {
    check(CallbackContext.current === this)
    active = false
    CallbackContext.current = parent
  }
}

internal object CallbackAdmission {
  fun scope(owner: Long?, allowed: Set<String>?): CallbackScope =
    CallbackScope(owner, allowed, CallbackContext.current).also { CallbackContext.current = it }

  fun checkOperation(operation: String) {
    var scope = CallbackContext.current
    while (scope != null) {
      if (scope.allowed != null && operation !in scope.allowed)
        throw Status.invalidState(
          "native operation is forbidden by the active callback: $operation"
        )
      scope = scope.parent
    }
  }

  fun check(owner: Long?, operation: String) {
    var scope = CallbackContext.current
    while (scope != null) {
      if (scope.allowed != null && (scope.owner != owner || operation !in scope.allowed))
        throw Status.invalidState(
          "native operation is forbidden by the active callback: $operation"
        )
      scope = scope.parent
    }
  }
}
