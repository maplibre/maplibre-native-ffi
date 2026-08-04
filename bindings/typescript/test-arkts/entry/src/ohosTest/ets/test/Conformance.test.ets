/**
 * The conformance suite, registered in hypium.
 *
 * ArkTS implements Node-API and loads the same addon Node does, so it registers
 * the same tree every other runtime registers. A case that passes on Node and
 * fails here is a real difference between the runtimes.
 */

import { describe, expect, it, TestType } from '@ohos/hypium';
import fileIo from '@ohos.file.fs';

// The suite and the loaded library come from one bundle, because ArkTS resolves
// modules through its own build rather than through node_modules.
import { CONFORMANCE, Expect, Maplibre, MaplibreError } from '../../../../conformance.bundle';

export default function conformanceTest() {
  const maplibre = Maplibre.loadArkTs();

  let cacheSequence = 0;
  const cacheDirectory = async (): Promise<string> => {
    cacheSequence += 1;
    // An application's own files directory is the writable place on a device.
    const path = `/data/storage/el2/base/haps/entry_test/files/cache-${cacheSequence}`;
    fileIo.mkdirSync(path, true);
    return path;
  };

  const assertions: Expect = {
    equal: (actual, expected, what) => {
      expect(JSON.stringify(actual)).assertEqual(JSON.stringify(expected));
    },
    notEqual: (actual, unexpected, what) => {
      expect(JSON.stringify(actual) !== JSON.stringify(unexpected)).assertTrue();
    },
    ok: (actual, what) => {
      expect(actual).assertTrue();
    },
    closeTo: (actual, expected, digits, what) => {
      expect(Math.abs(actual - expected) <= 10 ** -digits / 2).assertTrue();
    },
    defined: (actual, what) => {
      expect(actual !== undefined && actual !== null).assertTrue();
      return actual;
    },
    absent: (actual, what) => {
      expect(actual === undefined).assertTrue();
    },
    contains: (haystack, needle, what) => {
      expect(haystack.includes(needle)).assertTrue();
    },
    throws: (body, what) => {
      let thrown: ESObject = undefined;
      try {
        body();
      } catch (error) {
        thrown = error;
      }
      expect(thrown instanceof MaplibreError).assertTrue();
      return thrown as MaplibreError;
    },
    throwsAny: (body, what) => {
      let thrown: ESObject = undefined;
      try {
        body();
      } catch (error) {
        thrown = error;
      }
      expect(thrown !== undefined).assertTrue();
      return thrown as Error;
    },
    fail: (what) => {
      throw new Error(what);
    },
  };

  for (const group of CONFORMANCE) {
    describe(group.name, () => {
      for (const entry of group.cases) {
        it(entry.name, TestType.FUNCTION, async () => {
          await entry.run({ maplibre, expect: assertions, cacheDirectory, loadPackage });
        });
      }
    });
  }

  // The bundle is the package here, so there is one module format to load.
  const loadPackage = async (): Promise<ESObject> =>
    import('../../../../conformance.bundle');
}
