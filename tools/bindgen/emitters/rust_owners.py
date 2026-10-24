"""Generate Rust handle owners from handle plans."""

from __future__ import annotations

from tools.bindgen.semantic import BoundApi, HandlePlan

from .rust import counted_owner, doc, native_call, pascal


def owner_name(native: str) -> str:
    return pascal(native) + "Handle"


def module_name(native: str) -> str:
    return native.removeprefix("mln_")


def disposer(bound: BoundApi, handle: HandlePlan):
    """The single-handle call `Drop` makes; an implicit release must be unconditional.

    A dispose that takes a completion gets a null one, so it reports nothing.
    """
    plan = bound.operations_by_name.get(handle.cleanup or handle.release)
    if plan is None or [p.name for p in plan.function.parameters[1:]] != [
        name for name in [handle.dispose_completion] if name
    ]:
        return None
    if not handle.cleanup and plan.function.return_type.kind != "void":
        return None
    return plan.function


def event_sources(bound: BoundApi) -> frozenset[str]:
    """The handles that runtime events name as their source."""
    prefix = "MLN_RUNTIME_EVENT_SOURCE_"
    return frozenset(
        "mln_" + member.name.removeprefix(prefix).lower()
        for enum in bound.source.enums
        if enum.name == "mln_runtime_event_source_type"
        for member in enum.values
    )


def owned_handles(bound: BoundApi) -> dict[str, HandlePlan]:
    """Public handles a Rust owner can hold and dispose from `Drop`."""
    candidates = {
        native: handle
        for native, handle in bound.public_handles.items()
        if disposer(bound, handle)
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
    arguments = ["raw", *(["std::ptr::null()"] if handle.dispose_completion else [])]
    if dispose.return_type.kind == "void":
        finalize = f"|raw| {{ unsafe {{ {native_call(dispose, arguments)} }}; Ok(()) }}"
    else:
        call = native_call(dispose, arguments, diagnostic="out_diagnostic")
        finalize = f"|raw| maplibre_core::check(|out_diagnostic| unsafe {{ {call} }})"
    counted = counted_owner(bound, native)
    must_use = ""
    if handle.release_inputs:
        release = handle.release.removeprefix(native + "_")
        must_use = f'    #[must_use = "`{owner}` must be released with `{owner}::{release}`"]\n'
    id_doc = "    /// Returns the native handle value"
    id_doc += (
        ", which runtime events report as their source.\n"
        if native in event_sources(bound)
        else ".\n"
    )
    return (
        "native_owner! {\n"
        f"    /// Owns one `{native}` native handle.\n"
        + (f"    ///\n{summary}" if (summary := doc(bound, native, "    ")) else "")
        + f"{must_use}"
        f"    pub struct {owner}({native}) dispose {finalize}, counted {str(counted).lower()};\n"
        f"{id_doc}"
        "    fn id;\n"
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
