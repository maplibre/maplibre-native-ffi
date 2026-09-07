import 'dart:async';

/// Retains native callback resources until queued and active Dart upcalls drain.
abstract base class RetainedCallbackState {
  var _activeUpcalls = 0;
  var _retired = false;
  var _closeScheduled = false;
  var _closed = false;

  /// Whether all callback resources have closed, exposed for lifecycle tests.
  bool get closedForTesting => _closed;

  /// Runs [body] as one active Dart upcall.
  ///
  /// Returns false when this state is already closed and [body] was not run.
  /// Callers that own copied native payloads must release them on a false
  /// return.
  bool runUpcall(void Function() body) {
    if (_retired || _closed) {
      return false;
    }
    _activeUpcalls += 1;
    try {
      body();
    } finally {
      _activeUpcalls -= 1;
      _scheduleCloseIfReady();
    }
    return true;
  }

  /// Retires this callback state after queued and active upcalls drain.
  void close() {
    if (_retired) {
      return;
    }
    _retired = true;
    _scheduleCloseIfReady();
  }

  /// Retires and releases callback resources before this call returns.
  ///
  /// The caller must first detach the native callback so that no new upcall can
  /// start. An active upcall keeps the ordinary deferred close behavior.
  void closeSynchronously() {
    if (_closed) {
      return;
    }
    _retired = true;
    if (_activeUpcalls != 0) {
      _scheduleCloseIfReady();
      return;
    }
    _closed = true;
    closeResources();
  }

  void _scheduleCloseIfReady() {
    if (!_retired || _closed || _closeScheduled || _activeUpcalls != 0) {
      return;
    }
    _closeScheduled = true;
    Timer.run(() {
      _closeScheduled = false;
      if (!_retired || _closed || _activeUpcalls != 0) {
        _scheduleCloseIfReady();
        return;
      }
      _closed = true;
      closeResources();
    });
  }

  /// Releases native resources after the state has retired and upcalls drained.
  void closeResources();
}
