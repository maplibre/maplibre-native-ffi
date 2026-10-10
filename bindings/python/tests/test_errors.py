"""How native statuses and binding failures reach Python as exceptions."""

import maplibre_native_ffi as mln
import pytest
from support import Harness, result


def test_native_statuses_raise_their_exception_with_code_and_diagnostic(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    # An accepted call reports its status through the future; a rejected
    # submission raises before returning one.
    with pytest.raises(mln.NotFoundError) as not_found:
        result(map_handle.get_style_layer_property("missing", "visibility"))
    with pytest.raises(mln.InvalidStateError) as invalid_state:
        harness.runtime.close()
    with pytest.raises(mln.InvalidArgumentError) as invalid_argument:
        mln.network_set_status(mln.NetworkStatus(999_001))

    for raised, status in (
        (not_found, mln.Status.NOT_FOUND),
        (invalid_state, mln.Status.INVALID_STATE),
        (invalid_argument, mln.Status.INVALID_ARGUMENT),
    ):
        assert isinstance(raised.value, mln.MaplibreError)
        assert raised.value.status == status
        assert raised.value.native_status_code == status.native_code
        assert raised.value.diagnostic
        assert str(raised.value) == raised.value.diagnostic
    assert "network status" in invalid_argument.value.diagnostic

    # Native returns only the statuses it defines, so no call reaches the
    # mapping's unknown arm; the shared Rust core tests that a code it does
    # not know stays raw. The exception that arm raises keeps the code.
    unknown = mln.UnknownStatusError("", 12345)
    assert unknown.native_status_code == 12345
    assert unknown.status is not None
    assert unknown.status.is_unknown
    assert unknown.status.native_code == 12345
    # With no diagnostic, the message names the status.
    assert str(unknown) == "unknown 12345"


def test_a_binding_failure_carries_no_native_status() -> None:
    runtime = mln.runtime_create()
    result(runtime.close())

    with pytest.raises(mln.InvalidStateError) as raised:
        runtime.create_map()

    assert raised.value.status == mln.Status.INVALID_STATE
    assert raised.value.native_status_code is None
    assert raised.value.diagnostic == "RuntimeHandle is closed"
