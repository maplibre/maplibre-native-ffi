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

/// Starts a call that creates an owner in a pending state, such as a map
/// creation or a render-session attach. The call writes its owner's handle at
/// admission and completes once the owner is ready.
///
/// [adopt] gives the handle its owner as soon as native code admits the call,
/// and [result] pairs that owner with its readiness, which keeps the owner
/// reachable until it completes.
A _attach<T extends Object, A>(
  int Function(
    Allocator arena,
    Pointer<raw.mln_completion> completion,
    Pointer<Uint64> output,
  )
  start,
  T Function(int handle) adopt,
  A Function(T owner, Future<void> ready) result,
) {
  var handle = 0;
  final ready = submitNativeCompletion<void>(
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
    ready.ignore();
    rethrow;
  }
  return result(owner, ready.whenComplete(() => _keepAlive(owner)));
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

/// One borrowed-view scope of an owner, which native code holds open while a
/// synchronous `with` callback reads a view. Native keeps the frame alive and
/// reports busy if a release overlaps the scope.
final class _NativeViewScope {
  _NativeViewScope(this.owner, this.begin, this.end);
  final NativeHandleState<NativeHandle> owner;
  final int Function(int, Pointer<Pointer<Void>>, Pointer<raw.mln_diagnostic>)
  begin;
  final void Function(Pointer<Void>) end;
  bool _open = false;

  void checkActive() {
    if (!_open) {
      throwInvalidState(
        'a borrowed native view is readable only in its callback',
      );
    }
  }

  T active<T>(T value) {
    checkActive();
    return value;
  }

  /// Opens the scope, runs [callback] with it, and closes it.
  T use<T>(T Function(_NativeViewScope scope) callback) =>
      withNativeArena((arena) {
        final token = arena<Pointer<Void>>();
        _check(begin(owner.handle.raw, token, nativeDiagnostic));
        _open = true;
        try {
          final result = callback(this);
          if (result is Future) {
            throwInvalidArgument('a view callback must complete synchronously');
          }
          return result;
        } finally {
          _open = false;
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

/// Serializes a JSON value with `jsonEncode` into a view in arena. A record
/// member that is null, [member], is absent and converts to an empty view; a
/// parameter that is null is JSON null.
raw.mln_buffer_view _nativeJsonView(
  Object? value,
  Allocator arena, {
  bool member = false,
}) {
  if (member && value == null) return arena<raw.mln_buffer_view>().ref;
  return nativeStringView(jsonEncode(value), arena).value;
}

/// Parses UTF-8 JSON with `jsonDecode`; empty text is absent, null.
Object? _decodeJson(Uint8List bytes) =>
    bytes.isEmpty ? null : jsonDecode(utf8.decode(bytes));

/// Parses a JSON view with `jsonDecode`; an empty view is absent, null.
Object? _decodeJsonView(raw.mln_buffer_view view) =>
    _decodeJson(_copyBufferView(view));

Uint8List _copyBufferView(raw.mln_buffer_view view) {
  if (view.size == 0) return Uint8List(0);
  if (view.data == nullptr) {
    throwInvalidState('native completion returned an invalid buffer');
  }
  return Uint8List.fromList(view.data.cast<Uint8>().asTypedList(view.size));
}
