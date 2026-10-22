// The part of node:process that the suite reads, for the Hermes bundle. The
// host gives the script its environment.

function unavailable(): never {
  throw new Error("a script on the Hermes host cannot end its own process");
}

const process = {
  env: { ...hermesHost.env } as Record<string, string | undefined>,
  platform: "hermes",
  on: unavailable,
  exit: unavailable,
};

export default process;
