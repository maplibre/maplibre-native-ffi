package org.maplibre.nativeffi.internal.lifecycle

// The browser ends every agent of a page together, and abandoning sessions on pagehide would break
// a restore from the back-forward cache, so nothing holds sessions for exit.
internal actual fun endGraphicsAtExit(state: HandleState) {}
