import maplibre, { NativeBuffer } from "@maplibre/native-ffi-node";
import { MaplibreError } from "@maplibre/native-ffi-node/error";
import { projectedMetersForLatLng } from "@maplibre/native-ffi-node/geo";
import { setLogCallback } from "@maplibre/native-ffi-node/log";
import { MapHandle } from "@maplibre/native-ffi-node/map";
import { OfflineOperationHandle } from "@maplibre/native-ffi-node/offline";
import { NativePointer } from "@maplibre/native-ffi-node/render";
import { ResourceRequestHandle } from "@maplibre/native-ffi-node/resource";
import { RuntimeHandle } from "@maplibre/native-ffi-node/runtime";

if (
  !maplibre.RuntimeHandle ||
  !NativeBuffer ||
  !MaplibreError ||
  !projectedMetersForLatLng ||
  !setLogCallback ||
  !MapHandle ||
  !OfflineOperationHandle ||
  !NativePointer ||
  !ResourceRequestHandle ||
  !RuntimeHandle
) {
  throw new Error("ESM named exports did not resolve");
}
