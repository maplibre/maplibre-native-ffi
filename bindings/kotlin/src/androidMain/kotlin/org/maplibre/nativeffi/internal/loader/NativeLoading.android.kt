package org.maplibre.nativeffi.internal.loader

import org.maplibre.nativeffi.internal.c.Jni

internal actual fun ensureNativeLibrary() = Jni.ensureLoaded()
