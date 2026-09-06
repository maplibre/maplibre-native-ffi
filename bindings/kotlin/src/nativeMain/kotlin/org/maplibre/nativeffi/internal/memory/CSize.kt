@file:OptIn(kotlinx.cinterop.ExperimentalForeignApi::class)

package org.maplibre.nativeffi.internal.memory

import kotlinx.cinterop.convert
import platform.posix.size_t
import platform.posix.size_tVar

internal typealias CSize = size_t

internal typealias CSizeVar = size_tVar

internal fun Int.toCSize(): CSize = convert()

internal fun Long.toCSize(): CSize = convert()

internal fun ULong.toCSize(): CSize = convert()
