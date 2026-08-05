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
  /**
   * The entrypoint returns a struct by value and slot 0 named no suitably
   * aligned caller storage for it.
   */
  MLN_ABI_CALL_BAD_RESULT_STORAGE = 4,
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

/** Which callback family a queued record came from. */
enum mln_abi_record_kind {
  MLN_ABI_RECORD_LOG = 1,
  MLN_ABI_RECORD_RESOURCE_REQUEST = 2,
  MLN_ABI_RECORD_CUSTOM_GEOMETRY_TILE = 3,
};

/**
 * A custom geometry tile request, copied for the host.
 *
 * MapLibre passes the tile id by value and returns immediately, so the id is
 * copied into a record this layer owns and the host releases through
 * mln_abi_record_destroy().
 */
typedef struct mln_abi_custom_geometry_tile {
  /** Zero for a fetch, one for a cancel. */
  uint32_t cancelled;
  uint32_t z;
  uint32_t x;
  uint32_t y;
} mln_abi_custom_geometry_tile;

/**
 * One record a MapLibre thread handed to this layer.
 *
 * `registration` is the listener_data the host gave the registration, so a
 * delivery names which registration it belongs to. `record` is the
 * adapter-owned record the host reads and then destroys, and is null for the
 * retirement the adapter delivers once a registration can no longer be reached.
 */
typedef struct mln_abi_record {
  uint32_t kind;
  uint64_t registration;
  uint64_t record;
} mln_abi_record;

/**
 * Creates an owner, which is one host execution context's identity here.
 *
 * A process holds one copy of this layer and may hold many host execution
 * contexts: a Node application runs a worker thread per runtime, and each
 * worker is a JavaScript realm with its own module graph while every worker
 * shares this one loaded library. A realm therefore cannot mint registration
 * identities of its own, and cannot be the only holder of a wake signal,
 * without taking another realm's records or silencing it. An owner is where
 * both of those live.
 *
 * Returns 0 when no owner can be created.
 */
uint64_t mln_abi_owner_create(void);

/**
 * Destroys an owner.
 *
 * Records still queued for it are released rather than left outstanding: no
 * context can reach them once their owner is gone. Registration identities the
 * owner minted stop naming anything, so a record a MapLibre thread pushes
 * afterwards is released the same way.
 *
 * Like mln_abi_queue_set_notifier(), this returns only once every call to this
 * owner's notifier has returned, so the caller may release what the notifier
 * read.
 */
void mln_abi_owner_destroy(uint64_t owner);

/**
 * Reserves a registration identity belonging to an owner.
 *
 * The identity is what the host stores as a registration's listener_data, so a
 * record names both the registration it belongs to and the owner it goes to.
 * Minting it here rather than in a host realm is what keeps two realms from
 * choosing the same one.
 *
 * The identity is a pointer-width value, because listener_data crosses the C
 * API as a void*, and it carries the owner in its high half. An owner that has
 * exhausted the low half, or that no longer exists, gets 0 rather than an
 * identity that would name someone else's registration.
 */
uint64_t mln_abi_owner_register(uint64_t owner);

/**
 * The listener a host registers for adapted log callbacks.
 *
 * A host with no way to mint a native function per registration uses this one
 * for every registration and tells them apart by their listener_data.
 */
void mln_abi_log_listener(void* listener_data, void* record);

/** The listener a host registers for adapted queued resource providers. */
void mln_abi_resource_request_listener(void* listener_data, void* request);

/**
 * Reports the address of the log listener.
 *
 * A host that reaches this layer through exported functions alone cannot take a
 * function's address, so the address is reported by a function of its own.
 */
void* mln_abi_log_listener_address(void);

/** Reports the address of the queued resource provider listener. */
void* mln_abi_resource_request_listener_address(void);

/** Reports the address of the custom geometry tile fetch listener. */
void* mln_abi_custom_geometry_fetch_listener_address(void);

/** Reports the address of the custom geometry tile cancel listener. */
void* mln_abi_custom_geometry_cancel_listener_address(void);

/**
 * Releases a record a drain delivered.
 *
 * Each family owns its records differently, and the host should not have to
 * know which; naming the kind is enough.
 */
void mln_abi_record_destroy(uint32_t kind, void* record);

/**
 * Installs the function this layer calls when a record is queued for an owner.
 *
 * The notifier runs on whichever MapLibre thread produced the record, so it
 * does the least possible: wake the owner's own execution context, which then
 * drains. Passing null clears this owner's. Each owner keeps its own, so a
 * context that installs one later does not silence the ones already there.
 *
 * This returns only once every call to the notifier it replaced has returned,
 * so the caller may release whatever the old notifier's user_data named. A
 * producer reads the notifier under a lock this does not hold while it waits,
 * which is what keeps a wake from deadlocking against the thread that produced
 * the record.
 *
 * A notifier therefore must not install or clear a notifier, or destroy an
 * owner, from inside itself: it would be waiting for its own call to return.
 */
void mln_abi_queue_set_notifier(
  uint64_t owner, void (*notify)(void* user_data), void* user_data
);

/**
 * Moves an owner's queued records into host storage.
 *
 * Returns how many records were written, and writes only records addressed to
 * this owner. A host drains until this reports fewer than the capacity it
 * offered.
 */
uint32_t mln_abi_queue_drain(
  uint64_t owner, mln_abi_record* records, uint32_t capacity
);

/** Reports how many of an owner's records are queued, for a drain to empty. */
uint32_t mln_abi_queue_depth(uint64_t owner);

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
