(() => {
  const test = globalThis.__mlnShimTest;
  function check(value) {
    if (!value) throw new Error("Node-API-on-JSI shim assertion failed");
  }
  function throws(callback, expected) {
    try {
      callback();
    } catch (error) {
      check(error === expected || error instanceof expected);
      return;
    }
    throw new Error("Expected an exception");
  }
  for (const value of [0n, 1n, -1n, -(2n ** 63n), 2n ** 63n - 1n]) {
    check(test.signed(value) === value);
  }
  for (const value of [0n, 1n, 2n ** 63n, 2n ** 64n - 1n]) {
    check(test.unsigned(value) === value);
  }
  throws(() => test.signed(2n ** 63n), RangeError);
  throws(() => test.unsigned(-1n), RangeError);
  const sentinel = {};
  throws(
    () =>
      test.pending({
        get throwing() {
          throw sentinel;
        },
      }),
    sentinel,
  );
  throws(
    () =>
      test.call(() => {
        throw sentinel;
      }),
    sentinel,
  );
  check(test.call(() => test.unsigned(2n ** 64n - 1n)) === 2n ** 64n - 1n);
  const bytes = new Uint8Array([10, 20, 30, 40]).subarray(1, 3);
  for (const name of ["buffer", "byteOffset", "byteLength", "length"]) {
    Object.defineProperty(bytes, name, {
      get() {
        throw sentinel;
      },
    });
  }
  Object.defineProperty(bytes, Symbol.toStringTag, { value: "Float64Array" });
  check(JSON.stringify(test.bytes(bytes)) === "[1,2,1,20]");
  check(JSON.stringify(test.bytes(new Uint8Array(0))) === "[1,0,0,-1]");
  throws(
    () =>
      test.bytes({
        buffer: new ArrayBuffer(100),
        length: 100,
        byteOffset: 0,
        byteLength: 100,
        [Symbol.toStringTag]: "Uint8Array",
      }),
    TypeError,
  );
  throws(() => test.bytes(new Proxy(new Uint8Array(1), {})), TypeError);
  const created = test.createBytes();
  check(created instanceof Uint8Array && created.byteOffset === 2);
  check(created.length === 3 && created[0] === 2 && created[2] === 4);
  check(test.external(test.token));
  throws(() => test.external({}), TypeError);
  throws(() => test.external(globalThis.__mlnForeignState), TypeError);
  globalThis.__mlnShimPassed = true;
})();
