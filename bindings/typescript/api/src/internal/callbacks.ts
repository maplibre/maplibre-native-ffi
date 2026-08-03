/**
 * Delivering native callbacks to JavaScript.
 *
 * MapLibre calls back on its own worker, network, and logging threads and wants
 * an answer immediately. JavaScript can neither run there nor answer in time, so
 * the C callback adapter answers on the binding's behalf: it copies the borrowed
 * payload into a record it owns, decides from native-owned rules when a decision
 * is needed at once, and hands the record to a listener that returns void.
 *
 * This module is the host end of that. The listener queues the record in the
 * support layer, the transport wakes this execution context, and the drain here
 * decodes each record, runs user code inside a boundary that contains its
 * failures, and destroys the record exactly once.
 */

import { MaplibreError } from "../errors.ts";
import { EP } from "../raw/entrypoints.ts";
import type { Native } from "./native.ts";
import type { Ptr } from "./transport.ts";

/** Which callback family a queued record came from. */
export const RecordKind = {
  log: 1,
  resourceRequest: 2,
} as const;

/** The support layer's record struct, whose layout its own assertions pin. */
const RECORD_BYTES = 24;
const RECORD_KIND_OFFSET = 0;
const RECORD_REGISTRATION_OFFSET = 8;
const RECORD_POINTER_OFFSET = 16;
const DRAIN_BATCH = 64;

/** What a registration does with the records addressed to it. */
export interface Registration {
  readonly kind: number;
  /** Handles one record. The record is destroyed by the registry afterwards. */
  deliver(record: Ptr): void;
  /**
   * Runs when the adapter reports that native code can no longer reach this
   * registration, which is the moment its native state may be released.
   */
  retire(): void;
}

/**
 * Owns every live callback registration for one host execution context.
 */
export class CallbackRegistry {
  readonly #native: Native;
  readonly #registrations = new Map<bigint, Registration>();
  readonly #records: Ptr;
  #nextId = 1n;
  #draining = false;

  constructor(native: Native) {
    this.#native = native;
    this.#records = native.memory.allocate(RECORD_BYTES * DRAIN_BATCH);
  }

  /** Reserves an identity a registration's `listener_data` carries. */
  register(registration: Registration): bigint {
    const id = this.#nextId;
    this.#nextId += 1n;
    this.#registrations.set(id, registration);
    return id;
  }

  /** Reports whether an identity still names a live registration. */
  has(id: bigint): boolean {
    return this.#registrations.has(id);
  }

  /**
   * Delivers every queued record.
   *
   * Draining is not re-entrant: user code that calls back into the binding must
   * not start a second drain and see records out of order.
   */
  drain(): void {
    if (this.#draining) {
      return;
    }
    this.#draining = true;
    try {
      for (;;) {
        const count = this.#native.transport.drainRecords(
          this.#records,
          DRAIN_BATCH,
        );
        for (let index = 0; index < count; index += 1) {
          this.#deliver(index);
        }
        if (count < DRAIN_BATCH) {
          return;
        }
      }
    } finally {
      this.#draining = false;
    }
  }

  #deliver(index: number): void {
    const base = (this.#records + BigInt(index * RECORD_BYTES)) as Ptr;
    const view = this.#native.memory.view(base, RECORD_BYTES);
    const kind = view.getUint32(RECORD_KIND_OFFSET, true);
    const id = view.getBigUint64(RECORD_REGISTRATION_OFFSET, true);
    const record = view.getBigUint64(RECORD_POINTER_OFFSET, true) as Ptr;
    const registration = this.#registrations.get(id);

    if (record === 0n) {
      // The retirement sentinel. Native can no longer reach this registration,
      // so its state goes now rather than when the host next touches it.
      this.#registrations.delete(id);
      registration?.retire();
      return;
    }

    if (registration === undefined) {
      // A record queued before its registration retired. It is still an owned
      // record, so it is released rather than delivered.
      this.#destroy(kind, record);
      return;
    }

    try {
      registration.deliver(record);
    } catch {
      // A host failure must not escape into native code or stop the drain. The
      // record is still destroyed below.
    } finally {
      this.#destroy(kind, record);
    }
  }

  #destroy(kind: number, record: Ptr): void {
    const entrypoint =
      kind === RecordKind.log
        ? EP.mln_adapter_log_record_destroy
        : EP.mln_adapter_resource_provider_request_destroy;
    this.#native.scope((scope) => {
      this.#native.raw(scope, entrypoint, [record]);
    });
  }

  /** Releases the drain storage once no registration remains. */
  close(): void {
    if (this.#registrations.size > 0) {
      throw new MaplibreError(
        "invalidState",
        `${this.#registrations.size} callback registrations are still live`,
      );
    }
    this.#native.memory.free(this.#records);
  }
}

/**
 * The log registration.
 *
 * The registration's native state has to outlive the call that installs it and
 * stay reachable until the adapter reports it retired, so it lives in slab
 * storage this object owns rather than in call-scoped storage.
 */
