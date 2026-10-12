package org.maplibre.nativeffi.internal.lifecycle

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
internal actual fun trackLeak(owner: Any, report: HandleStateCore.LeakReport): Any =
  createCleaner(report) { it.report() }

@OptIn(ExperimentalNativeApi::class)
internal actual fun closeWhenUnreachable(owner: Any, core: DecisionOwnerCore): Any =
  createCleaner(core) { it.close() }
