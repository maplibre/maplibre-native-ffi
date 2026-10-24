package org.maplibre.nativeffi.internal.callback

import android.util.Log
import org.maplibre.nativeffi.error.CallbackException

internal actual fun platformReportUncaught(failure: Throwable) {
  // Android's default uncaught exception handler ends the process, which would undo the
  // containment, so the failure goes to the log instead. A leaked handle is already disposed, so
  // its report is a warning.
  if (failure is CallbackException) {
    Log.e(LOG_TAG, failure.message, failure)
  } else {
    Log.w(LOG_TAG, failure.message, failure)
  }
}

internal const val LOG_TAG = "MaplibreNativeFfi"
