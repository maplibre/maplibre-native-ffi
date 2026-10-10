package org.maplibre.nativeffi.internal.lifecycle

/** Keeps an owner reachable until its native call and copied result have completed. */
internal expect fun bindingKeepAlive(owner: Any)
