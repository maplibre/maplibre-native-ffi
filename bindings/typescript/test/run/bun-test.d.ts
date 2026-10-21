// The part of Bun's test runner that test/run/bun.test.ts uses.

declare module "bun:test" {
  export function test(name: string, run: () => Promise<void>): void;
}
