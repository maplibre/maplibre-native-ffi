/**
 * Native-visible storage, allocated and encoded from TypeScript.
 *
 * The C API takes pointers to structs, string views, and out-parameters, so the
 * binding needs memory it can both write and name to native code. Slabs are
 * ordinary `ArrayBuffer`s whose address the transport reports, which keeps one
 * allocator and one struct encoder shared by both transports.
 */

import type { Ptr, Slab, Transport } from "./transport.ts";

/** Every allocation is at least eight-byte aligned, which the slot array needs. */
const DEFAULT_ALIGNMENT = 8;
const DEFAULT_SLAB_BYTES = 64 * 1024;

interface Block {
  offset: number;
  size: number;
}

interface ManagedSlab {
  readonly slab: Slab;
  readonly byteLength: number;
  /** Free blocks, ordered by offset and coalesced, so reuse stays predictable. */
  free: Block[];
}

/** Thrown when the binding is asked for memory it cannot obtain. */
export class MemoryError extends Error {
  override name = "MemoryError";
}

export class Memory {
  readonly #transport: Transport;
  readonly #slabs: ManagedSlab[] = [];
  /** Allocation address to its block, so a free knows what it is returning. */
  readonly #live = new Map<bigint, { slab: ManagedSlab; block: Block }>();

  constructor(transport: Transport) {
    this.#transport = transport;
  }

  get pointerSize(): 4 | 8 {
    return this.#transport.pointerSize;
  }

  /**
   * Allocates native-visible storage.
   *
   * The returned address stays valid until it is freed: slabs never move, and a
   * slab is never released while it holds a live allocation.
   */
  allocate(size: number, alignment: number = DEFAULT_ALIGNMENT): Ptr {
    if (!Number.isInteger(size) || size < 0) {
      throw new MemoryError(
        `allocation size must be a non-negative integer, not ${size}`,
      );
    }
    if (
      !Number.isInteger(alignment) ||
      alignment <= 0 ||
      (alignment & (alignment - 1)) !== 0
    ) {
      throw new MemoryError(
        `alignment must be a power of two, not ${alignment}`,
      );
    }
    const request = Math.max(size, 1);
    for (const managed of this.#slabs) {
      const address = this.#allocateIn(managed, request, alignment);
      if (address !== undefined) {
        return address;
      }
    }
    // The new slab is named directly rather than found by position: the list
    // is kept ordered by address so it can be searched, and where an allocator
    // places a fresh buffer differs by runtime.
    const managed = this.#addSlab(
      Math.max(DEFAULT_SLAB_BYTES, request + alignment),
    );
    const address = this.#allocateIn(managed, request, alignment);
    if (address === undefined) {
      throw new MemoryError(`a fresh slab could not satisfy ${request} bytes`);
    }
    return address;
  }

  /** Returns an allocation. Freeing an address twice is a binding error. */
  free(address: Ptr): void {
    const entry = this.#live.get(address);
    if (entry === undefined) {
      throw new MemoryError(`${address} does not name a live allocation`);
    }
    this.#live.delete(address);
    insertFree(entry.slab.free, entry.block);
    this.#retireIfEmpty(entry.slab);
  }

  /**
   * Retires a slab that holds nothing.
   *
   * One slab stays for the transport's life, because a binding that allocates
   * and frees in a loop would otherwise ask the transport for a new one each
   * time. Every slab beyond it goes back as soon as it empties, so a burst of
   * large temporary allocations does not become permanent memory.
   */
  #retireIfEmpty(managed: ManagedSlab): void {
    // The baseline is the slab this memory started with, not whichever one
    // sorts lowest: the list is ordered by address so it can be searched, and
    // which address an allocator hands out first differs by runtime.
    if (this.#slabs.length <= 1 || this.#baseline === managed) {
      return;
    }
    if (
      managed.free.length !== 1 ||
      managed.free[0]!.size !== managed.byteLength
    ) {
      return;
    }
    const index = this.#slabs.indexOf(managed);
    if (index < 0) {
      return;
    }
    this.#slabs.splice(index, 1);
    this.#transport.releaseSlab(managed.slab.base);
  }

  /**
   * Runs `body` with an allocator whose allocations are released afterwards.
   *
   * Native input storage lives exactly as long as the C call that borrows it, so
   * a scope is what most of the binding allocates through.
   */
  scope<T>(body: (scope: Scope) => T): T {
    const scope = new Scope(this);
    try {
      return body(scope);
    } finally {
      scope.release();
    }
  }

  /** Reports whether an address names memory this binding allocated. */
  owns(address: Ptr): boolean {
    return this.#find(address) !== undefined;
  }

  /** Reports the size of a live allocation, or `undefined` when there is none. */
  allocationSize(address: Ptr): number | undefined {
    return this.#live.get(address)?.block.size;
  }

  /**
   * Returns a view over binding-owned memory.
   *
   * Views are made per use rather than cached, because a WebAssembly transport
   * replaces a slab's buffer when its memory grows.
   */
  view(address: Ptr, length: number): DataView {
    const found = this.#find(address);
    if (found === undefined) {
      throw new MemoryError(`${address} is outside this binding's memory`);
    }
    const offset = Number(address - found.slab.base);
    if (offset + length > found.byteLength) {
      throw new MemoryError(
        `a ${length}-byte view at ${address} leaves its slab`,
      );
    }
    return new DataView(
      found.slab.buffer,
      found.slab.byteOffset + offset,
      length,
    );
  }

  /** Returns a byte view over binding-owned memory, under the same rules. */
  bytes(address: Ptr, length: number): Uint8Array {
    const found = this.#find(address);
    if (found === undefined) {
      throw new MemoryError(`${address} is outside this binding's memory`);
    }
    const offset = Number(address - found.slab.base);
    if (offset + length > found.byteLength) {
      throw new MemoryError(
        `a ${length}-byte view at ${address} leaves its slab`,
      );
    }
    return new Uint8Array(
      found.slab.buffer,
      found.slab.byteOffset + offset,
      length,
    );
  }

  /** Reads a pointer field, whose width follows the transport's ABI class. */
  readPointer(address: Ptr): Ptr {
    const view = this.view(address, this.pointerSize);
    return this.pointerSize === 8
      ? view.getBigUint64(0, true)
      : BigInt(view.getUint32(0, true));
  }

  /** Writes a pointer field, whose width follows the transport's ABI class. */
  writePointer(address: Ptr, value: Ptr): void {
    const view = this.view(address, this.pointerSize);
    if (this.pointerSize === 8) {
      view.setBigUint64(0, value, true);
      return;
    }
    if (value > 0xffff_ffffn) {
      throw new MemoryError(`${value} does not fit a 32-bit pointer`);
    }
    view.setUint32(0, Number(value), true);
  }

  #baseline: ManagedSlab | undefined;

  #addSlab(byteLength: number): ManagedSlab {
    const slab = this.#transport.addSlab(byteLength);
    const managed: ManagedSlab = {
      slab,
      byteLength,
      free: [{ offset: 0, size: byteLength }],
    };
    this.#slabs.push(managed);
    this.#baseline ??= managed;
    // Slabs are searched by address, and a binary search wants them ordered.
    this.#slabs.sort((left, right) =>
      left.slab.base < right.slab.base ? -1 : 1,
    );
    return managed;
  }

  #allocateIn(
    managed: ManagedSlab,
    size: number,
    alignment: number,
  ): Ptr | undefined {
    for (let index = 0; index < managed.free.length; index += 1) {
      const block = managed.free[index]!;
      const blockAddress = managed.slab.base + BigInt(block.offset);
      const padding = Number(-blockAddress & BigInt(alignment - 1));
      if (block.size < padding + size) {
        continue;
      }
      const offset = block.offset + padding;
      const remainderBefore = padding;
      const remainderAfter = block.size - padding - size;
      managed.free.splice(index, 1);
      if (remainderBefore > 0) {
        insertFree(managed.free, {
          offset: block.offset,
          size: remainderBefore,
        });
      }
      if (remainderAfter > 0) {
        insertFree(managed.free, {
          offset: offset + size,
          size: remainderAfter,
        });
      }
      const address = managed.slab.base + BigInt(offset);
      this.#live.set(address, { slab: managed, block: { offset, size } });
      return address;
    }
    return undefined;
  }

  #find(address: Ptr): ManagedSlab | undefined {
    let low = 0;
    let high = this.#slabs.length - 1;
    while (low <= high) {
      const middle = (low + high) >> 1;
      const managed = this.#slabs[middle]!;
      if (address < managed.slab.base) {
        high = middle - 1;
      } else if (address >= managed.slab.base + BigInt(managed.byteLength)) {
        low = middle + 1;
      } else {
        return managed;
      }
    }
    return undefined;
  }
}

