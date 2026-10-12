package org.maplibre.nativeffi.internal.callback

import android.util.Log
import org.maplibre.nativeffi.error.CallbackException

internal actual fun platformReportCallbackFailure(failure: CallbackException) {
  // Android's default uncaught exception handler ends the process, which would undo the
  // containment, so the failure goes to the log instead.
  Log.e("MaplibreNativeFfi", failure.message, failure)
}
