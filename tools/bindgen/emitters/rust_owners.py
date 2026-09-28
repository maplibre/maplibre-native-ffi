"""Generate Rust handle owners from handle plans."""

from __future__ import annotations

from tools.bindgen.semantic import BoundApi, HandlePlan

from .rust import pascal


def owner_name(native: str) -> str:
    return pascal(native) + "Handle"


def state_name(native: str) -> str:
    return owner_name(native) + "State"


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
    native, owner, state = (
        handle.native,
        owner_name(handle.native),
        state_name(handle.native),
    )
    dispose = disposer(bound, handle)
    if dispose.return_type.kind == "void":
        finalize = f"|raw| {{ unsafe {{ sys::{dispose.name}(raw) }}; Ok(()) }}"
    else:
        finalize = f"|raw| unsafe {{ maplibre_core::generated::{module_name(native)}_dispose(raw) }}"
    parent_field = (
        f"    _parent: std::sync::Arc<crate::{state_name(handle.parent)}>,\n"
        if handle.parent
        else ""
    )
    parent_parameter = (
        f", parent: std::sync::Arc<crate::{state_name(handle.parent)}>"
        if handle.parent
        else ""
    )
    parent_value = " _parent: parent," if handle.parent else ""
    must_use = ""
    if handle.release_inputs or handle.dispose_invalidates == "parent":
        release = handle.release.removeprefix(native + "_")
        must_use = (
            f'#[must_use = "`{owner}` must be released with `{owner}::{release}`"]\n'
        )
    return (
        f"#[derive(Debug)]\n"
        f"pub(crate) struct {state} {{\n"
        f"    pub(crate) handle: crate::handle::ConcurrentNativeHandle<sys::{native}>,\n"
        f"    id: u64,\n"
        f"{parent_field}"
        f"}}\n"
        f"impl {state} {{\n"
        f"    pub(crate) fn native(&self) -> Result<sys::{native}> {{\n"
        f'        maplibre_core::callback::check("", 0)?;\n'
        f'        self.handle.live_handle().ok_or_else(|| crate::handle::closed_handle_error("{owner}"))\n'
        f"    }}\n"
        f"}}\n"
        f"impl Drop for {state} {{\n"
        f"    fn drop(&mut self) {{\n"
        f"        self.handle.finalize_with({finalize});\n"
        f"    }}\n"
        f"}}\n"
        f"/// Owns one `{native}` native handle.\n"
        f"{must_use}"
        f"pub struct {owner} {{\n"
        f"    pub(crate) inner: std::sync::Arc<{state}>,\n"
        f"}}\n"
        f"impl std::fmt::Debug for {owner} {{\n"
        f"    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {{\n"
        f'        f.debug_struct("{owner}").field("closed", &self.is_closed()).finish()\n'
        f"    }}\n"
        f"}}\n"
        f"impl {owner} {{\n"
        f"    pub(crate) fn from_native(raw: sys::{native}{parent_parameter}) -> Result<Self> {{\n"
        f"        // SAFETY: raw came from an accepted ownership transfer of this handle type.\n"
        f'        let handle = unsafe {{ crate::handle::ConcurrentNativeHandle::from_handle(raw, "{native}") }}?;\n'
        f"        Ok(Self {{ inner: std::sync::Arc::new({state} {{ handle, id: raw.0,{parent_value} }}) }})\n"
        f"    }}\n"
        f"\n"
        f"    /// Returns the native handle value, which event sources report for this handle.\n"
        f"    pub fn id(&self) -> u64 {{\n"
        f"        self.inner.id\n"
        f"    }}\n"
        f"\n"
        f"    /// Reports whether an explicit release, close, or disposal consumed this handle.\n"
        f"    pub fn is_closed(&self) -> bool {{\n"
        f"        self.inner.handle.is_closed()\n"
        f"    }}\n"
        f"}}\n"
    )


def module_index(owners: dict[str, HandlePlan], modules: list[str]) -> str:
    """Declare the generated modules and check every owner crosses threads."""
    return (
        "use crate::{NativeFuture, Result};\n"
        "use maplibre_native_ffi_core as maplibre_core;\n"
        "use maplibre_native_ffi_sys as sys;\n\n"
        + "".join(f"mod {module};\npub use {module}::*;\n" for module in modules)
        + "\n"
        + "".join(
            f"#[cfg(test)]\nstatic_assertions::assert_impl_all!({owner_name(native)}: Send, Sync);\n"
            for native in owners
        )
    )