/** A group of allocations released together. */
export class Scope {
  readonly #memory: Memory;
  readonly #allocations: Ptr[] = [];

  constructor(memory: Memory) {
    this.#memory = memory;
  }

  allocate(size: number, alignment?: number): Ptr {
    const address = this.#memory.allocate(size, alignment);
    this.#allocations.push(address);
    return address;
  }

  /** Allocates zeroed storage, which out-parameters and option structs need. */
  allocateZeroed(size: number, alignment?: number): Ptr {
    const address = this.allocate(size, alignment);
    this.#memory.bytes(address, size).fill(0);
    return address;
  }

  release(): void {
    // Freeing in reverse keeps coalescing cheap and makes reuse deterministic.
    for (let index = this.#allocations.length - 1; index >= 0; index -= 1) {
      this.#memory.free(this.#allocations[index]!);
    }
    this.#allocations.length = 0;
  }
}

/** Inserts a free block, keeping the list ordered and coalesced. */
function insertFree(free: Block[], block: Block): void {
  let index = 0;
  while (index < free.length && free[index]!.offset < block.offset) {
    index += 1;
  }
  free.splice(index, 0, block);
  const previous = free[index - 1];
  if (
    previous !== undefined &&
    previous.offset + previous.size === block.offset
  ) {
    previous.size += block.size;
    free.splice(index, 1);
    index -= 1;
  }
  const merged = free[index]!;
  const next = free[index + 1];
  if (next !== undefined && merged.offset + merged.size === next.offset) {
    merged.size += next.size;
    free.splice(index + 1, 1);
  }
}
