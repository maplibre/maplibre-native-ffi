/**
 * The binding's error family.
 *
 * Every failure — native or binding-owned — arrives as a `MaplibreError` whose
 * `kind` is stable API. The diagnostic string is not: it is the C API's
 * explanation of one call, copied while it still belonged to that call.
 */

import { MLN_STATUS } from "./raw/enums.ts";

/** What went wrong, as a stable value callers may branch on. */
export type ErrorKind =
  /** A pointer, size field, mask, or handle argument was invalid. */
  | "invalidArgument"
  /** The object is valid but not in a state that permits the call. */
  | "invalidState"
  /** A thread-affine handle was used from the wrong thread. */
  | "wrongThread"
  /** The entry point or requested behavior is unavailable in this build. */
  | "unsupported"
  /** A native MapLibre error or C++ exception was converted to status. */
  | "nativeError"
  /** A status this binding does not know. `nativeStatus` carries the value. */
  | "unknownStatus"
  /** The handle was already closed, so the call never reached native code. */
  | "closedHandle"
  /** The handle is being closed on another path. */
  | "releaseInProgress"
  /** A live child handle keeps this handle open. */
  | "childrenLive"
  /** The input could not be represented at the C boundary. */
  | "invalidInput"
  /** The installed runtime payload describes another ABI. */
  | "abiMismatch";

export interface MaplibreErrorOptions {
  /** The `mln_status` value this error was converted from, when there was one. */
  readonly nativeStatus?: number;
  /** The C API's diagnostic, copied inside the failing call. */
  readonly diagnostic?: string;
  /** The public operation that failed. */
  readonly operation?: string;
}

/** Every failure this binding reports. */
export class MaplibreError extends Error {
  override readonly name = "MaplibreError";
  readonly kind: ErrorKind;
  readonly nativeStatus: number | undefined;
  readonly diagnostic: string;
  readonly operation: string | undefined;

  constructor(
    kind: ErrorKind,
    message: string,
    options: MaplibreErrorOptions = {},
  ) {
    super(message);
    this.kind = kind;
    this.nativeStatus = options.nativeStatus;
    this.diagnostic = options.diagnostic ?? "";
    this.operation = options.operation;
  }
}

/** Maps an `mln_status` value to its documented error kind. */
export function errorKindForStatus(status: number): ErrorKind {
  switch (status) {
    case MLN_STATUS.MLN_STATUS_INVALID_ARGUMENT:
      return "invalidArgument";
    case MLN_STATUS.MLN_STATUS_INVALID_STATE:
      return "invalidState";
    case MLN_STATUS.MLN_STATUS_WRONG_THREAD:
      return "wrongThread";
    case MLN_STATUS.MLN_STATUS_UNSUPPORTED:
      return "unsupported";
    case MLN_STATUS.MLN_STATUS_NATIVE_ERROR:
      return "nativeError";
    default:
      // A future library can report a status this build does not name. The raw
      // value is preserved rather than collapsed onto a known kind.
      return "unknownStatus";
  }
}
