package org.maplibre.nativeffi.internal.lifecycle

import android.annotation.SuppressLint
import android.os.Build
import java.util.concurrent.atomic.AtomicReference

private val ownerFence = AtomicReference<Any?>(null)

@SuppressLint("NewApi")
internal actual fun bindingKeepAlive(owner: Any) {
  if (Build.VERSION.SDK_INT >= 28) {
    java.lang.ref.Reference.reachabilityFence(owner)
  } else {
    // Publishing to shared atomic storage makes this use observable to the collector.
    // Compare-and-set clears only our reference if another thread has published since.
    ownerFence.set(owner)
    ownerFence.compareAndSet(owner, null)
  }
}
