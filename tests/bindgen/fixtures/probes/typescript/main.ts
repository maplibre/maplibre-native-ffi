// Runs the TypeScript binding that the probe generated, with its handwritten
// runtime, against protocols_stub.c through the generated Node-API addon.

import assert from "node:assert/strict";

import {
  MaplibreError,
  keywordCombine,
  probeBatchPoints,
  probeBatchScores,
  probeHooksDefault,
  probeZerosJson,
  probeReadLevel,
  probeRoundtrip,
  probeSettingsCheck,
  probeSettingsDefault,
  probeTakeParcel,
} from "./index.ts";

function failure(body: () => unknown): MaplibreError {
  try {
    body();
  } catch (error) {
    assert.ok(error instanceof MaplibreError, String(error));
    return error;
  }
  assert.fail("the call did not fail");
}

// A record round-trips through C: a presence bit, a nullable array, and a
// required array.
const point = { type: 9.5, gain: 3.25 };
const input = { title: "", point, left: [point], right: [point, point] };
assert.deepEqual(probeRoundtrip(input), input);
assert.deepEqual(probeRoundtrip({ left: [], right: [] }), {
  left: [],
  right: [],
});
assert.deepEqual(probeRoundtrip({ right: [] }), { right: [] });

// Caller-sized text that fits the addon's first storage, and text that does
// not, which the addon fetches again with enough storage.
assert.equal(probeZerosJson(0), "[]");
assert.equal(probeZerosJson(3), "[0,0,0]");
assert.equal(probeZerosJson(300), `[${Array(300).fill(0).join(",")}]`);

// A caller-sized output array holds one element per input element, in input
// order.
const batch = [
  { x: 2, y: 5 },
  { x: 7, y: 11 },
  { x: -3, y: 17 },
];
assert.deepEqual(probeBatchPoints(batch), [
  { x: 5, y: 3 },
  { x: 11, y: 8 },
  { x: 17, y: -2 },
]);
assert.deepEqual(probeBatchScores(batch), [7, 18, 14]);
assert.deepEqual(probeBatchPoints([]), []);
assert.deepEqual(probeBatchScores([]), []);

// Parameters that are TypeScript keywords keep their order.
assert.deepEqual(keywordCombine(5, 2, 7, 11), { type: 3, defer: 7, raw: 11 });

// An empty record keeps the native default, and a copy of the default holds
// every value the default sets.
probeSettingsCheck({});
probeSettingsCheck(probeSettingsDefault());

// A copy of a native default keeps its values and leaves a registration unset.
assert.deepEqual(probeHooksDefault(), { limit: 4 });

// A status that the C API declares as absence returns undefined, and any other
// failure still reports its own diagnostic.
assert.equal(probeReadLevel(), undefined);
assert.equal(probeReadLevel(), 0.5);
const exhausted = failure(() => probeReadLevel());
assert.equal(exhausted.status, -1);
assert.equal(exhausted.diagnostic, "exhausted");

assert.equal(probeTakeParcel(), undefined);
const parcel = probeTakeParcel();
assert.equal(parcel?.id, 42n);
parcel?.close();
failure(() => probeTakeParcel());

const rejected = failure(() =>
  probeRoundtrip({ right: Array<typeof point>(9).fill(point) }),
);
assert.equal(rejected.diagnostic, "right holds more than 8 points");
