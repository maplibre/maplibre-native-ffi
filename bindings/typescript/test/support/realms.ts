// Spawned realms that run test/fixtures/realm.ts.

import { bounded, teardown } from "./harness.ts";
import { capabilities, type Realm } from "./host-capabilities.ts";

/** Spawned realms that have not been terminated. */
const open = new Set<Realm>();

/** Starts a realm for one scenario; the caller terminates it. */
export function startRealm(request: Record<string, unknown>): Realm {
  const realm = capabilities.realmSpawn!(request);
  open.add(realm);
  return realm;
}

/** Starts a realm for one scenario, terminated by a teardown. */
export function spawnRealm(request: Record<string, unknown>): Realm {
  const realm = startRealm(request);
  teardown(() => terminateRealm(realm, "the realm's termination"));
  return realm;
}

/** Terminates a realm, once. */
export async function terminateRealm(realm: Realm, what: string) {
  if (!open.delete(realm)) {
    return;
  }
  await bounded(realm.terminate(), what);
}

/** The realm's next message, which fails the case if it reports an error. */
export async function nextMessage<T>(realm: Realm, what: string): Promise<T> {
  const message = (await bounded(realm.next(), what, 60_000)) as {
    error?: string;
  };
  if (message.error !== undefined) {
    throw new Error(`${what}: ${message.error}`);
  }
  return message as T;
}
