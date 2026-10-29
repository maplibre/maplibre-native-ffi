import React from "react";
import { AppRegistry, Text } from "react-native";

import probe from "./specs/NativeLifecycleProbe";

async function run() {
  global.__mlnReleaseEarly();
  const generation = global.__mlnLifecycleGeneration;
  if (generation === 1) {
    const { runSuite } = require("./suite/runner.js");
    await runSuite(() => {
      console.log("MlnBinding 1 passed");
      probe.reload();
    });
  } else if (generation === 2) {
    // A fresh runtime loads only the installed package, with no source bundle.
    const { RuntimeHandle } = require("@maplibre/native-ffi");
    const runtime = await RuntimeHandle.create();
    const { map, ready } = runtime.createMap({
      initialExtent: { width: 32, height: 32, scaleFactor: 1 },
    });
    await ready;
    await map.close();
    await runtime.close();
    console.log("MlnBinding 2 passed");
  } else {
    throw new Error(`Unexpected runtime generation: ${generation}`);
  }
}

function App() {
  const [status, setStatus] = React.useState("running");
  React.useEffect(() => {
    run().then(
      () => setStatus("passed"),
      (error) => {
        setStatus(`failed: ${String(error)}`);
        console.error(`FAIL ${String(error)}\n${error.stack ?? ""}`);
      },
    );
  }, []);
  return React.createElement(
    Text,
    null,
    `React Native binding suite: ${status}`,
  );
}

AppRegistry.registerComponent("HelloWorld", () => App);
