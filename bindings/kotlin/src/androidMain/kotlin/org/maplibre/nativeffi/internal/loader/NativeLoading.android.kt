package org.maplibre.nativeffi.internal.loader

import org.maplibre.nativeffi.internal.c.Jni

internal actual fun ensureNativeLibrary() = Jni.ensureLoaded()

/** Each class with natives loads the shim before its first call. */
internal actual fun loadNativeLibraryUnchecked() {}
