package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.newSingleThreadContext

/** A thread of its own, separate from the thread that runs the test. */
@OptIn(DelicateCoroutinesApi::class, ExperimentalCoroutinesApi::class)
internal actual val testGraphicsThread: CoroutineDispatcher =
  newSingleThreadContext("maplibre-test-graphics")
