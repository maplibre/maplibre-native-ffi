/**
 * A resource request a provider claimed.
 *
 * MapLibre asked for this resource on one of its own threads. The route that
 * claimed it answered there, and the request itself was copied and handed here,
 * so the handler answers whenever it can: inline, after a fetch, or from another
 * host context.
 *
 * Completion is terminal and happens once. A request the host neither completes
 * nor releases stays outstanding, which is what `close()` prevents.
 */

import { MaplibreError } from "./errors.ts";
import { NamedValue } from "./events.ts";
import type { Native } from "./internal/native.ts";
import { asInt64 } from "./internal/numbers.ts";
import type { Ptr } from "./internal/transport.ts";
import { EP } from "./raw/entrypoints.ts";
import {
  MLN_RESOURCE_ERROR_REASON,
  MLN_RESOURCE_RESPONSE_STATUS,
} from "./raw/enums.ts";
import type { ResourceKind } from "./resources.ts";

/** How a provider answered a request. */
export class ResourceResponseStatus extends NamedValue {
  static readonly ok = new ResourceResponseStatus(
    MLN_RESOURCE_RESPONSE_STATUS.MLN_RESOURCE_RESPONSE_STATUS_OK,
    "ok",
  );
  static readonly noContent = new ResourceResponseStatus(
    MLN_RESOURCE_RESPONSE_STATUS.MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT,
    "noContent",
  );
  static readonly notModified = new ResourceResponseStatus(
    MLN_RESOURCE_RESPONSE_STATUS.MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED,
    "notModified",
  );
  static readonly error = new ResourceResponseStatus(
    MLN_RESOURCE_RESPONSE_STATUS.MLN_RESOURCE_RESPONSE_STATUS_ERROR,
    "error",
  );

  static fromRawValue(rawValue: number): ResourceResponseStatus {
    return (
      [
        ResourceResponseStatus.ok,
        ResourceResponseStatus.noContent,
        ResourceResponseStatus.notModified,
        ResourceResponseStatus.error,
      ].find((value) => value.rawValue === rawValue) ??
      new ResourceResponseStatus(rawValue, `unknown(${rawValue})`)
    );
  }
}

/** Why a request failed. */
export class ResourceErrorReason extends NamedValue {
  static readonly none = new ResourceErrorReason(
    MLN_RESOURCE_ERROR_REASON.MLN_RESOURCE_ERROR_REASON_NONE,
    "none",
  );
  static readonly notFound = new ResourceErrorReason(
    MLN_RESOURCE_ERROR_REASON.MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
    "notFound",
  );
  static readonly server = new ResourceErrorReason(
    MLN_RESOURCE_ERROR_REASON.MLN_RESOURCE_ERROR_REASON_SERVER,
    "server",
  );
  static readonly connection = new ResourceErrorReason(
    MLN_RESOURCE_ERROR_REASON.MLN_RESOURCE_ERROR_REASON_CONNECTION,
    "connection",
  );
  static readonly rateLimit = new ResourceErrorReason(
    MLN_RESOURCE_ERROR_REASON.MLN_RESOURCE_ERROR_REASON_RATE_LIMIT,
    "rateLimit",
  );
  static readonly other = new ResourceErrorReason(
    MLN_RESOURCE_ERROR_REASON.MLN_RESOURCE_ERROR_REASON_OTHER,
    "other",
  );

  static fromRawValue(rawValue: number): ResourceErrorReason {
    return (
      [
        ResourceErrorReason.none,
        ResourceErrorReason.notFound,
        ResourceErrorReason.server,
        ResourceErrorReason.connection,
        ResourceErrorReason.rateLimit,
        ResourceErrorReason.other,
      ].find((value) => value.rawValue === rawValue) ??
      new ResourceErrorReason(rawValue, `unknown(${rawValue})`)
    );
  }
}

/** What a provider answers with. */
export interface ResourceResponse {
  readonly status?: ResourceResponseStatus;
  readonly errorReason?: ResourceErrorReason;
  /** The bytes MapLibre receives. Copied at the boundary. */
  readonly bytes?: Uint8Array;
  readonly errorMessage?: string;
  readonly mustRevalidate?: boolean;
  readonly modifiedUnixMs?: bigint;
  readonly expiresUnixMs?: bigint;
  readonly etag?: string;
  readonly retryAfterUnixMs?: bigint;
}

/** What MapLibre asked for. */
export interface ResourceRequestInfo {
  /**
   * The request's logical, cache-facing identity, which keeps configured
   * URI-scheme aliases and custom schemes.
   */
  readonly requestedUrl: string;
  /** What a provider fetches, which equals the requested URL when no alias applies. */
  readonly resolvedUrl: string;
  readonly kind: ResourceKind;
  readonly priorEtag: string | undefined;
  readonly priorModifiedUnixMs: bigint | undefined;
  readonly priorExpiresUnixMs: bigint | undefined;
  readonly priorData: Uint8Array | undefined;
  readonly range: { readonly start: bigint; readonly end: bigint } | undefined;
}

