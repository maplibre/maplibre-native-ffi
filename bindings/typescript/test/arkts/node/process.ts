// The part of node:process that the suite reads, for the ArkTS bundle. The
// runner fills `env` from the parameters of `aa test` before the suite runs.

function unavailable(): never {
  throw new Error("an ArkTS application cannot end its own process");
}

const process = {
  env: {} as Record<string, string | undefined>,
  platform: "openharmony",
  on: unavailable,
  exit: unavailable,
};

export default process;
