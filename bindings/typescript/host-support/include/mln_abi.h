/**
 * @file mln_abi.h
 * The normalized call ABI the TypeScript binding's transports share.
 *
 * JavaScript cannot pass a struct by value, cannot receive one by value, and
 * cannot spell a wasm32 struct-argument lowering. This layer normalizes every
 * public C entrypoint into one shape a host can express: an entrypoint id and a
 * pointer to an array of eight-byte slots. Slot 0 carries the return value, or
 * the address of caller storage when the entrypoint returns a struct. Slots 1
 * and up carry the arguments in declaration order, with a by-value struct
 * argument carried as the address of a copy the caller owns.
 *
 * The dispatch itself is generated from the public headers, so this file adds
 * validation, diagnostic capture, and handle transfer, and nothing else. Both
 * transports call the same generated code, which is what keeps the Node-API and
 * WebAssembly paths from drifting apart.
 *
 * This header is internal to the TypeScript binding rather than a public C API.
 */

#ifndef MLN_ABI_H
#define MLN_ABI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Outcomes of a normalized call. These describe the call, not its result. */
enum mln_abi_call_status {
  /** The entrypoint ran. Its own result is in slot 0. */
  MLN_ABI_CALL_OK = 0,
  /** The entrypoint id names no entrypoint in this build. */
  MLN_ABI_CALL_UNKNOWN_ENTRYPOINT = 1,
  /** The slot array was null. */
  MLN_ABI_CALL_NULL_SLOTS = 2,
  /** The slot array was not eight-byte aligned. */
  MLN_ABI_CALL_MISALIGNED_SLOTS = 3,
};

/** Reports the schema fingerprint this build's dispatch was generated from. */
const char* mln_abi_fingerprint(void);

/** Reports the digest of the public headers the dispatch was generated from. */
const char* mln_abi_header_digest(void);

/** Reports how many entrypoints this build dispatches. */
uint32_t mln_abi_entrypoint_count(void);

/** Names an entrypoint, or returns null for an id outside the table. */
const char* mln_abi_entrypoint_name(uint32_t entrypoint);

/**
 * Calls one public C entrypoint through the slot array.
 *
 * When the entrypoint reports `mln_status` and slot 0 comes back non-OK, the
 * thread-local diagnostic is copied into `diagnostic` before this function
 * returns, on the thread that made the call, with no host code in between. That
 * is the only moment at which the diagnostic is known to belong to this call.
 * `diagnostic_length` receives the byte length written, excluding the
 * terminator, and a message longer than the capacity is truncated.
 *
 * Returns an mln_abi_call_status value. The entrypoint's own status, if it has
 * one, lands in slot 0.
 */
int32_t mln_abi_call(
  uint32_t entrypoint, void* slots, char* diagnostic,
  uint32_t diagnostic_capacity, uint32_t* diagnostic_length
);

/**
 * Reports the address of a public entrypoint, so a host can store it in a
 * struct field the C API reads as a callback.
 *
 * Uses the same id space as mln_abi_call(). Returns null outside the table.
 */
void* mln_abi_symbol(uint32_t entrypoint);

/**
 * Issues a one-shot token naming a handle, for moving it to another host
 * execution context.
 *
 * A host cannot transfer ownership by sending the id itself: an id is copyable
 * data, so every receiver would become an owner. The token registry lives here,
 * in the one native process every host context shares, so exactly one claim can
 * succeed no matter how many copies of the token exist.
 *
 * Returns 0 when the token cannot be issued.
 */
uint64_t mln_abi_transfer_issue(uint64_t handle);

/**
 * Claims a token, reporting the handle it named.
 *
 * Returns 0 for a token that was never issued or has already been claimed.
 */
uint64_t mln_abi_transfer_claim(uint64_t token);

/**
 * Discards an unclaimed token, so a transfer that is never received releases
 * its registry entry.
 *
 * Returns the handle the token named, or 0 when the token is already gone.
 */
uint64_t mln_abi_transfer_discard(uint64_t token);

#ifdef __cplusplus
}
#endif

#endif /* MLN_ABI_H */
