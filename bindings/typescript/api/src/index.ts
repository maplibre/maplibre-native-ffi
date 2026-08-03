/**
 * Low-level TypeScript binding for the MapLibre Native C API.
 *
 * This package owns every public name. A runtime payload package supplies one
 * compiled artifact and the metadata describing it, and defines no MapLibre API
 * of its own.
 */

export {
  AbiMismatchError,
  type NodeApiAddon,
} from "./internal/node-transport.ts";
