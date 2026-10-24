part of 'runtime.dart';

// Copy borrowed outputs before releasing the receiver. Callback registrations
// remain rooted until the native call accepts or rejects them.

typedef _NativeStart =
    int Function(Allocator arena, Pointer<raw.mln_completion> completion);

NativeCompletionStart _withArena(_NativeStart start) =>
    (completion) => withNativeArena((arena) => start(arena, completion));

/// Makes a call that writes text into storage the caller sizes, and copies
/// the text. A call reports the text's length whether or not it fit, and
/// reports the same text for the same inputs, so a call that did not fit is
/// made again with enough storage.
final class _CallerSizedText {
  Uint8List text = Uint8List(0);

  int call(
    Allocator arena,
    int Function(Pointer<Char> storage, int capacity, Pointer<Size> length)
    native,
  ) {
    final length = arena<Size>();
    var capacity = 256;
    while (true) {
      final storage = arena<Char>(capacity);
      final status = native(storage, capacity, length);
      if (status != nativeStatusOk) return status;
      if (length.value < capacity) {
        text = Uint8List.fromList(
          storage.cast<Uint8>().asTypedList(length.value),
        );
        return status;
      }
      capacity = length.value + 1;
    }
  }
}

final class _CompletionValue<T> {
  _CompletionValue(this.elementSize, this.read);

  /// The size of one element in this binding's header, which the native
  /// stride must cover.
  final int elementSize;

  final T Function(Pointer<Void> element) read;

  /// Reads each element of the result's array, which native lays out
  /// value_size bytes apart.
  List<T> readList(raw.mln_completion_result result) {
    final count = result.value_count;
    if (count == 0) return List<T>.unmodifiable(const []);
    final stride = result.value_size;
    if (stride < elementSize) {
      throwInvalidState('native completion stride is too small');
    }
    return List<T>.unmodifiable(
      List.generate(
        count,
        (index) => read((result.value.cast<Uint8>() + index * stride).cast()),
      ),
    );
  }
}

Future<CommandCompletion> _command(_NativeStart start) =>
    _startCommand(_withArena(start));

void _discard(_NativeStart start) =>
    withNativeArena((arena) => _check(start(arena, nullptr)));

Future<void> _run(_NativeStart start) =>
    startNativeCompletion(start: _withArena(start), decode: (_) {});

/// Starts an operation like [_run], but throws at the call if native rejects
/// it.
Future<void> _submit(_NativeStart start) =>
    submitNativeCompletion(start: _withArena(start), decode: (_) {});

Future<T> _query<T>(_CompletionValue<T> value, _NativeStart start) =>
    startNativeCompletion(
      start: _withArena(start),
      decode: (result) => value.read(result.value),
    );

Future<T?> _queryOptional<T>(_CompletionValue<T?> value, _NativeStart start) =>
    startNativeCompletion(
      start: _withArena(start),
      decode: (result) =>
          result.value_count == 0 ? null : value.read(result.value),
    );

Future<List<T>> _queryList<T>(_CompletionValue<T> value, _NativeStart start) =>
    startNativeCompletion(start: _withArena(start), decode: value.readList);

// ignore: unused_element
Future<List<T>?> _queryOptionalList<T>(
  _CompletionValue<T> value,
  _NativeStart start,
) => startNativeCompletion(
  start: _withArena(start),
  decode: (result) => result.value == nullptr ? null : value.readList(result),
);

/// Starts an operation whose completion transfers one handle, which [adopt]
/// gives its owner once the binding has taken it from the delivery batch.
Future<T> _queryOwned<T>(_NativeStart start, T Function(int handle) adopt) =>
    startNativeCompletion(
      start: _withArena(start),
      decode: (result) => adopt(result.value.cast<Uint64>().value),
      ownsHandle: true,
    );

/// Starts an attachment, which writes its owner's handle at admission and
/// completes once attachment finishes.
///
/// [adopt] gives the handle its owner as soon as native code admits the call,
/// and [attachment] pairs that owner with the completion, which keeps the
/// owner reachable until it completes.
A _attach<T extends Object, A>(
  int Function(
    Allocator arena,
    Pointer<raw.mln_completion> completion,
    Pointer<Uint64> output,
  )
  start,
  T Function(int handle) adopt,
  A Function(T owner, Future<void> completed) attachment,
) {
  var handle = 0;
  final completed = submitNativeCompletion<void>(
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

/// Takes the native registration that [value] carries for one call, or returns
/// null when it carries none. A registration that an earlier call carried is
/// invalid state, whatever native code did with it.
NativeRegistration? _takeNativeRegistration(Object value) {
  final native = nativeRegistrations[value];
  if (native == null) return null;
  if (native.consumed) {
    throwInvalidState('a native registration reaches only its first call');
  }
  native.consumed = true;
  return native;
}

/// Native acceptance keeps registrations rooted until native release.
/// Every other outcome rejects them before the call returns.
final class _NativeRegistrations {
  _NativeRegistrations(this.roots);
  final _CallbackRoots roots;
  final _pending = <_NativeRegistration>[];
  bool _accepted = false;

  Pointer<T> add<T extends Struct>(_NativeRegistration<T> registration) {
    _pending.add(registration);
    return registration.pointer;
  }

  /// Runs the call that carries this transaction's descriptors, accepts them
  /// when it succeeds, and returns its status. A successful call for which
  /// [declined] reports that native kept nothing accepts none of them.
  int run(int Function() call, {bool Function()? declined}) {
    try {
      final status = call();
      if (status == nativeStatusOk && !(declined?.call() ?? false)) {
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
/// synchronous `withView` callback reads a view. Native keeps the frame alive
/// and reports busy if a release overlaps the scope.
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

  T active<T>(T value) {
    checkActive();
    return value;
  }

  T use<T>(T Function() callback) => withNativeArena((arena) {
    final token = arena<Pointer<Void>>();
    _check(begin(owner.handle.raw, token, nativeDiagnostic));
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
  });
}

int _nativeInteger(int value, int minimum, int maximum) {
  if (value < minimum || value > maximum) {
    throwInvalidArgument('integer is outside its native range');
  }
  return value;
}

Uint8List _copyBufferView(raw.mln_buffer_view view) {
  if (view.size == 0) return Uint8List(0);
  if (view.data == nullptr) {
    throwInvalidState('native completion returned an invalid buffer');
  }
  return Uint8List.fromList(view.data.cast<Uint8>().asTypedList(view.size));
}
