package org.maplibre.nativeffi.internal.lifecycle

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
internal actual fun disposeWhenUnreachable(owner: Any, state: HandleState): Any =
  createCleaner(state) { it.disposeLeaked() }