/** A claimed request, which the host answers exactly once. */
export class ResourceRequest {
  readonly #native: Native;
  readonly #handle: bigint;
  readonly info: ResourceRequestInfo;
  #settled = false;

  /** @internal */
  constructor(native: Native, handle: bigint, info: ResourceRequestInfo) {
    this.#native = native;
    this.#handle = handle;
    this.info = info;
  }

  /** Reports whether MapLibre has stopped waiting for this request. */
  get isCancelled(): boolean {
    this.#requireOpen("ResourceRequest.isCancelled");
    const native = this.#native;
    return native.scope((scope) => {
      const out = scope.allocateZeroed(1);
      native.checked(scope, EP.mln_resource_request_cancelled, [
        this.#handle,
        out,
      ]);
      return native.memory.bytes(out, 1)[0] !== 0;
    });
  }

  /**
   * Answers the request.
   *
   * Completion is terminal: a request completes once, and a completion that
   * reached the C API consumes it even when the C API reports a failure.
   */
  complete(response: ResourceResponse): void {
    this.#requireOpen("ResourceRequest.complete");
    const native = this.#native;
    native.scope((scope) => {
      const layout = native.layout("mln_resource_response");
      const storage = scope.allocateZeroed(layout.size, layout.align);
      const view = native.memory.view(storage, layout.size);
      const fields = layout.fields;
      view.setUint32(fields.size!.offset, layout.size, true);
      view.setUint32(
        fields.status!.offset,
        (response.status ?? ResourceResponseStatus.ok).rawValue,
        true,
      );
      view.setUint32(
        fields.error_reason!.offset,
        (response.errorReason ?? ResourceErrorReason.none).rawValue,
        true,
      );
      if (response.bytes !== undefined && response.bytes.length > 0) {
        const bytes = scope.allocate(response.bytes.length, 1);
        native.memory.bytes(bytes, response.bytes.length).set(response.bytes);
        native.memory.writePointer(
          (storage + BigInt(fields.bytes!.offset)) as Ptr,
          bytes,
        );
        writeSizeField(
          native,
          storage,
          fields.byte_count!.offset,
          response.bytes.length,
        );
      }
      if (response.errorMessage !== undefined) {
        native.memory.writePointer(
          (storage + BigInt(fields.error_message!.offset)) as Ptr,
          native.cString(scope, response.errorMessage, "errorMessage"),
        );
      }
      if (response.etag !== undefined) {
        native.memory.writePointer(
          (storage + BigInt(fields.etag!.offset)) as Ptr,
          native.cString(scope, response.etag, "etag"),
        );
      }
      view.setUint8(
        fields.must_revalidate!.offset,
        response.mustRevalidate === true ? 1 : 0,
      );
      writeOptionalTime(
        view,
        fields.has_modified!.offset,
        fields.modified_unix_ms!.offset,
        response.modifiedUnixMs,
        "modifiedUnixMs",
      );
      writeOptionalTime(
        view,
        fields.has_expires!.offset,
        fields.expires_unix_ms!.offset,
        response.expiresUnixMs,
        "expiresUnixMs",
      );
      writeOptionalTime(
        view,
        fields.has_retry_after!.offset,
        fields.retry_after_unix_ms!.offset,
        response.retryAfterUnixMs,
        "retryAfterUnixMs",
      );
      // Settled once the completion reaches C, and not before: a response
      // this binding refuses never reached native code, so the request is
      // still outstanding and the host must be able to answer it again. A
      // native failure still counts, because C may have taken the response.
      this.#settled = true;
      native.checked(scope, EP.mln_resource_request_complete, [
        this.#handle,
        storage,
      ]);
    });
    this.#release();
  }

  /**
   * Gives the request up without answering it.
   *
   * MapLibre stops waiting on this provider for it. A request the host neither
   * completes nor closes stays outstanding for the runtime's life.
   */
  close(): void {
    if (this.#settled) {
      return;
    }
    this.#settled = true;
    this.#release();
  }

  get isSettled(): boolean {
    return this.#settled;
  }

  #release(): void {
    const native = this.#native;
    native.scope((scope) => {
      native.raw(scope, EP.mln_resource_request_release, [this.#handle]);
    });
  }

  #requireOpen(operation: string): void {
    if (this.#settled) {
      throw new MaplibreError(
        "closedHandle",
        `this resource request was already answered, so ${operation} cannot run`,
        { operation },
      );
    }
  }
}

function writeOptionalTime(
  view: DataView,
  presentOffset: number,
  valueOffset: number,
  value: bigint | undefined,
  what: string,
): void {
  if (value === undefined) {
    view.setUint8(presentOffset, 0);
    return;
  }
  view.setUint8(presentOffset, 1);
  view.setBigInt64(valueOffset, asInt64(value, what), true);
}

function writeSizeField(
  native: Native,
  storage: Ptr,
  offset: number,
  value: number,
): void {
  const address = (storage + BigInt(offset)) as Ptr;
  const view = native.memory.view(address, native.transport.pointerSize);
  if (native.transport.pointerSize === 8) {
    view.setBigUint64(0, BigInt(value), true);
    return;
  }
  view.setUint32(0, value, true);
}
