// The parts of the OpenHarmony system modules, hypium, and the packed
// native libraries that the ArkTS entry and the ArkTS harness use. The SDK's own declarations are ArkTS sources that the
// TypeScript checker cannot read.

declare module "@ohos.app.ability.abilityDelegatorRegistry" {
  interface AbilityDelegator {
    printSync(message: string): void;
    finishTest(message: string, code: number, callback: () => void): void;
  }
  const registry: {
    getAbilityDelegator(): AbilityDelegator;
    getArguments(): { bundleName: string; parameters: Record<string, string> };
  };
  export default registry;
}

declare module "@ohos.app.ability.UIAbility" {
  export default class UIAbility {}
}

declare module "@ohos.net.http" {
  interface HttpResponse {
    readonly responseCode: number;
    readonly result: string | object | ArrayBuffer;
  }
  interface HttpRequest {
    request(
      url: string,
      options: {
        method: string;
        header?: Record<string, string>;
        extraData?: string;
        expectDataType?: number;
      },
    ): Promise<HttpResponse>;
    destroy(): void;
  }
  const http: {
    createHttp(): HttpRequest;
    readonly RequestMethod: {
      readonly GET: string;
      readonly POST: string;
      readonly DELETE: string;
    };
    readonly HttpDataType: { readonly STRING: number };
  };
  export default http;
}

declare module "@ohos.util" {
  const util: {
    TextEncoder: new () => { encodeInto(input: string): Uint8Array };
  };
  export default util;
}

declare module "@ohos.worker" {
  interface ThreadWorker {
    onmessage: ((event: { data: unknown }) => void) | null;
    onerror: ((error: { message: string }) => void) | null;
    onexit: ((code: number) => void) | null;
    postMessage(message: unknown): void;
    terminate(): void;
  }
  const worker: {
    ThreadWorker: new (script: string) => ThreadWorker;
    readonly workerPort: {
      onmessage: ((event: { data: unknown }) => void) | null;
      postMessage(message: unknown): void;
    };
  };
  export default worker;
}

declare module "@ohos/hypium" {
  export const TestType: { readonly FUNCTION: number };
  export function describe(name: string, body: () => void): void;
  export function it(
    name: string,
    type: number,
    body: () => Promise<void>,
  ): void;
  export const Hypium: {
    hypiumTest(
      delegator: unknown,
      parameters: unknown,
      suite: () => void,
    ): void;
  };
}

declare module "libmaplibre_native_ffi_test_graphics.so" {
  const addon: unknown;
  export default addon;
}
