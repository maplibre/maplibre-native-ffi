import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

extension RuntimeTestWorkflows on RuntimeHandle {
  Future<MapHandle> createMap({MapOptions? options}) =>
      mapCreate(options ?? mapOptionsDefault());
  List<RuntimeEvent> drainCopiedEvents() {
    final batch = drainEvents();
    try {
      return batch.getValue().events;
    } finally {
      batch.close();
    }
  }

  RuntimeEventMask get eventMask => getEventMask();
}

extension RenderTestWorkflows on RenderSessionHandle {
  List<RenderFrameResult> drainCopiedFrameResults() {
    try {
      final batch = drainFrameResults();
      try {
        return [for (var i = 0; i < batch.count(); i++) batch.getValue(i)];
      } finally {
        batch.close();
      }
    } on NotReadyException {
      return const [];
    }
  }

  AcquiredFrameHandle? tryAcquireFrame() {
    try {
      return acquireFrame();
    } on NotReadyException {
      return null;
    }
  }
}
