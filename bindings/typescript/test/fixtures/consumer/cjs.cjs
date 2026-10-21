// A CommonJS script that loads the installed facade through require().

const { cVersion, runtimeCreate } = require("@maplibre/native-ffi");

const runtime = runtimeCreate();
void runtime.close().then(() => {
  console.log(`loaded C API version ${cVersion()}`);
});
