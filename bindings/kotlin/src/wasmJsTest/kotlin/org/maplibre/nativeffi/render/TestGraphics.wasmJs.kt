package org.maplibre.nativeffi.render

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers

// The page is the only thread that runs Kotlin, so it drives graphics too.
internal actual val testGraphicsThread: CoroutineDispatcher = Dispatchers.Default
