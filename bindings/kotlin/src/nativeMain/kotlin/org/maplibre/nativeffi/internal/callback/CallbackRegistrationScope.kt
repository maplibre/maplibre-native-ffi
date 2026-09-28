@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.internal.callback

import kotlin.native.concurrent.ThreadLocal
import kotlinx.cinterop.*
import org.maplibre.nativeffi.internal.lifecycle.WeakBox

internal actual object CallbackRoots {
  actual fun retain(root: CallbackRoot): Long =
    StableRef.create(WeakBox(root)).asCPointer().rawValue.toLong()

  actual fun get(token: Long): CallbackRoot? =
    token.toCPointer<ByteVar>()?.asStableRef<WeakBox<CallbackRoot>>()?.get()?.get()

  actual fun release(token: Long) {
    val stable = token.toCPointer<ByteVar>()?.asStableRef<WeakBox<CallbackRoot>>() ?: return
    stable.get().get()?.release()
    stable.dispose()
  }
}

@ThreadLocal
internal actual object CallbackContext {
  actual var current: CallbackScope? = null
}
