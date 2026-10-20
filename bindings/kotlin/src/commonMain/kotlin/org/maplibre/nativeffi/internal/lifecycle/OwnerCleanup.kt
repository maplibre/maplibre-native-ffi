package org.maplibre.nativeffi.internal.lifecycle

/**
 * Disposes and reports [state]'s handle once [owner] becomes unreachable unreleased.
 *
 * The owner keeps the returned value in a field, which ties the cleanup to the owner's lifetime on
 * a platform whose cleaner is itself a collected object.
 */
internal expect fun disposeWhenUnreachable(owner: Any, state: HandleState): Any
