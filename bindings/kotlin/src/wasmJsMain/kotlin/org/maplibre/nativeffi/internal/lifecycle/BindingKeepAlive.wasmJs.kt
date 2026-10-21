package org.maplibre.nativeffi.internal.lifecycle

// A finalizer runs only as a task of its own, never during a C call, so a call keeps its owner
// reachable without help.
internal actual fun bindingKeepAlive(owner: Any) {}