export class LogRegistration implements Registration {
  readonly kind = RecordKind.log;
  readonly #native: Native;
  readonly #registry: CallbackRegistry;
  readonly #state: Ptr;
  readonly #handler: (record: Ptr) => void;

  constructor(
    native: Native,
    registry: CallbackRegistry,
    handler: (record: Ptr) => void,
    consume: boolean,
  ) {
    this.#native = native;
    this.#registry = registry;
    this.#handler = handler;
    const layout = native.layout("mln_adapter_log_callback_state");
    this.#state = native.memory.allocate(layout.size, layout.align);
    native.memory.bytes(this.#state, layout.size).fill(0);
    const id = registry.register(this);
    const view = native.memory.view(this.#state, layout.size);
    view.setUint32(layout.fields.consume!.offset, consume ? 1 : 0, true);
    native.memory.writePointer(
      (this.#state + BigInt(layout.fields.listener!.offset)) as Ptr,
      native.transport.listenerAddress(RecordKind.log),
    );
    // The identity is a plain number the C API only passes through, so it needs
    // no storage of its own.
    native.memory.writePointer(
      (this.#state + BigInt(layout.fields.listener_data!.offset)) as Ptr,
      id as Ptr,
    );
    this.id = id;
  }

  readonly id: bigint;

  /** The address the C API stores as this registration. */
  get statePointer(): Ptr {
    return this.#state;
  }

  deliver(record: Ptr): void {
    this.#handler(record);
  }

  retire(): void {
    this.#native.memory.free(this.#state);
  }

  /** Drops the registration without waiting for a retirement record. */
  abandon(): void {
    if (this.#registry.has(this.id)) {
      this.retire();
    }
  }
}

/**
 * A native-owned rule table.
 *
 * The C API borrows a rule table for as long as the registration lives, so the
 * table, the rule array, and every string in it are allocated here and released
 * only when the registration is replaced or cleared.
 */
export class RuleTable {
  readonly #native: Native;
  readonly #allocations: Ptr[] = [];
  readonly table: Ptr;

  private constructor(native: Native, table: Ptr, allocations: readonly Ptr[]) {
    this.#native = native;
    this.table = table;
    this.#allocations.push(...allocations);
  }

  /** Builds the rewrite-rule table a resource transform reads. */
  static rewriteRules(
    native: Native,
    rules: readonly { kind: number; url: string; replacementUrl?: string }[],
  ): RuleTable {
    const allocations: Ptr[] = [];
    const allocate = (size: number, alignment?: number): Ptr => {
      const address = native.memory.allocate(size, alignment);
      allocations.push(address);
      native.memory.bytes(address, size).fill(0);
      return address;
    };
    const persist = (value: string, what: string): Ptr => {
      const bytes = new TextEncoder().encode(value);
      if (bytes.includes(0)) {
        throw new MaplibreError(
          "invalidInput",
          `${what} contains an embedded NUL, which a null-terminated C string cannot carry`,
        );
      }
      const address = allocate(bytes.length + 1, 1);
      const target = native.memory.bytes(address, bytes.length + 1);
      target.set(bytes);
      return address;
    };

    try {
      const ruleLayout = native.layout("mln_adapter_resource_rewrite_rule");
      const tableLayout = native.layout("mln_adapter_resource_rewrite_rules");
      const array = allocate(
        Math.max(ruleLayout.size * rules.length, 1),
        ruleLayout.align,
      );
      rules.forEach((rule, index) => {
        const base = (array + BigInt(index * ruleLayout.size)) as Ptr;
        const view = native.memory.view(base, ruleLayout.size);
        view.setUint32(ruleLayout.fields.kind!.offset, rule.kind, true);
        native.memory.writePointer(
          (base + BigInt(ruleLayout.fields.url!.offset)) as Ptr,
          persist(rule.url, "rule url"),
        );
        if (rule.replacementUrl !== undefined) {
          native.memory.writePointer(
            (base + BigInt(ruleLayout.fields.replacement_url!.offset)) as Ptr,
            persist(rule.replacementUrl, "rule replacementUrl"),
          );
        }
      });

      const table = allocate(tableLayout.size, tableLayout.align);
      native.memory.writePointer(
        (table + BigInt(tableLayout.fields.rules!.offset)) as Ptr,
        array,
      );
      writeSize(
        native,
        (table + BigInt(tableLayout.fields.count!.offset)) as Ptr,
        rules.length,
      );
      return new RuleTable(native, table, allocations);
    } catch (error) {
      for (const address of allocations) {
        native.memory.free(address);
      }
      throw error;
    }
  }

  /** Releases the table once native code can no longer read it. */
  release(): void {
    for (const address of this.#allocations) {
      this.#native.memory.free(address);
    }
    this.#allocations.length = 0;
  }
}

/** Writes a `size_t` field at the transport's pointer width. */
export function writeSize(native: Native, address: Ptr, value: number): void {
  const view = native.memory.view(address, native.transport.pointerSize);
  if (native.transport.pointerSize === 8) {
    view.setBigUint64(0, BigInt(value), true);
    return;
  }
  view.setUint32(0, value, true);
}
