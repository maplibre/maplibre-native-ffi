import maplibre, {
  NativeBuffer,
  type JsonValue,
  type RuntimeOptions,
} from "@maplibre/native-ffi-node";
import { MaplibreError } from "@maplibre/native-ffi-node/error";
import { projectedMetersForLatLng } from "@maplibre/native-ffi-node/geo";
import { setLogCallback } from "@maplibre/native-ffi-node/log";
import { MapHandle } from "@maplibre/native-ffi-node/map";
import { OfflineOperationHandle } from "@maplibre/native-ffi-node/offline";
import { NativePointer } from "@maplibre/native-ffi-node/render";
import { ResourceRequestHandle } from "@maplibre/native-ffi-node/resource";
import { RuntimeHandle } from "@maplibre/native-ffi-node/runtime";

const options: RuntimeOptions = { maximumCacheSize: 1024n };
const json: JsonValue = { enabled: true, maxZoom: 12, tags: ["esm"] };

void maplibre.RuntimeHandle;
void NativeBuffer;
void MaplibreError;
void projectedMetersForLatLng;
void setLogCallback;
void MapHandle;
void OfflineOperationHandle;
void NativePointer;
void ResourceRequestHandle;
void RuntimeHandle;
void options;
void json;
