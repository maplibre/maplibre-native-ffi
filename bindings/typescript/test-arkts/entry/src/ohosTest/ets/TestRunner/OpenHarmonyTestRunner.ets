/**
 * The runner `aa test` starts.
 *
 * It hands hypium the suite and the arguments the shell passed, then starts the
 * test ability. This is the shape the OpenHarmony toolchain expects; the cases
 * themselves are the shared tree.
 */

import AbilityDelegatorRegistry from '@ohos.app.ability.abilityDelegatorRegistry';
import { Hypium } from '@ohos/hypium';
import testsuite from '../test/List.test';

export default class OpenHarmonyTestRunner {
  onPrepare() {}

  onRun() {
    const delegator = AbilityDelegatorRegistry.getAbilityDelegator();
    const parameters = AbilityDelegatorRegistry.getArguments();
    Hypium.hypiumTest(delegator, parameters, testsuite);
  }
}
