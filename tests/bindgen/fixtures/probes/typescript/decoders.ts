// Drives the generated decoders with records that the real core never
// produces: elements wider than this binding's, an unknown union tag, and
// strides narrower than an element.

import assert from "node:assert/strict";

import { MapHandle, ProbeLedgerHandle } from "./index.ts";
import { ADOPT } from "./internal/owners.ts";

const map = new MapHandle(ADOPT, 1n);

// An array completion is read at the result's value_size, and an absent one
// reads as undefined.
assert.deepEqual(await map.visibleTiles(), [
  { x: 1, y: 2 },
  { x: 3, y: 4 },
]);
assert.equal(await map.visibleTiles(), undefined);
// A stride narrower than the element cannot hold one.
await assert.rejects(map.visibleTiles(), {
  name: "RangeError",
  message: "native completion stride is too small",
});

// A single value is read the same way.
assert.deepEqual(await map.findTile(), { x: 3, y: 4 });
await assert.rejects(map.findTile(), {
  name: "RangeError",
  message: "native completion stride is too small",
});

// A record of only a tag and a union is the union itself. It keeps a known
// arm's value. An unknown arm keeps its tag and the bytes of the whole record,
// as far as the reported stride reaches, which here includes a member that
// this header does not declare.
assert.deepEqual(await map.event(), {
  kind: "frame",
  value: { timestamp: 2.5 },
});
const unknown = await map.event();
assert.ok(unknown.kind === "unknown");
assert.equal(unknown.tag, 7);
assert.equal(unknown.bytes.length, 24);
assert.equal(
  new DataView(unknown.bytes.buffer, unknown.bytes.byteOffset).getUint32(
    0,
    true,
  ),
  7,
);
assert.deepEqual([...unknown.bytes.subarray(8, 16)], Array(8).fill(0xab));
assert.deepEqual([...unknown.bytes.subarray(16)], Array(8).fill(0xcd));

map.close();

// Records that native code steps through by a wider stride than the binding
// compiled keep their values. The drain releases its batch, so the next drain
// reaches native, which has nothing queued.
const ledger = ProbeLedgerHandle.open();
assert.deepEqual(ledger.drain(), [{ value: 7n }, { value: 9n }]);
assert.deepEqual(ledger.drain(), []);
ledger.close();
