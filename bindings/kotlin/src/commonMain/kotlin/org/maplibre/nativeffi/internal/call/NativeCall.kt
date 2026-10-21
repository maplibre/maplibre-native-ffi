package org.maplibre.nativeffi.internal.call

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope
import org.maplibre.nativeffi.internal.callback.CallbackScope
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.ReadHandleState
import org.maplibre.nativeffi.internal.lifecycle.ViewScope
import org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive
import org.maplibre.nativeffi.internal.lifecycle.endGraphicsAtExit
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readAddress
import org.maplibre.nativeffi.internal.memory.readI64
import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import org.maplibre.nativeffi.internal.status.checkStatus
import org.maplibre.nativeffi.runtime.CommandCompletion

/** Callback registrations remain rooted only if the native call accepts them. */
internal class NativeCall(val handle: Long, var completion: Long = 0L) : NativeArena() {
  private var scope: CallbackRegistrationScope? = null
  private var arguments: MutableList<Any>? = null

  fun argument(owner: Any, state: HandleState): Long {
    val retained = arguments ?: ArrayList<Any>(1).also { arguments = it }
    retained.add(owner)
    return state.handle
  }

  val diagnostic: Long
    get() = NativeDiagnostics.buffer()

  val registrations: CallbackRegistrationScope
    get() = scope ?: CallbackRegistrationScope().also { scope = it }

  fun check(status: Int) {
    checkStatus(status) { NativeDiagnostics.message(NativeDiagnostics.buffer()) }
  }

  /**
   * Checks [status] like [check], except that [absent] reports that native published no output.
   * Returns whether the output holds a value.
   */
  fun present(status: Int, absent: Int): Boolean {
    if (status == absent) return false
    check(status)
    return true
  }

  fun accept(owner: CallbackOwner) {
    scope?.accept(owner)
  }

  /** Adopts the owned handle that native wrote to [out], disposing it if [create] fails. */
  fun <T> adopt(out: Long, dispose: (Long) -> Unit, create: (Long) -> T): T =
    adoptOwned(readI64(out), dispose, create)

  /**
   * Adopts the owned handle at [out] like [adopt], then roots this call's registrations in the
   * [callbacks] of the new owner, releasing it through [drop] if that fails.
   */
  fun <T> adoptRegistered(
    out: Long,
    dispose: (Long) -> Unit,
    create: (Long) -> T,
    callbacks: (T) -> CallbackOwner,
    drop: (T) -> Unit,
  ): T =
    adopt(out, dispose, create).let { owner -> accept(owner, callbacks(owner)) { drop(owner) } }

  /**
   * Runs a render-session attach: [submit] receives an out slot and a completion, writes the new
   * session to the slot at once, and reports readiness through the completion. A refused call
   * throws before anything is adopted. Otherwise the session is adopted as [adoptRegistered] does,
   * or as [adopt] does when [callbacks] is null, and held for abandonment at process exit. [result]
   * pairs it with the readiness.
   */
  fun <R> attach(
    submit: (out: Long, completion: Long) -> Int,
    dispose: (Long) -> Unit,
    create: (Long) -> RenderSessionHandle,
    callbacks: ((RenderSessionHandle) -> CallbackOwner)?,
    drop: (RenderSessionHandle) -> Unit,
    result: (RenderSessionHandle, Deferred<Unit>) -> R,
  ): R {
    val out = allocate(8)
    val ready = CompletionBridge.unit(this) { completion -> check(submit(out, completion)) }
    val owner =
      if (callbacks == null) adopt(out, dispose, create)
      else adoptRegistered(out, dispose, create, callbacks, drop)
    endGraphicsAtExit(owner.binding)
    return result(owner, ready)
  }

  /**
   * Runs [block] on a borrowed view: [begin] opens it and writes its token to a slot, and [end]
   * closes it with that token. Values read through the [ViewScope] stop working once [block]
   * returns.
   */
  inline fun <T> borrowView(
    begin: (token: Long) -> Int,
    end: (Long) -> Unit,
    block: (ViewScope) -> T,
  ): T {
    val token = allocate(8)
    check(begin(token))
    val scope = ViewScope()
    try {
      return block(scope)
    } finally {
      scope.close()
      end(readAddress(token))
    }
  }

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
    try {
      try {
        scope?.close()
      } finally {
        super.close()
      }
    } finally {
      arguments?.forEach { bindingKeepAlive(it) }
    }
  }
}

/** Runs [body] on [state]'s handle, keeping [owner] reachable until it returns. */
private inline fun <T> receive(owner: Any?, state: HandleState?, body: (Long) -> T): T {
  try {
    ensureNativeLibrary()
    return body(state?.handle ?: 0L)
  } finally {
    if (owner != null) bindingKeepAlive(owner)
  }
}

