/**
 * @file mln_abi.c
 * Validation, diagnostic capture, and handle transfer around the generated
 * dispatch table.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "mln_abi.h"

#include "fingerprint.h"
#include "maplibre_native_c.h"
#include "maplibre_native_c/callback_adapter.h"

/*
 * Floating-point slots carry their bit pattern, because a slot is an integer.
 * memcpy is the portable spelling of that reinterpretation, and every compiler
 * this builds with folds it away.
 */
static float mln_abi_read_f32(uint64_t bits) {
  uint32_t narrow = (uint32_t)bits;
  float value;
  memcpy(&value, &narrow, sizeof(value));
  return value;
}

static double mln_abi_read_f64(uint64_t bits) {
  double value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

static uint64_t mln_abi_write_f32(float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  return (uint64_t)bits;
}

static uint64_t mln_abi_write_f64(double value) {
  uint64_t bits;
  memcpy(&bits, &value, sizeof(bits));
  return bits;
}

static const char* const mln_abi_entrypoint_names[] = {
#include "entrypoint_names.inc"
};

/* Which entrypoints report mln_status, so a failing call knows to capture the
 * thread-local diagnostic while it is still this call's. */
static const unsigned char mln_abi_result_is_status[] = {
#include "result_is_status.inc"
};

/* The alignment each struct-returning entrypoint's caller storage needs, and
 * zero for every entrypoint that returns something else. */
static const uint64_t mln_abi_result_struct_align[] = {
#include "result_struct_align.inc"
};

const char* mln_abi_fingerprint(void) { return MLN_ABI_FINGERPRINT_VALUE; }

const char* mln_abi_header_digest(void) { return MLN_ABI_HEADER_DIGEST; }

uint32_t mln_abi_entrypoint_count(void) { return MLN_ABI_ENTRYPOINT_COUNT; }

const char* mln_abi_entrypoint_name(uint32_t entrypoint) {
  if (entrypoint >= MLN_ABI_ENTRYPOINT_COUNT) {
    return NULL;
  }
  return mln_abi_entrypoint_names[entrypoint];
}

static void mln_abi_capture_diagnostic(
  char* diagnostic, uint32_t diagnostic_capacity, uint32_t* diagnostic_length
) {
  if (diagnostic_length != NULL) {
    *diagnostic_length = 0;
  }
  if (diagnostic == NULL || diagnostic_capacity == 0U) {
    return;
  }
  const char* message = mln_thread_last_error_message();
  if (message == NULL) {
    diagnostic[0] = '\0';
    return;
  }
  size_t length = strlen(message);
  if (length > (size_t)diagnostic_capacity - 1U) {
    length = (size_t)diagnostic_capacity - 1U;
  }
  memcpy(diagnostic, message, length);
  diagnostic[length] = '\0';
  if (diagnostic_length != NULL) {
    *diagnostic_length = (uint32_t)length;
  }
}

int32_t mln_abi_call(
  uint32_t entrypoint, void* slots_storage, char* diagnostic,
  uint32_t diagnostic_capacity, uint32_t* diagnostic_length
) {
  if (diagnostic_length != NULL) {
    *diagnostic_length = 0;
  }
  if (slots_storage == NULL) {
    return MLN_ABI_CALL_NULL_SLOTS;
  }
  /* Slots are read and written as uint64_t, so the array carries that
   * alignment. A host that allocates it elsewhere is told rather than left to
   * trap. */
  if (((uintptr_t)slots_storage & (uintptr_t)(sizeof(uint64_t) - 1U)) != 0U) {
    return MLN_ABI_CALL_MISALIGNED_SLOTS;
  }
  if (entrypoint >= MLN_ABI_ENTRYPOINT_COUNT) {
    return MLN_ABI_CALL_UNKNOWN_ENTRYPOINT;
  }

  uint64_t* slots = (uint64_t*)slots_storage;
  /* A struct return is stored through slot 0, so caller storage that is absent
   * or misaligned is reported rather than dereferenced. */
  const uint64_t result_align = mln_abi_result_struct_align[entrypoint];
  if (result_align != 0U) {
    const uintptr_t storage = (uintptr_t)slots[0];
    if (storage == 0U || (storage & (uintptr_t)(result_align - 1U)) != 0U) {
      return MLN_ABI_CALL_BAD_RESULT_STORAGE;
    }
  }
  (void)slots;
  (void)mln_abi_read_f32;
  (void)mln_abi_read_f64;
  (void)mln_abi_write_f32;
  (void)mln_abi_write_f64;

  switch (entrypoint) {
#include "dispatch.inc"
    default:
      return MLN_ABI_CALL_UNKNOWN_ENTRYPOINT;
  }

  if (
    mln_abi_result_is_status[entrypoint] != 0U &&
    (int32_t)(uint32_t)slots[0] != MLN_STATUS_OK
  ) {
    mln_abi_capture_diagnostic(
      diagnostic, diagnostic_capacity, diagnostic_length
    );
  }
  return MLN_ABI_CALL_OK;
}

void* mln_abi_symbol(uint32_t entrypoint) {
  switch (entrypoint) {
#include "symbols.inc"
    default:
      return NULL;
  }
}

/*
 * Handle transfer.
 *
 * The table is small and fixed: a host moves a wake source or a resource
 * request between contexts, not thousands of handles at once. A fixed table
 * keeps the claim path allocation-free and bounded, and a full table reports
 * failure rather than growing without a bound the host controls.
 */

#define MLN_ABI_TRANSFER_SLOTS 256U

typedef struct mln_abi_transfer_slot {
  uint64_t token;
  uint64_t handle;
} mln_abi_transfer_slot;

static mln_abi_transfer_slot mln_abi_transfers[MLN_ABI_TRANSFER_SLOTS];
static uint64_t mln_abi_transfer_sequence;

#if defined(_WIN32)
#include <windows.h>
static SRWLOCK mln_abi_transfer_lock = SRWLOCK_INIT;
static void mln_abi_transfer_acquire(void) {
  AcquireSRWLockExclusive(&mln_abi_transfer_lock);
}
static void mln_abi_transfer_release(void) {
  ReleaseSRWLockExclusive(&mln_abi_transfer_lock);
}
#else
#include <pthread.h>
static pthread_mutex_t mln_abi_transfer_lock = PTHREAD_MUTEX_INITIALIZER;
static void mln_abi_transfer_acquire(void) {
  pthread_mutex_lock(&mln_abi_transfer_lock);
}
static void mln_abi_transfer_release(void) {
  pthread_mutex_unlock(&mln_abi_transfer_lock);
}
#endif

uint64_t mln_abi_transfer_issue(uint64_t handle) {
  if (handle == 0U) {
    return 0U;
  }
  uint64_t token = 0U;
  mln_abi_transfer_acquire();
  /* Tokens never repeat, so a stale carrier cannot claim a later transfer that
   * reuses its slot. Issuance stops at the end of the sequence rather than
   * wrapping onto a token that is still outstanding. */
  if (mln_abi_transfer_sequence == UINT64_MAX) {
    mln_abi_transfer_release();
    return 0U;
  }
  for (uint32_t index = 0U; index < MLN_ABI_TRANSFER_SLOTS; ++index) {
    if (mln_abi_transfers[index].token == 0U) {
      token = ++mln_abi_transfer_sequence;
      mln_abi_transfers[index].token = token;
      mln_abi_transfers[index].handle = handle;
      break;
    }
  }
  mln_abi_transfer_release();
  return token;
}

static uint64_t mln_abi_transfer_take(uint64_t token) {
  if (token == 0U) {
    return 0U;
  }
  uint64_t handle = 0U;
  mln_abi_transfer_acquire();
  for (uint32_t index = 0U; index < MLN_ABI_TRANSFER_SLOTS; ++index) {
    if (mln_abi_transfers[index].token == token) {
      handle = mln_abi_transfers[index].handle;
      mln_abi_transfers[index].token = 0U;
      mln_abi_transfers[index].handle = 0U;
      break;
    }
  }
  mln_abi_transfer_release();
  return handle;
}

uint64_t mln_abi_transfer_claim(uint64_t token) {
  return mln_abi_transfer_take(token);
}

uint64_t mln_abi_transfer_discard(uint64_t token) {
  return mln_abi_transfer_take(token);
}
