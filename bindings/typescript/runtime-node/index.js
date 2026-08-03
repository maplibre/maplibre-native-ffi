import metadata from "./runtime.json" with { type: "json" };
// Loads this payload's compiled addon. The payload defines no MapLibre API of
// its own; the facade owns every public name.
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";

const require = createRequire(import.meta.url);

export const runtime = metadata;
export const addon = require(
  fileURLToPath(new URL(metadata.addon, import.meta.url)),
);
