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
// Types are imported separately: a merged-bytecode import is checked against
// what the record actually exports, and a type is not one of them.
import { groupsFor, loadArkTs, MaplibreError } from '../conformance.bundle';
import { CONFORMANCE } from '../conformance.bundle';
import type { Expect } from '../conformance.bundle';

export default function conformanceTest() {
  const maplibre = loadArkTs();

  // Distinct per run, so a case starts from a directory nothing has written.
  const runTag = `${Date.now()}`;
  let cacheSequence = 0;
  const cacheDirectory = async (): Promise<string> => {
    cacheSequence += 1;
    // An application's own files directory is the writable place on a device.
    // An application keeps its files across runs, so the name carries the run
    // as well as the case: a second run must not inherit the first's database.
    const path = `/data/storage/el2/base/haps/entry_test/files/cache-${runTag}-${cacheSequence}`;
    fileIo.mkdirSync(path, true);
    return path;
  };

  // Values are compared by their serialization, and this binding's addresses
  // are bigint, which JSON refuses. A bigint is shown with the marker so it
  // never compares equal to the string of the same digits.
  const show = (value: ESObject): string =>
    JSON.stringify(value, (_key: string, member: ESObject): ESObject =>
      typeof member === 'bigint' ? `${member}n` : member) ?? String(value);

  // A case that needs one names the capability and is left out above, so
  // reaching this is a registration mistake rather than something to work
  // around.
  const renderContext = (): ESObject => {
    throw new Error('this runtime has no graphics context to render through');
  };

  const assertions: Expect = {
    equal: (actual, expected, what) => {
      expect(show(actual)).assertEqual(show(expected));
    },
    notEqual: (actual, unexpected, what) => {
      expect(show(actual) !== show(unexpected)).assertTrue();
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

  // What this runner offers: ArkTS resolves no package by name, and a device
  // test has no graphics context to attach a render session to. `groupsFor`
  // decides what to register, so this runner cannot drift from the others.
  const HOST: ESObject = { transport: 'node-api', capabilities: [] };

  for (const group of groupsFor(CONFORMANCE, HOST)) {
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
    import('../conformance.bundle');
}
