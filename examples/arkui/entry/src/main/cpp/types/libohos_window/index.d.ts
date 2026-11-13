// The native module in entry/src/main/cpp/ohos_window.c.

/** Returns a new reference to the window of an XComponent's surface. */
export const windowFromSurfaceId: (surfaceId: bigint) => bigint;

/** Releases a reference that `windowFromSurfaceId` returned. */
export const releaseWindow: (window: bigint) => void;
