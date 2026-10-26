// The part of node:process that the suite reads, for the browser bundle.

function unavailable(): never {
  throw new Error("a page cannot end its own process");
}

const process = {
  env: {} as Record<string, string | undefined>,
  platform: "browser",
  on: unavailable,
  exit: unavailable,
};

export default process;
