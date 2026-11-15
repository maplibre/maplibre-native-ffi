package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.newSingleThreadContext

/** A thread of its own, separate from the thread that runs the test. */
@OptIn(DelicateCoroutinesApi::class, ExperimentalCoroutinesApi::class)
internal actual val testGraphicsThread: CoroutineDispatcher =
  newSingleThreadContext("maplibre-test-graphics")

/** Only a browser presents to canvases: a session of its own renders offscreen here. */
internal actual fun createTestCanvas(width: Int, height: Int): TestCanvas? = null
