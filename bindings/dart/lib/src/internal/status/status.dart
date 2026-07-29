import '../../error/maplibre_exception.dart';

/// Native C status code for success.
const int nativeStatusOk = 0;

/// Native C status code for invalid arguments.
const int nativeStatusInvalidArgument = -1;

/// Native C status code for invalid state.
const int nativeStatusInvalidState = -2;

/// Native C status code for wrong-thread access.
const int nativeStatusWrongThread = -3;

/// Native C status code for unsupported behavior.
const int nativeStatusUnsupported = -4;

/// Native C status code for converted native errors.
const int nativeStatusNativeError = -5;

/// Converts a native status code into the public Dart exception model.
void checkNativeStatus(int statusCode, String Function() diagnostic) {
  if (statusCode == nativeStatusOk) {
    return;
  }
  throw MaplibreException.forNativeStatusCode(statusCode, diagnostic());
}

/// Reports a Dart-side invalid argument before calling native code.
Never throwInvalidArgument(String diagnostic) {
  throw MaplibreException.invalidArgument(diagnostic);
}

/// Reports a Dart-side invalid state before calling native code.
Never throwInvalidState(String diagnostic) {
  throw MaplibreException.invalidState(diagnostic);
}

/// Reports Dart-side owner-isolate misuse before calling native code.
Never throwWrongThread(String diagnostic) {
  throw MaplibreException.forNativeStatusCode(
    nativeStatusWrongThread,
    diagnostic,
  );
}
