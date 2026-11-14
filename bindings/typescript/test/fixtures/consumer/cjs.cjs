// A CommonJS script that loads the installed facade through require().

const { cVersion, RuntimeHandle } = require("@maplibre/native-ffi");

void RuntimeHandle.create()
  .release()
  .then(() => {
    console.log(`loaded C API version ${cVersion()}`);
  });
