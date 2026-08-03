/**
 * Copying a queued resource request out of the record the adapter owns.
 *
 * The record's strings and bytes die with the record, which is destroyed as soon
 * as the handler returns, so everything the handler can keep is copied here. The
 * request handle is not: it is an ordinary id the host completes later, from
 * this context or another one.
 */

import {
  ResourceRequest,
  type ResourceRequestInfo,
} from "../resource-request.ts";
import { ResourceKind } from "../resources.ts";
import type { Native } from "./native.ts";
import type { Ptr } from "./transport.ts";

export function readQueuedRequest(
  native: Native,
  record: Ptr,
): ResourceRequest {
  const layout = native.layout("mln_adapter_queued_resource_request");
  const bytes = native.transport.readForeign(record, layout.size);
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const fields = layout.fields;
  const pointerSize = native.transport.pointerSize;

  const readPointer = (offset: number): Ptr =>
    (pointerSize === 8
      ? view.getBigUint64(offset, true)
      : BigInt(view.getUint32(offset, true))) as Ptr;
  const readSize = (offset: number): number =>
    pointerSize === 8
      ? Number(view.getBigUint64(offset, true))
      : view.getUint32(offset, true);
  const readString = (offset: number): string =>
    native.transport.readForeignCString(readPointer(offset)) ?? "";
  const optional = <T>(presentOffset: number, read: () => T): T | undefined =>
    view.getUint8(presentOffset) === 0 ? undefined : read();

  const priorDataPointer = readPointer(fields.prior_data!.offset);
  const priorDataSize = readSize(fields.prior_data_size!.offset);
  const priorEtagPointer = readPointer(fields.prior_etag!.offset);

  const info: ResourceRequestInfo = {
    requestedUrl: readString(fields.requested_url!.offset),
    resolvedUrl: readString(fields.resolved_url!.offset),
    kind: ResourceKind.fromRawValue(view.getUint32(fields.kind!.offset, true)),
    // Unlike the URLs, this one is null when the request carried none.
    priorEtag:
      priorEtagPointer === 0n
        ? undefined
        : (native.transport.readForeignCString(priorEtagPointer) ?? undefined),
    priorModifiedUnixMs: optional(fields.has_prior_modified!.offset, () =>
      view.getBigInt64(fields.prior_modified_unix_ms!.offset, true),
    ),
    priorExpiresUnixMs: optional(fields.has_prior_expires!.offset, () =>
      view.getBigInt64(fields.prior_expires_unix_ms!.offset, true),
    ),
    priorData:
      priorDataPointer === 0n || priorDataSize === 0
        ? undefined
        : native.transport.readForeign(priorDataPointer, priorDataSize),
    range: optional(fields.has_range!.offset, () => ({
      start: view.getBigUint64(fields.range_start!.offset, true),
      end: view.getBigUint64(fields.range_end!.offset, true),
    })),
  };

  return new ResourceRequest(
    native,
    view.getBigUint64(fields.handle!.offset, true),
    info,
  );
}
