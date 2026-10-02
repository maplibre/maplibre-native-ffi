part of 'runtime.dart';

Uint8List _copyBufferView(raw.mln_buffer_view view) {
  if (view.size == 0) return Uint8List(0);
  if (view.data == nullptr) {
    throwInvalidState('native completion returned an invalid buffer');
  }
  return Uint8List.fromList(view.data.cast<Uint8>().asTypedList(view.size));
}
