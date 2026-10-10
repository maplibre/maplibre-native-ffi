/**
 * @file maplibre_native_c/binding.h
 * Machine-readable contracts for language binding generation.
 */

#ifndef MAPLIBRE_NATIVE_C_BINDING_H
#define MAPLIBRE_NATIVE_C_BINDING_H

/**
 * Attaches a binding contract to a declaration during header extraction.
 *
 * A contract states only what the declaration's C shape leaves open; the
 * conventions in tools/bindgen/schema.py supply the rest, and the schema
 * rejects a key that restates one of them. A function or callback typedef
 * carries its contract before the declaration; a parameter, field, or record
 * typedef carries it after the name. The attribute has no effect on the C ABI,
 * compiler warnings, or ordinary header consumers.
 */
#if defined(MLN_BINDGEN) && defined(__clang__)
#define MLN_BINDING(contract) __attribute__((annotate("mln:" contract)))
#else
#define MLN_BINDING(contract)
#endif

#endif  // MAPLIBRE_NATIVE_C_BINDING_H
