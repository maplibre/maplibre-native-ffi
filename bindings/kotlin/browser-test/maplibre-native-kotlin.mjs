// The Kotlin distribution the module boots, for the browser test suite.
//
// The module imports `./maplibre-native-kotlin.mjs` on the pthread
// -sPROXY_TO_PTHREAD gives main() and calls `mlnKotlinMain()`. An application
// serves its own distribution under that name and exports the entry point from
// Kotlin. A test binary has no entry point to export: the compiler emits the
// suite as `startUnitTests`, which only JavaScript can call. So this stands in
// for the application, and everything below it is the same Kotlin the module
// would boot in production.
import * as suite from "./maplibre-native-kotlin-suite.mjs";

export function mlnKotlinMain() {
  let status;
  try {
    suite.mlnKotlinTestBegin();
    suite.startUnitTests();
    status = suite.mlnKotlinTestFailures() === 0 ? 0 : 1;
  } catch (error) {
    console.error("maplibre: the browser suite did not finish", error);
    status = 70;
  }

  // The status leaves this thread through the process exit, because nothing
  // else crosses back to the host: the suite runs in a realm of its own, and
  // main() is holding a runtime keepalive that would otherwise keep the module
  // alive with nothing left to run. mln_kotlin_exit() clears that keepalive on
  // the host's thread and exits there, which is where Module.onExit is raised.
  try {
    globalThis.Module._mln_kotlin_exit(status);
  } catch {
    // Emscripten unwinds the calling thread by throwing out of exit().
  }
}
