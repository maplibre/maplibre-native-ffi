"""Generate Rust handle owners from handle plans."""

from __future__ import annotations

from tools.bindgen.semantic import BoundApi, HandlePlan

from .rust import doc, native_call, pascal


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


def owned_handles(bound: BoundApi) -> dict[str, HandlePlan]:
    """Public handles a Rust owner can hold and dispose from `Drop`."""
    return dict(
        sorted(
            (native, handle)
            for native, handle in bound.public_handles.items()
            if disposer(bound, handle)
        )
    )


def declaration(bound: BoundApi, handle: HandlePlan) -> str:
    native, owner = handle.native, owner_name(handle.native)
    dispose = disposer(bound, handle)
    arguments = ["raw", *(["std::ptr::null()"] if handle.dispose_completion else [])]
    if dispose.return_type.kind == "void":
        finalize = f"|raw| {{ unsafe {{ {native_call(dispose, arguments)} }}; Ok(()) }}"
    else:
        call = native_call(dispose, arguments, diagnostic="out_diagnostic")
        finalize = f"|raw| crate::error::check(|out_diagnostic| unsafe {{ {call} }})"
    must_use = ""
    if handle.release_inputs:
        release = handle.release.removeprefix(native + "_")
        must_use = f'    #[must_use = "`{owner}` must be released with `{owner}::{release}`"]\n'
    id_doc = "    /// Returns the native handle value"
    id_doc += (
        ", which runtime events report as their source.\n"
        if native in bound.event_sources
        else ".\n"
    )
    return (
        "native_owner! {\n"
        "    /// Owns one native handle.\n"
        + (f"    ///\n{summary}" if (summary := doc(bound, native, "    ")) else "")
        + f"{must_use}"
        f"    pub struct {owner}({native}) dispose {finalize};\n"
        f"{id_doc}"
        "    fn id;\n"
        "}\n"
    )


def view_guard(bound: BoundApi, handle: HandlePlan, getters: list[str]) -> str:
    """The guard of a handle's view scope, whose getters lend what native keeps
    valid until the scope ends. The guard borrows its owner, so the owner can't
    be consumed while it lives."""
    assert handle.view_begin and handle.view_end
    owner = owner_name(handle.native)
    guard = pascal(handle.native) + "View"
    return (
        f"\n/// A view scope of [`{owner}`], which ends when it drops. Its\n"
        "/// getters lend values that stay valid until then, and its owner can't\n"
        "/// be consumed while it lives.\n"
        "///\n" + doc(bound, handle.view_begin) + f"pub struct {guard}<'a> {{\n"
        f"    owner: &'a {owner},\n"
        "    _scope: crate::call::ViewScope,\n"
        "}\n"
        f"impl std::fmt::Debug for {guard}<'_> {{\n"
        "    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {\n"
        f'        write!(f, "{guard}(id={{:#x}})", self.owner.id())\n'
        "    }\n}\n"
        + f"impl {owner} {{\n"
        + doc(bound, handle.view_begin, "    ")
        + f"    pub fn view(&self) -> Result<{guard}<'_>> {{\n"
        "        // SAFETY: these are this handle type's view scope functions.\n"
        "        let scope = unsafe {\n"
        "            crate::call::ViewScope::begin(\n"
        "                self.inner.native(),\n"
        f"                sys::{handle.view_begin},\n"
        f"                sys::{handle.view_end},\n"
        "            )\n"
        "        }?;\n"
        "        Ok(" + guard + " { owner: self, _scope: scope })\n"
        "    }\n}\n"
        f"impl {guard}<'_> {{\n" + "\n".join(getters) + "}\n"
    )


def queued_view(handle: HandlePlan, methods: list[str]) -> str:
    """The view that makes a handle's calls in queue mode: its queue holds each
    call's completion handler and the handlers of the registrations it
    carries."""
    owner = owner_name(handle.native)
    view = f"{owner}Queued"
    return (
        f"\n/// Makes [`{owner}`] calls in queue mode on a [`DeliveryQueue`]. The queue\n"
        "/// holds each call's completion handler, which receives the result when\n"
        "/// a drain of the queue runs it, and the handlers of the registrations\n"
        "/// that the call carries. A call fails with `Status::InvalidState` once\n"
        "/// the queue is closed.\n"
        f"pub struct {view}<'a, K: QueueKind>(&'a {owner}, &'a DeliveryQueue<K>);\n"
        f"impl {owner} {{\n"
        f"    pub fn queued<'a, K: QueueKind>(&'a self, queue: &'a DeliveryQueue<K>) -> {view}<'a, K> {{\n"
        f"        {view}(self, queue)\n"
        "    }\n}\n"
        f"impl<K: QueueKind> {view}<'_, K> {{\n" + "\n".join(methods) + "}\n"
    )


def queued_discarding_view(handle: HandlePlan, methods: list[str]) -> str:
    """The view that submits a handle's commands in queue mode without a
    completion: its queue holds only the registrations' handlers."""
    owner = owner_name(handle.native)
    queued = f"{owner}Queued"
    view = f"{owner}QueuedDiscarding"
    return (
        f"\n/// Submits [`{owner}`] commands in queue mode without a completion. The\n"
        "/// queue holds the handlers of the registrations that each call carries,\n"
        "/// and a call fails with `Status::InvalidState` once the queue is closed.\n"
        f"pub struct {view}<'a, K: QueueKind>(&'a {owner}, &'a DeliveryQueue<K>);\n"
        f"impl<'a, K: QueueKind> {queued}<'a, K> {{\n"
        f"    pub fn discarding(&self) -> {view}<'a, K> {{\n"
        f"        {view}(self.0, self.1)\n"
        "    }\n}\n"
        f"impl<K: QueueKind> {view}<'_, K> {{\n" + "\n".join(methods) + "}\n"
    )


def module_index(
    owners: dict[str, HandlePlan], modules: list[str], *, delivers: bool
) -> str:
    """Declare the generated modules and check that every owner crosses
    threads and that none can be copied, since each is the only owner of its
    handle."""
    return (
        "use crate::call::{Call, Lent};\n"
        "use crate::callback;\n"
        "use crate::completion::{self, CommandCompletion, NativeFuture};\n"
        + (
            "use crate::delivery::{self, Callback, CompletionHandler, DeliveryQueue, Local, QueueKind, Shared};\n"
            if delivers
            else ""
        )
        + "use crate::convert::{self, FromNative, InputArena, ToNative, from_native, native_enum, native_flags, to_native};\n"
        "use crate::handle::native_owner;\n"
        "use crate::{Error, Result};\n"
        "use maplibre_native_ffi_sys as sys;\n\n"
        + "".join(f"mod {module};\npub use {module}::*;\n" for module in modules)
        + "\n"
        + "".join(
            f"#[cfg(test)]\nstatic_assertions::assert_impl_all!({owner_name(native)}: Send, Sync);\n"
            f"#[cfg(test)]\nstatic_assertions::assert_not_impl_any!({owner_name(native)}: Clone);\n"
            for native in owners
        )
    )
