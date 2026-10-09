part of 'runtime.dart';

// The steps that generated operations take around their native calls. A
// generated operation names its C function, maps its arguments, and passes the
// call to one of these helpers, which own completion wiring, admission, and
// callback registration.

/// Calls a C function that takes a completion, with a scratch arena for its
/// arguments, and returns the call's status.
typedef _NativeStart =
    int Function(Arena arena, Pointer<raw.mln_completion> completion);

NativeCompletionStart _withArena(_NativeStart start) =>
    (completion) => withNativeArena((arena) => start(arena, completion));

/// How a completion copies one native value, and how the binding reads it.
final class _CompletionValue<T> {
  _CompletionValue(this.copyKind, this.elementSize, this.read);

  final int copyKind;
  final int elementSize;

  /// Reads the value stored at an element's address.
  final T Function(Pointer<Void> element) read;

  List<T> readList(raw.mln_completion_result result) => List<T>.unmodifiable(
    List.generate(
      result.value_count,
      (index) =>
          read((result.value.cast<Uint8>() + index * elementSize).cast()),
    ),
  );
}

/// Starts an ordered command.
Future<CommandCompletion> _command(_NativeStart start) =>
    _startCommand(_withArena(start));

/// Starts an operation that completes without a value.
Future<void> _run(_NativeStart start) => startNativeCompletion(
  copyKind: raw.MLN_ADAPTER_COMPLETION_COPY_FLAT,
  elementSize: 0,
  start: _withArena(start),
  decode: (_) {},
);

/// Starts an operation whose completion carries one value.
Future<T> _query<T>(_CompletionValue<T> value, _NativeStart start) =>
    startNativeCompletion(
      copyKind: value.copyKind,
      elementSize: value.elementSize,
      start: _withArena(start),
      decode: (result) => value.read(result.value),
    );

/// Starts an operation whose completion carries zero or one value.
Future<T?> _queryOptional<T>(_CompletionValue<T?> value, _NativeStart start) =>
    startNativeCompletion(
      copyKind: value.copyKind,
      elementSize: value.elementSize,
      start: _withArena(start),
      decode: (result) =>
          result.value_count == 0 ? null : value.read(result.value),
    );

/// Starts an operation whose completion carries an array.
Future<List<T>> _queryList<T>(_CompletionValue<T> value, _NativeStart start) =>
    startNativeCompletion(
      copyKind: value.copyKind,
      elementSize: value.elementSize,
      start: _withArena(start),
      decode: value.readList,
    );

/// Starts an operation whose completion carries an array or no array.
Future<List<T>?> _queryOptionalList<T>(
  _CompletionValue<T> value,
  _NativeStart start,
) => startNativeCompletion(
  copyKind: value.copyKind,
  elementSize: value.elementSize,
  start: _withArena(start),
  decode: (result) => result.value == nullptr ? null : value.readList(result),
);

/// Starts an operation whose completion transfers one handle, which [adopt]
/// gives its owner once the binding has claimed the native record.
Future<T> _queryOwned<T>(
  int copyKind,
  _NativeStart start,
  T Function(int handle) adopt,
) => startNativeCompletion(
  copyKind: copyKind,
  elementSize: sizeOf<Uint64>(),
  start: _withArena(start),
  decode: (result) => adopt(result.value.cast<Uint64>().value),
  claimBeforeDecode: true,
);

/// Starts an attachment, which writes its owner's handle at admission and
/// completes once attachment finishes.
///
/// [adopt] gives the handle its owner as soon as native code admits the call,
/// and [attachment] pairs that owner with the completion, which keeps the
/// owner reachable until it completes.
A _attach<T extends Object, A>(
  int Function(
    Arena arena,
    Pointer<raw.mln_completion> completion,
    Pointer<Uint64> output,
  )
  start,
  T Function(int handle) adopt,
  A Function(T owner, Future<void> completed) attachment,
) {
  var handle = 0;
  final completed = startNativeCompletion<void>(
    copyKind: raw.MLN_ADAPTER_COMPLETION_COPY_FLAT,
    elementSize: 0,
    start: (completion) => withNativeArena((arena) {
      final output = arena<Uint64>();
      final status = start(arena, completion, output);
      handle = output.value;
      return status;
    }),
    decode: (_) {},
  );
  final T owner;
  try {
    owner = adopt(handle);
  } catch (_) {
    completed.ignore();
    rethrow;
  }
  return attachment(owner, completed.whenComplete(() => _keepAlive(owner)));
}

