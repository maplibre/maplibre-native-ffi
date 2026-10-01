package org.maplibre.nativeffi.internal.call

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.callback.CallbackAdmission
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope
import org.maplibre.nativeffi.internal.callback.CallbackScope
import org.maplibre.nativeffi.internal.lifecycle.DecisionOwnerState
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.OwnerState
import org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readI64
import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.runtime.CommandCompletion

/**
 * One C call's native storage and contracts, which a generated operation's body runs in.
 *
 * The body allocates its inputs in this arena, passes [handle], [completion], and [diagnostic] to
 * the C function, and wraps a returned status in [check]. Callback registrations made while
 * preparing inputs stay rooted only if the call accepts them.
 */
internal class NativeCall(val handle: Long, val completion: Long = 0L) : NativeArena() {
  private var scope: CallbackRegistrationScope? = null

  /** This thread's diagnostic, sized for the call that reads it next. */
  val diagnostic: Long
    get() = NativeDiagnostics.buffer()

  /** The registrations this call's inputs hand to native. */
  val registrations: CallbackRegistrationScope
    get() = scope ?: CallbackRegistrationScope().also { scope = it }

  /** Throws the mapped exception, with this thread's diagnostic, unless [status] is OK. */
  fun check(status: Int) {
    Status.check(status) { NativeDiagnostics.message(NativeDiagnostics.buffer()) }
  }

  /** Roots the registrations native accepted in [owner]. */
  fun accept(owner: CallbackOwner) {
    scope?.accept(owner)
  }

  /** Adopts the owned handle that native wrote, disposing it if [create] fails. */
  fun <T> adopt(raw: Long, dispose: (Long) -> Unit, create: (Long) -> T): T =
    adoptOwned(raw, dispose, create)

  /** Roots this call's registrations in a new [owner], disposing it if that fails. */
  fun <T> accept(owner: T, callbacks: CallbackOwner, dispose: () -> Unit): T =
    try {
      accept(callbacks)
      owner
    } catch (failure: Throwable) {
      try {
        dispose()
      } catch (cleanup: Throwable) {
        failure.addSuppressed(cleanup)
      }
      throw failure
    }

  override fun close() {
    scope?.close()
    super.close()
  }
}

/** How an operation reaches its receiver's handle. */
internal enum class Access {
  /** Checks that the owner still holds the handle. */
  LIVE,

  /** Holds off a concurrent release while the call copies borrowed outputs. */
  READ,

  /** Passes the handle whatever the owner's state. */
  ISSUED,
}

/** Checks admission for [name] on [state]'s handle, then runs [block] with it. */
private fun <T> receive(
  owner: Any?,
  state: OwnerState?,
  name: String,
  access: Access,
  block: (Long) -> T,
): T {
  try {
    ensureNativeLibrary()
    if (state == null) {
      CallbackAdmission.check(null, name)
      return block(0L)
    }
    if (access == Access.READ) {
      return state.read { handle ->
        CallbackAdmission.check(handle, name)
        block(handle)
      }
    }
    val handle = if (access == Access.ISSUED) state.issued() else state.handle()
    CallbackAdmission.check(handle, name)
    return block(handle)
  } finally {
    if (owner != null) bindingKeepAlive(owner)
  }
}

/** Runs the C operation [name] on [state]'s handle, or on none for a free function. */
internal fun <T> nativeCall(
  owner: Any?,
  state: OwnerState?,
  name: String,
  access: Access = Access.LIVE,
  body: NativeCall.() -> T,
): T = receive(owner, state, name, access) { handle -> NativeCall(handle).use { it.body() } }

/** Submits a C operation that reports through a completion, decoding its result with [decode]. */
internal fun <T> nativeSubmit(
  owner: Any?,
  state: OwnerState?,
  name: String,
  decode: (Long) -> T,
  callbacks: CallbackOwner? = null,
  body: NativeCall.() -> Unit,
): Deferred<T> =
  receive(owner, state, name, Access.LIVE) { handle ->
    CompletionBridge.submit(decode) { completion -> run(handle, completion, callbacks, body) }
  }

