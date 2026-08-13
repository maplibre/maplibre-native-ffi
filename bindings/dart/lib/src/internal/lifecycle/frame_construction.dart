import '../status/status.dart';

/// Cleans up a native frame when its Dart wrapper could not be constructed.
///
/// A successful release transfers the descriptor to [releaseSucceeded]. A
/// failed release transfers it to [releaseFailed] so a retry can retain the
/// still-live native frame.
void cleanupFailedFrameConstruction({
  required int Function() release,
  required void Function() releaseSucceeded,
  required void Function() releaseFailed,
}) {
  if (release() == nativeStatusOk) {
    releaseSucceeded();
  } else {
    releaseFailed();
  }
}
