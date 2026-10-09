"""Generate Rust handle owners from handle plans."""

from __future__ import annotations

from tools.bindgen.semantic import BoundApi, HandlePlan

from .rust import pascal


def owner_name(native: str) -> str:
    return pascal(native) + "Handle"


def module_name(native: str) -> str:
    return native.removeprefix("mln_")


def disposer(bound: BoundApi, handle: HandlePlan):
    """The single-handle call `Drop` makes; an implicit release must be unconditional."""
    plan = bound.operations_by_name.get(handle.dispose or handle.release)
    if plan is None or len(plan.function.parameters) != 1:
        return None
    if not handle.dispose and plan.function.return_type.kind != "void":
        return None
    return plan.function


def owned_handles(bound: BoundApi) -> dict[str, HandlePlan]:
    """Public handles a Rust owner can hold and dispose from `Drop`."""
    decisions = {
        callback.decision.handle.native
        for callback in bound.callbacks.values()
        if callback.decision
    }
    candidates = {
        native: handle
        for native, handle in bound.public_handles.items()
        if native not in decisions and disposer(bound, handle)
    }
    # An owner retains its parent's state, so a parent without an owner leaves
    # its children without one too.
    while orphans := [
        native
        for native, handle in candidates.items()
        if handle.parent and handle.parent not in candidates
    ]:
        for native in orphans:
            del candidates[native]
    return dict(sorted(candidates.items()))


def declaration(bound: BoundApi, handle: HandlePlan) -> str:
    native, owner = handle.native, owner_name(handle.native)
    dispose = disposer(bound, handle)
    if dispose.return_type.kind == "void":
        finalize = f"|raw| {{ unsafe {{ sys::{dispose.name}(raw) }}; Ok(()) }}"
    else:
        finalize = f"|raw| maplibre_core::check(|out_diagnostic| unsafe {{ sys::{dispose.name}(raw, out_diagnostic) }})"
    must_use = ""
    if handle.release_inputs or handle.dispose_invalidates == "parent":
        release = handle.release.removeprefix(native + "_")
        must_use = f'    #[must_use = "`{owner}` must be released with `{owner}::{release}`"]\n'
    return (
        "native_owner! {\n"
        f"    /// Owns one `{native}` native handle.\n"
        f"{must_use}"
        f"    pub struct {owner}({native}) dispose {finalize};\n"
        "}\n"
    )


def module_index(owners: dict[str, HandlePlan], modules: list[str]) -> str:
    """Declare the generated modules and check every owner crosses threads."""
    return (
        "use crate::call::Call;\n"
        "use crate::callback;\n"
        "use crate::completion::{self, CommandCompletion, NativeFuture};\n"
        "use crate::convert::{self, FromNative, InputArena, ToNative, from_native, native_enum, native_flags, to_native};\n"
        "use crate::handle::native_owner;\n"
        "use crate::{Error, Result};\n"
        "use maplibre_native_ffi_core as maplibre_core;\n"
        "use maplibre_native_ffi_sys as sys;\n\n"
        + "".join(f"mod {module};\npub use {module}::*;\n" for module in modules)
        + "\n"
        + "".join(
            f"#[cfg(test)]\nstatic_assertions::assert_impl_all!({owner_name(native)}: Send, Sync);\n"
            for native in owners
        )
    )