internal fun <T> nativeCall(owner: Any?, state: HandleState?, body: NativeCall.() -> T): T =
  receive(owner, state) { handle -> NativeCall(handle).use { it.body() } }

/**
 * Runs a call like [nativeCall] that copies outputs native lends until [state]'s release, which
 * fails until the copy ends.
 */
internal fun <T> nativeRead(owner: Any, state: ReadHandleState, body: NativeCall.() -> T): T =
  receive(owner, state) { handle -> state.read { NativeCall(handle).use { it.body() } } }

internal fun <T> nativeSubmit(
  owner: Any?,
  state: HandleState?,
  decode: (Long) -> T,
  callbacks: CallbackOwner? = null,
  body: NativeCall.() -> Unit,
): Deferred<T> =
  receive(owner, state) { handle ->
    NativeCall(handle).use { call ->
      CompletionBridge.submit(call, decode) { completion -> run(call, completion, callbacks, body) }
    }
  }

internal fun nativeUnit(
  owner: Any?,
  state: HandleState?,
  callbacks: CallbackOwner? = null,
  body: NativeCall.() -> Unit,
): Deferred<Unit> =
  receive(owner, state) { handle ->
    NativeCall(handle).use { call ->
      CompletionBridge.unit(call) { completion -> run(call, completion, callbacks, body) }
    }
  }

/**
 * Submits an ordered command. A rejected submission throws; a terminal failure arrives as data in
 * its completion.
 */
internal fun nativeCommand(
  owner: Any?,
  state: HandleState?,
  callbacks: CallbackOwner? = null,
  body: NativeCall.() -> Unit,
): Deferred<CommandCompletion> =
  receive(owner, state) { handle ->
    NativeCall(handle).use { call ->
      CompletionBridge.command(call) { completion -> run(call, completion, callbacks, body) }
    }
  }

internal fun nativeDiscard(
  owner: Any?,
  state: HandleState?,
  callbacks: CallbackOwner? = null,
  body: NativeCall.() -> Unit,
) {
  receive(owner, state) { handle ->
    NativeCall(handle).use { call -> run(call, 0L, callbacks, body) }
  }
}

/**
 * Submits a C operation whose completion carries a new owned handle: [adopt] wraps it, [dispose]
 * releases one nobody adopted, and [drop] releases a wrapper whose caller stopped waiting.
 */
internal fun <T> nativeSubmitOwned(
  owner: Any?,
  state: HandleState?,
  adopt: (Long) -> T,
  dispose: (Long) -> Unit,
  drop: (T) -> Unit,
  body: NativeCall.() -> Unit,
): Deferred<T> =
  receive(owner, state) { handle ->
    NativeCall(handle).use { call ->
      CompletionBridge.submitOwned(
        call,
        { result -> adopt(readI64(CompletionBridge.value(result))) },
        drop,
        { result -> dispose(readI64(CompletionBridge.value(result))) },
      ) { completion ->
        run(call, completion, null, body)
      }
    }
  }

private fun run(
  call: NativeCall,
  completion: Long,
  callbacks: CallbackOwner?,
  body: NativeCall.() -> Unit,
) {
  call.completion = completion
  call.body()
  if (callbacks != null) call.accept(callbacks)
}

/**
 * Runs a C operation on a callback response at [address], which [scope] keeps valid only on the
 * callback's thread while it runs. The body passes [address] as [NativeCall.handle].
 */
internal fun <T> nativeRespond(scope: CallbackScope, address: Long, body: NativeCall.() -> T): T {
  ensureNativeLibrary()
  scope.ensureActive()
  return NativeCall(address).use { it.body() }
}

/** Runs [state]'s synchronous release, which does nothing once a release has ended the handle. */
internal fun nativeClose(owner: Any, state: HandleState, body: NativeCall.() -> Unit) {
  try {
    if (state.isReleased) return
    state.release { NativeCall(state.handle).use { it.body() } }
  } finally {
    bindingKeepAlive(owner)
  }
}

/** Starts [state]'s asynchronous release, whose completion keeps [owner] reachable. */
internal fun nativeRetire(
  owner: Any,
  state: HandleState,
  body: NativeCall.() -> Unit,
): Deferred<Unit> =
  try {
    state.release {
      NativeCall(state.handle).use { call ->
        CompletionBridge.submit(call, { bindingKeepAlive(owner) }) { completion ->
          run(call, completion, null, body)
        }
      }
    }
  } finally {
    bindingKeepAlive(owner)
  }
