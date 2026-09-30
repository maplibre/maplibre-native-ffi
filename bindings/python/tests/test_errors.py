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
        result(map_handle.get_layer_property("missing", "visibility"))
    with pytest.raises(mln.InvalidStateError) as invalid_state:
        harness.runtime.close()
    with pytest.raises(mln.InvalidArgumentError) as invalid_argument:
        mln.network_status_set(mln.NetworkStatus(999_001))

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


def test_a_binding_failure_carries_no_native_status() -> None:
    runtime = mln.runtime_create()
    result(runtime.close())

    with pytest.raises(mln.InvalidStateError) as raised:
        runtime.map_create()

    assert raised.value.status == mln.Status.INVALID_STATE
    assert raised.value.native_status_code is None
    assert raised.value.diagnostic == "handle is closed"


def test_an_unknown_status_code_is_kept() -> None:
    error = mln.UnknownStatusError("", 12345)

    assert error.native_status_code == 12345
    assert error.status is not None
    assert error.status.is_unknown
    assert error.status.native_code == 12345
    # With no diagnostic, the message names the status.
    assert str(error) == "unknown 12345"
    assert str(mln.NotReadyError()) == "not ready"
