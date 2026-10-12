import '../../error/maplibre_exception.dart';

const int nativeStatusOk = 0;

const int nativeStatusInvalidArgument = -1;

const int nativeStatusInvalidState = -2;

const int nativeStatusUnsupported = -4;

const int nativeStatusNativeError = -5;

void checkNativeStatus(int statusCode, String Function() diagnostic) {
  if (statusCode == nativeStatusOk) {
    return;
  }
  throw MaplibreException.forNativeStatusCode(statusCode, diagnostic());
}

Never throwInvalidArgument(String diagnostic) {
  throw MaplibreException.invalidArgument(diagnostic);
}

Never throwInvalidState(String diagnostic) {
  throw MaplibreException.invalidState(diagnostic);
}