@pragma('vm:never-inline')
@pragma('dart2js:noInline')
void _keepAlive(Object? value) {}

/// One prepared callback descriptor, which its registration transaction
/// rejects unless native code accepts the call that carries it.
final class _NativeRegistration<T extends Struct> {
  const _NativeRegistration(this.pointer, this.reject, [this.releaseMemory]);
  final Pointer<T> pointer;
  final void Function() reject;
  final void Function()? releaseMemory;
}

/// The callback registrations of one native call.
///
/// A registration roots its callbacks before the call. Native acceptance keeps
/// them until native release; any other outcome rejects them at once.
final class _NativeRegistrations {
  _NativeRegistrations(this.ports);
  final _NativeCallbackPorts ports;
  final _pending = <_NativeRegistration>[];
  bool _accepted = false;

  /// Adds a prepared descriptor to this transaction and returns its storage.
  Pointer<T> add<T extends Struct>(_NativeRegistration<T> registration) {
    _pending.add(registration);
    return registration.pointer;
  }

  /// Runs the call that carries this transaction's descriptors, accepts them
  /// when it succeeds, and returns its status.
  int run(int Function() call) {
    try {
      final status = call();
      if (status == nativeStatusOk) {
        _accepted = true;
      }
      return status;
    } finally {
      close();
    }
  }

  void close() {
    for (final registration in _pending.reversed) {
      if (!_accepted) {
        registration.reject();
      }
      registration.releaseMemory?.call();
    }
    _pending.clear();
  }
}

/// The borrowed-view scope of an owner, which native code holds open while a
/// synchronous `withView` callback reads a view. The view reads its owner, so
/// the owner refuses to close until the callback returns.
final class _NativeViewScope {
  _NativeViewScope(this.owner, this.begin, this.end);
  final NativeHandleState<NativeHandle> owner;
  final int Function(int, Pointer<Pointer<Void>>, Pointer<raw.mln_diagnostic>)
  begin;
  final void Function(Pointer<Void>) end;
  int _active = 0;

  void checkActive() {
    if (_active == 0) {
      throwInvalidState(
        'borrowed native value requires an active withView callback',
      );
    }
  }

  /// Returns [value] once a view is known to be active.
  T active<T>(T value) {
    checkActive();
    return value;
  }

  T use<T>(T Function() callback) => owner.read(
    (handle) => withNativeArena((arena) {
      final token = arena<Pointer<Void>>();
      _check(begin(handle.raw, token, nativeDiagnostic));
      _active++;
      try {
        final result = callback();
        if (result is Future) {
          throwInvalidArgument('withView callback must complete synchronously');
        }
        return result;
      } finally {
        _active--;
        end(token.value);
      }
    }),
  );
}

/// Checks that an integer fits its native field before it is stored.
int _nativeInteger(int value, int minimum, int maximum) {
  if (value < minimum || value > maximum) {
    throwInvalidArgument('integer is outside its native range');
  }
  return value;
}

/// Copies a native byte view into a Dart-owned list.
Uint8List _copyBufferView(raw.mln_buffer_view view) {
  if (view.size == 0) return Uint8List(0);
  if (view.data == nullptr) {
    throwInvalidState('native completion returned an invalid buffer');
  }
  return Uint8List.fromList(view.data.cast<Uint8>().asTypedList(view.size));
}

/// Decodes one message from the UTF-8 arena of a native record array.
String _arenaUtf8(Pointer<Uint8> data, int size, int offset, int length) {
  if (offset < 0 || length < 0 || offset > size || length > size - offset) {
    throwInvalidState('native message slice exceeds its arena');
  }
  return length == 0 ? '' : utf8.decode((data + offset).asTypedList(length));
}
