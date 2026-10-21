// The hypium entry: the runner that `aa test` starts on the device. It
// registers every case that the harness schedules for ArkTS and prints how
// many it registered, which test-arkts.sh compares with the number that
// hypium ran.

import "./globals.ts";
import "../callbacks.test.ts";
import "../completions.test.ts";
import "../errors.test.ts";
import "../event-loop.test.ts";
import "../lifecycle.test.ts";
import "../log.test.ts";
import "../platform.test.ts";
import "../realms.test.ts";
import "../render.test.ts";
import "../resource-provider.test.ts";
import "../values.test.ts";
import AbilityDelegatorRegistry from "@ohos.app.ability.abilityDelegatorRegistry";
import { describe, Hypium, it, TestType } from "@ohos/hypium";

import { execute, scheduled } from "../support/harness.ts";
import process from "./node/process.ts";

export default class OpenHarmonyTestRunner {
  onPrepare(): void {}

  onRun(): void {
    const delegator = AbilityDelegatorRegistry.getAbilityDelegator();
    const parameters = AbilityDelegatorRegistry.getArguments();
    const parameter = (name: string) =>
      parameters.parameters[`-s ${name}`] ?? parameters.parameters[name];
    process.env.MAPLIBRE_NATIVE_FFI_ORIGIN = parameter("origin");
    const order = scheduled();
    delegator.printSync(`harness registered ${order.length} cases`);
    Hypium.hypiumTest(delegator, parameters, () => {
      describe("typescript", () => {
        for (const entry of order) {
          it(entry.name, TestType.FUNCTION, () => execute(entry));
        }
      });
    });
  }
}
