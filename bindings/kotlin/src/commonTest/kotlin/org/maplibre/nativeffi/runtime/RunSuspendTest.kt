package org.maplibre.nativeffi.runtime

import kotlinx.coroutines.runBlocking

internal fun <T> runSuspendTest(block: suspend () -> T): T = runBlocking { block() }
