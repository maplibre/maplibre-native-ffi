/**
 * Copying one queried feature out of the struct the C API filled.
 *
 * The struct borrows its strings and its feature tree from the result handle,
 * which is released as soon as the query returns, so everything is copied here.
 */

import type { Feature } from "../geojson.ts";
import type { JsonValue } from "../json.ts";
import type { QueriedFeature } from "../query.ts";
import { MLN_QUERIED_FEATURE_FIELD } from "../raw/enums.ts";
import type { Native } from "./native.ts";
import type { Ptr } from "./transport.ts";
import { readFeature, readJsonValue, readStringView } from "./value-decode.ts";

export function readQueriedFeature(
  native: Native,
  storage: Ptr,
): QueriedFeature {
  const layout = native.layout("mln_queried_feature");
  const view = native.memory.view(storage, layout.size);
  const fields = layout.fields;
  const mask = view.getUint32(fields.fields!.offset, true);
  const present = (flag: number): boolean => (mask & flag) !== 0;

  const feature: Feature = readFeature(
    native,
    (storage + BigInt(fields.feature!.offset)) as Ptr,
  );
  const state: JsonValue | undefined = present(
    MLN_QUERIED_FEATURE_FIELD.MLN_QUERIED_FEATURE_STATE,
  )
    ? readJsonValue(native, (storage + BigInt(fields.state!.offset)) as Ptr)
    : undefined;

  return {
    feature,
    sourceId: present(MLN_QUERIED_FEATURE_FIELD.MLN_QUERIED_FEATURE_SOURCE_ID)
      ? readStringView(
          native,
          (storage + BigInt(fields.source_id!.offset)) as Ptr,
        )
      : undefined,
    sourceLayerId: present(
      MLN_QUERIED_FEATURE_FIELD.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID,
    )
      ? readStringView(
          native,
          (storage + BigInt(fields.source_layer_id!.offset)) as Ptr,
        )
      : undefined,
    state,
  };
}
