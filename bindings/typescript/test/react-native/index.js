import React from "react";
import { AppRegistry, Text, View } from "react-native";

import probe from "./specs/NativeLifecycleProbe";
import "./probe/native/shim_test.js";

const generation = global.__mlnLifecycleGeneration;
if (!Number.isInteger(generation)) {
  throw new Error("The lifecycle probe's JSI bindings were not installed");
}
function App() {
  const [status, setStatus] = React.useState("starting");
  // Reload through the native host after mounting, including in release
  // builds, without Metro or the development menu.
  React.useEffect(() => {
    async function run() {
      global.__mlnReleaseEarly();
      if (generation === 1) {
        // Load the installed package in a realm of its own. The later realms
        // bundle the source entry together with the suite's internal probes.
        const { RuntimeHandle } = require("@maplibre/native-ffi");
        const runtime = await RuntimeHandle.create();
        await runtime.setEventHandler(() => {});
        const { map, ready } = runtime.createMap({
          initialExtent: { width: 32, height: 32, scaleFactor: 1 },
        });
        await ready;
        await map.close();
        await runtime.close();
        console.log("MlnPackage passed");
      } else {
        const { leaveLiveWork, runSuite } = require("./suite/runner.js");
        await runSuite();
        if (generation <= 5) await leaveLiveWork();
      }
      setStatus("passed");
      console.log(`MlnBinding ${generation} passed`);
      if (generation <= 5) {
        probe.reload();
      }
    }
    run().catch((error) => {
      setStatus(`failed: ${String(error)}`);
      console.error(error);
    });
  }, []);
  return React.createElement(
    View,
    { style: { padding: 40, flex: 1, backgroundColor: "white" } },
    React.createElement(
      Text,
      { style: { color: "black" } },
      `Lifecycle probe: runtime ${generation}/6 ${status}`,
    ),
  );
}

AppRegistry.registerComponent("HelloWorld", () => App);
