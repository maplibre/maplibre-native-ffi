/**
 * @file maplibre_native_c/binding.h
 * Machine-readable contracts for language binding generation.
 */

#ifndef MAPLIBRE_NATIVE_C_BINDING_H
#define MAPLIBRE_NATIVE_C_BINDING_H

/**
 * Attaches a binding contract to a declaration during header extraction.
 *
 * Keys and values are validated by tools/bindgen/schema.py. The attribute has
 * no effect on the C ABI, compiler warnings, or ordinary header consumers.
 */
#if defined(MLN_BINDGEN) && defined(__clang__)
#define MLN_BINDING(contract) __attribute__((annotate("mln:" contract)))
#else
#define MLN_BINDING(contract)
#endif

#endif  // MAPLIBRE_NATIVE_C_BINDING_H
