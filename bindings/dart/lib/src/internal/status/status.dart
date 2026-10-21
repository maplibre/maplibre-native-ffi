import '../../error/maplibre_exception.dart';
import '../../values.dart';
import '../c/maplibre_native_c.g.dart' as raw;

const int nativeStatusOk = raw.MLN_STATUS_OK;

const int nativeStatusInvalidArgument = raw.MLN_STATUS_INVALID_ARGUMENT;

const int nativeStatusInvalidState = raw.MLN_STATUS_INVALID_STATE;

const int nativeStatusUnsupported = raw.MLN_STATUS_UNSUPPORTED;

const int nativeStatusNativeError = raw.MLN_STATUS_NATIVE_ERROR;

void checkNativeStatus(int statusCode, String Function() diagnostic) {
  if (statusCode == nativeStatusOk) {
    return;
  }
  throw MaplibreException(Status.fromRawValue(statusCode), diagnostic());
}

Never throwInvalidArgument(String diagnostic) {
  throw MaplibreException(Status.invalidArgument, diagnostic);
}

Never throwInvalidState(String diagnostic) {
  throw MaplibreException(Status.invalidState, diagnostic);
}
