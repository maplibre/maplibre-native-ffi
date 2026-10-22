/**
 * @file maplibre_native_c/binding.h
 * Machine-readable contracts for language binding generation.
 */

#ifndef MAPLIBRE_NATIVE_C_BINDING_H
#define MAPLIBRE_NATIVE_C_BINDING_H

/**
 * Attaches a binding generator contract to a declaration.
 *
 * The attribute has no effect on the C ABI or on other header consumers.
 */
#if defined(MLN_BINDGEN) && defined(__clang__)
#define MLN_BINDING(contract) __attribute__((annotate("mln:" contract)))
#else
#define MLN_BINDING(contract)
#endif

#endif  // MAPLIBRE_NATIVE_C_BINDING_H
