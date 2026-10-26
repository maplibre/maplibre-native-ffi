// A CommonJS script that loads the installed facade through require().

const { cVersion, RuntimeHandle } = require("@maplibre/native-ffi");

const runtime = RuntimeHandle.create();
void runtime.close().then(() => {
  console.log(`loaded C API version ${cVersion()}`);
});
