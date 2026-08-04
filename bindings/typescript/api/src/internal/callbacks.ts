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
  customGeometryTile: 3,
} as const;

/** The listeners a custom geometry source registers, by their address id. */
export const CustomGeometryListener = {
  fetch: 3,
  cancel: 4,
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

  /** Builds the header-transform table an HTTP header transform reads. */
  static headerTransformRules(
    native: Native,
    rules: readonly {
      kind: number;
      url: string;
      flags: number;
      headers: readonly { name: string; value: string }[];
    }[],
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
      native.memory.bytes(address, bytes.length + 1).set(bytes);
      return address;
    };

    try {
      const headerLayout = native.layout("mln_adapter_http_header");
      const ruleLayout = native.layout(
        "mln_adapter_http_header_transform_rule",
      );
      const tableLayout = native.layout(
        "mln_adapter_http_header_transform_rules",
      );
      const array = allocate(
        Math.max(ruleLayout.size * rules.length, 1),
        ruleLayout.align,
      );
      rules.forEach((rule, index) => {
        const base = (array + BigInt(index * ruleLayout.size)) as Ptr;
        const view = native.memory.view(base, ruleLayout.size);
        view.setUint32(ruleLayout.fields.kind!.offset, rule.kind, true);
        view.setUint32(ruleLayout.fields.flags!.offset, rule.flags, true);
        native.memory.writePointer(
          (base + BigInt(ruleLayout.fields.url!.offset)) as Ptr,
          persist(rule.url, "rule url"),
        );
        // The headers a rule supplies are native-owned for as long as the
        // table is registered, so they are copied here rather than borrowed
        // from the caller's strings.
        const headers = allocate(
          Math.max(headerLayout.size * rule.headers.length, 1),
          headerLayout.align,
        );
        rule.headers.forEach((header, position) => {
          const at = (headers + BigInt(position * headerLayout.size)) as Ptr;
          native.memory.writePointer(
            (at + BigInt(headerLayout.fields.name!.offset)) as Ptr,
            persist(header.name, "header name"),
          );
          native.memory.writePointer(
            (at + BigInt(headerLayout.fields.value!.offset)) as Ptr,
            persist(header.value, "header value"),
          );
        });
        native.memory.writePointer(
          (base + BigInt(ruleLayout.fields.headers!.offset)) as Ptr,
          headers,
        );
        writeSize(
          native,
          (base + BigInt(ruleLayout.fields.header_count!.offset)) as Ptr,
          rule.headers.length,
        );
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

/**
 * A queued resource provider registration.
 *
 * The routes decide in native code, at the moment MapLibre asks, whether a
 * request is this provider's. A claimed request is copied and queued; everything
 * else passes through to the native loader untouched.
 */
export class ProviderRegistration implements Registration {
  readonly kind = RecordKind.resourceRequest;
  readonly #native: Native;
  readonly #allocations: Ptr[] = [];
  readonly #handler: (request: Ptr) => void;
  readonly provider: Ptr;
  readonly id: bigint;

  constructor(
    native: Native,
    registry: CallbackRegistry,
    routes: readonly { kind: number; flags: number; url: string }[],
    handler: (request: Ptr) => void,
  ) {
    this.#native = native;
    this.#handler = handler;
    const allocate = (size: number, alignment?: number): Ptr => {
      const address = native.memory.allocate(size, alignment);
      this.#allocations.push(address);
      native.memory.bytes(address, size).fill(0);
      return address;
    };

    try {
      const routeLayout = native.layout(
        "mln_adapter_queued_resource_provider_route",
      );
      const providerLayout = native.layout(
        "mln_adapter_queued_resource_provider",
      );
      const array = allocate(
        Math.max(routeLayout.size * routes.length, 1),
        routeLayout.align,
      );
      routes.forEach((route, index) => {
        const base = (array + BigInt(index * routeLayout.size)) as Ptr;
        const view = native.memory.view(base, routeLayout.size);
        view.setUint32(routeLayout.fields.kind!.offset, route.kind, true);
        view.setUint32(routeLayout.fields.flags!.offset, route.flags, true);
        const bytes = new TextEncoder().encode(route.url);
        if (bytes.includes(0)) {
          throw new MaplibreError(
            "invalidInput",
            "a route url contains an embedded NUL, which a C string cannot carry",
          );
        }
        const url = allocate(bytes.length + 1, 1);
        native.memory.bytes(url, bytes.length + 1).set(bytes);
        native.memory.writePointer(
          (base + BigInt(routeLayout.fields.url!.offset)) as Ptr,
          url,
        );
      });

      this.provider = allocate(providerLayout.size, providerLayout.align);
      native.memory.writePointer(
        (this.provider + BigInt(providerLayout.fields.routes!.offset)) as Ptr,
        array,
      );
      writeSize(
        native,
        (this.provider +
          BigInt(providerLayout.fields.route_count!.offset)) as Ptr,
        routes.length,
      );
      native.memory.writePointer(
        (this.provider + BigInt(providerLayout.fields.listener!.offset)) as Ptr,
        native.transport.listenerAddress(RecordKind.resourceRequest),
      );
      this.id = registry.register(this);
      native.memory.writePointer(
        (this.provider +
          BigInt(providerLayout.fields.listener_data!.offset)) as Ptr,
        this.id as Ptr,
      );
    } catch (error) {
      for (const address of this.#allocations) {
        native.memory.free(address);
      }
      throw error;
    }
  }

  deliver(request: Ptr): void {
    this.#handler(request);
  }

  retire(): void {
    for (const address of this.#allocations) {
      this.#native.memory.free(address);
    }
    this.#allocations.length = 0;
  }
}

/** One tile a custom geometry source was asked about. */
export interface CustomGeometryTile {
  readonly z: number;
  readonly x: number;
  readonly y: number;
}

/** The zoom the adapter's retirement sentinel uses, which no real tile has. */
const RETIREMENT_ZOOM = 0xff;
const TILE_RECORD_BYTES = 16;

/**
 * A custom geometry source's tile callbacks.
 *
 * MapLibre asks for a tile on one of its own threads and expects nothing back,
 * so the request is copied and queued. The source's state stays reachable until
 * the adapter's retirement sentinel arrives, because native code can be inside
 * a fetch until then.
 */
export class CustomGeometryRegistration implements Registration {
  readonly kind = RecordKind.customGeometryTile;
  readonly #native: Native;
  readonly #onFetch: (tile: CustomGeometryTile) => void;
  readonly #onCancel: ((tile: CustomGeometryTile) => void) | undefined;
  readonly id: bigint;
  #retired = false;

  constructor(
    native: Native,
    registry: CallbackRegistry,
    onFetch: (tile: CustomGeometryTile) => void,
    onCancel?: (tile: CustomGeometryTile) => void,
  ) {
    this.#native = native;
    this.#onFetch = onFetch;
    this.#onCancel = onCancel;
    this.id = registry.register(this);
  }

  deliver(record: Ptr): void {
    // The tile arrived by value and was copied into this record, so its fields
    // come from the record rather than from anything native still owns.
    const bytes = this.#native.transport.readForeign(record, TILE_RECORD_BYTES);
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    const cancelled = view.getUint32(0, true) !== 0;
    const tile: CustomGeometryTile = {
      z: view.getUint32(4, true),
      x: view.getUint32(8, true),
      y: view.getUint32(12, true),
    };
    if (tile.z === RETIREMENT_ZOOM) {
      // The retirement sentinel, not a tile. Native can no longer reach this
      // registration, so the handlers stop here.
      this.#retired = true;
      return;
    }
    if (cancelled) {
      this.#onCancel?.(tile);
      return;
    }
    this.#onFetch(tile);
  }

  retire(): void {
    this.#retired = true;
  }

  get isRetired(): boolean {
    return this.#retired;
  }
}