/** Submits a C operation whose completion carries no value. */
internal fun nativeUnit(
  owner: Any?,
  state: OwnerState?,
  name: String,
  callbacks: CallbackOwner? = null,
  body: NativeCall.() -> Unit,
): Deferred<Unit> =
  receive(owner, state, name, Access.LIVE) { handle ->
    CompletionBridge.unit { completion -> run(handle, completion, callbacks, body) }
  }

/** Submits an ordered command, whose failure arrives as data. */
internal fun nativeCommand(
  owner: Any?,
  state: OwnerState?,
  name: String,
  callbacks: CallbackOwner? = null,
  body: NativeCall.() -> Unit,
): Deferred<CommandCompletion> =
  receive(owner, state, name, Access.LIVE) { handle ->
    CompletionBridge.command { completion -> run(handle, completion, callbacks, body) }
  }

/**
 * Submits a C operation whose completion carries a new owned handle: [adopt] wraps it, [dispose]
 * releases one nobody adopted, and [drop] releases a wrapper whose caller stopped waiting.
 */
internal fun <T> nativeSubmitOwned(
  owner: Any?,
  state: OwnerState?,
  name: String,
  adopt: (Long) -> T,
  dispose: (Long) -> Unit,
  drop: (T) -> Unit,
  body: NativeCall.() -> Unit,
): Deferred<T> =
  receive(owner, state, name, Access.LIVE) { handle ->
    CompletionBridge.submitOwned(
      { result -> adopt(readI64(CompletionBridge.value(result))) },
      drop,
      { result -> dispose(readI64(CompletionBridge.value(result))) },
    ) { completion ->
      run(handle, completion, null, body)
    }
  }

private fun run(
  handle: Long,
  completion: Long,
  callbacks: CallbackOwner?,
  body: NativeCall.() -> Unit,
) {
  NativeCall(handle, completion).use { call ->
    call.body()
    if (callbacks != null) call.accept(callbacks)
  }
}

/** Runs the C operation [name] that answers [state]'s decision, keeping it retryable on refusal. */
internal fun nativeComplete(
  owner: Any,
  state: DecisionOwnerState,
  name: String,
  body: NativeCall.() -> Unit,
) {
  try {
    ensureNativeLibrary()
    CallbackAdmission.check(state.handle(), name)
    state.complete { NativeCall(state.issued()).use { it.body() } }
  } finally {
    bindingKeepAlive(owner)
  }
}

/**
 * Runs the C operation [name] on a callback response at [address], which [scope] keeps valid only
 * on the callback's thread while it runs.
 */
internal fun <T> nativeRespond(
  scope: CallbackScope,
  address: Long,
  name: String,
  body: NativeCall.() -> T,
): T {
  ensureNativeLibrary()
  scope.ensureActive()
  CallbackAdmission.check(address, name)
  return NativeCall(0L).use { it.body() }
}

/** Runs [state]'s synchronous release, the C operation [name]. */
internal fun nativeClose(owner: Any, state: OwnerState, name: String, body: NativeCall.() -> Unit) {
  try {
    CallbackAdmission.checkOperation(name)
    state.closeHandle(name) { handle -> NativeCall(handle).use { it.body() } }
  } finally {
    bindingKeepAlive(owner)
  }
}

/** Runs [state]'s asynchronous release, the C operation [name]. */
internal fun nativeRetire(
  owner: Any,
  state: HandleStateCore,
  name: String,
  body: NativeCall.() -> Unit,
): Deferred<Unit> =
  try {
    CallbackAdmission.checkOperation(name)
    state.retireHandle { handle ->
      CallbackAdmission.check(handle, name)
      CompletionBridge.unitChecked { completion ->
        NativeCall(handle, completion).use { it.body() }
      }
    }
  } finally {
    bindingKeepAlive(owner)
  }
