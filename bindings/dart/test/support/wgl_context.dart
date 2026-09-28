import 'dart:ffi';

import 'package:ffi/ffi.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

typedef _Pointer1 = Pointer<Void> Function(Pointer<Void>);
typedef _Bool1Native = Int32 Function(Pointer<Void>);
typedef _Bool1 = int Function(Pointer<Void>);
typedef _Bool2Native = Int32 Function(Pointer<Void>, Pointer<Void>);
typedef _Bool2 = int Function(Pointer<Void>, Pointer<Void>);

/// A pbuffer keeps the fixture independent of Dart's current OS thread.
/// The bootstrap window is created and destroyed within one synchronous call.
/// Pbuffer DCs support ordinary shared rendering contexts:
/// https://registry.khronos.org/OpenGL/extensions/ARB/WGL_ARB_pbuffer.txt
final class WglTestContext {
  WglTestContext() {
    using((arena) {
      final className = 'MaplibreDartWglTests'.toNativeUtf16(allocator: arena);
      final instance = _kernel
          .lookupFunction<
            Pointer<Void> Function(Pointer<Utf16>),
            Pointer<Void> Function(Pointer<Utf16>)
          >('GetModuleHandleW')(nullptr);
      final windowClass = arena<_WindowClass>();
      windowClass.ref
        ..style =
            0x20 // CS_OWNDC
        ..procedure = _user
            .lookup<
              NativeFunction<
                IntPtr Function(Pointer<Void>, Uint32, UintPtr, IntPtr)
              >
            >('DefWindowProcW')
        ..instance = instance
        ..className = className;
      _require(
        _user.lookupFunction<
              Uint16 Function(Pointer<_WindowClass>),
              int Function(Pointer<_WindowClass>)
            >('RegisterClassW')(windowClass) !=
            0,
        'RegisterClassW',
      );
      var window = nullptr.cast<Void>();
      var dc = nullptr.cast<Void>();
      var bootstrap = nullptr.cast<Void>();
      final previousDc = _gl
          .lookupFunction<Pointer<Void> Function(), Pointer<Void> Function()>(
            'wglGetCurrentDC',
          )();
      final previousContext = _gl
          .lookupFunction<Pointer<Void> Function(), Pointer<Void> Function()>(
            'wglGetCurrentContext',
          )();
      try {
        window =
            _user.lookupFunction<
              Pointer<Void> Function(
                Uint32,
                Pointer<Utf16>,
                Pointer<Utf16>,
                Uint32,
                Int32,
                Int32,
                Int32,
                Int32,
                Pointer<Void>,
                Pointer<Void>,
                Pointer<Void>,
                Pointer<Void>,
              ),
              Pointer<Void> Function(
                int,
                Pointer<Utf16>,
                Pointer<Utf16>,
                int,
                int,
                int,
                int,
                int,
                Pointer<Void>,
                Pointer<Void>,
                Pointer<Void>,
                Pointer<Void>,
              )
            >('CreateWindowExW')(
              0,
              className,
              className,
              0x00CF0000,
              0,
              0,
              8,
              8,
              nullptr,
              nullptr,
              instance,
              nullptr,
            );
        _require(window != nullptr, 'CreateWindowExW');
        dc = _user.lookupFunction<_Pointer1, _Pointer1>('GetDC')(window);
        _require(dc != nullptr, 'GetDC');
        final format = arena<_PixelFormat>();
        format.ref
          ..size = sizeOf<_PixelFormat>()
          ..version = 1
          ..flags =
              0x25 // DRAW_TO_WINDOW | SUPPORT_OPENGL | DOUBLEBUFFER
          ..colorBits = 32
          ..depthBits = 24
          ..stencilBits = 8;
        final chosen = _gdi
            .lookupFunction<
              Int32 Function(Pointer<Void>, Pointer<_PixelFormat>),
              int Function(Pointer<Void>, Pointer<_PixelFormat>)
            >('ChoosePixelFormat')(dc, format);
        _require(chosen != 0, 'ChoosePixelFormat');
        _require(
          _gdi.lookupFunction<
                Int32 Function(Pointer<Void>, Int32, Pointer<_PixelFormat>),
                int Function(Pointer<Void>, int, Pointer<_PixelFormat>)
              >('SetPixelFormat')(dc, chosen, format) !=
              0,
          'SetPixelFormat',
        );
        bootstrap = _createContext(dc);
        _require(bootstrap != nullptr, 'wglCreateContext');
        _require(_makeCurrent(dc, bootstrap) != 0, 'wglMakeCurrent');
        _releaseDc = _extension(
          'wglReleasePbufferDCARB',
          arena,
        ).cast<NativeFunction<_Bool2Native>>().asFunction<_Bool2>();
        _destroyPbuffer = _extension(
          'wglDestroyPbufferARB',
          arena,
        ).cast<NativeFunction<_Bool1Native>>().asFunction<_Bool1>();
        final choose = _extension('wglChoosePixelFormatARB', arena)
            .cast<
              NativeFunction<
                Int32 Function(
                  Pointer<Void>,
                  Pointer<Int32>,
                  Pointer<Float>,
                  Uint32,
                  Pointer<Int32>,
                  Pointer<Uint32>,
                )
              >
            >()
            .asFunction<
              int Function(
                Pointer<Void>,
                Pointer<Int32>,
                Pointer<Float>,
                int,
                Pointer<Int32>,
                Pointer<Uint32>,
              )
            >();
        final attributes = arena<Int32>(13)
          ..asTypedList(13).setAll(0, [
            0x202D,
            1,
            0x2010,
            1,
            0x2013,
            0x202B,
            0x2014,
            32,
            0x2022,
            24,
            0x2023,
            8,
            0,
          ]);
        final pixelFormat = arena<Int32>();
        final count = arena<Uint32>();
        _require(
          choose(dc, attributes, nullptr, 1, pixelFormat, count) != 0 &&
              count.value > 0,
          'wglChoosePixelFormatARB',
        );
        final createPbuffer = _extension('wglCreatePbufferARB', arena)
            .cast<
              NativeFunction<
                Pointer<Void> Function(
                  Pointer<Void>,
                  Int32,
                  Int32,
                  Int32,
                  Pointer<Int32>,
                )
              >
            >()
            .asFunction<
              Pointer<Void> Function(
                Pointer<Void>,
                int,
                int,
                int,
                Pointer<Int32>,
              )
            >();
        _pbuffer = createPbuffer(dc, pixelFormat.value, 1, 1, arena<Int32>());
        _require(_pbuffer != nullptr, 'wglCreatePbufferARB');
        _dc = _extension(
          'wglGetPbufferDCARB',
          arena,
        ).cast<NativeFunction<_Pointer1>>().asFunction<_Pointer1>()(_pbuffer);
        _require(_dc != nullptr, 'wglGetPbufferDCARB');
        _context = _createContext(_dc);
        _require(_context != nullptr, 'wglCreateContext(pbuffer)');
      } catch (_) {
        close();
        rethrow;
      } finally {
        _makeCurrent(previousDc, previousContext);
        if (bootstrap != nullptr) _deleteContext(bootstrap);
        if (dc != nullptr) {
          _user.lookupFunction<_Bool2Native, _Bool2>('ReleaseDC')(window, dc);
        }
        if (window != nullptr) {
          _user.lookupFunction<_Bool1Native, _Bool1>('DestroyWindow')(window);
        }
        _user.lookupFunction<
          Int32 Function(Pointer<Utf16>, Pointer<Void>),
          int Function(Pointer<Utf16>, Pointer<Void>)
        >('UnregisterClassW')(className, instance);
      }
    });
  }

  final _user = DynamicLibrary.open('user32.dll');
  final _gdi = DynamicLibrary.open('gdi32.dll');
  final _gl = DynamicLibrary.open('opengl32.dll');
  final _kernel = DynamicLibrary.open('kernel32.dll');
  Pointer<Void> _pbuffer = nullptr, _dc = nullptr, _context = nullptr;
  late _Bool2 _releaseDc;
  late _Bool1 _destroyPbuffer;
  late final _createContext = _gl.lookupFunction<_Pointer1, _Pointer1>(
    'wglCreateContext',
  );
  late final _deleteContext = _gl.lookupFunction<_Bool1Native, _Bool1>(
    'wglDeleteContext',
  );
  late final _makeCurrent = _gl.lookupFunction<_Bool2Native, _Bool2>(
    'wglMakeCurrent',
  );

  WglContextDescriptor descriptor({required bool shared}) =>
      WglContextDescriptor(
        deviceContext: NativePointer(_dc.address),
        shareContext: NativePointer(shared ? _context.address : 0),
        getProcAddress: NativePointer(
          _gl
              .lookup<NativeFunction<Pointer<Void> Function(Pointer<Utf8>)>>(
                'wglGetProcAddress',
              )
              .address,
        ),
      );

  Pointer<Void> _extension(String name, Arena arena) {
    final result = _gl
        .lookupFunction<
          Pointer<Void> Function(Pointer<Utf8>),
          Pointer<Void> Function(Pointer<Utf8>)
        >('wglGetProcAddress')(name.toNativeUtf8(allocator: arena));
    if (result.address <= 3 || result.address == -1) {
      throw StateError('WGL extension $name is unavailable');
    }
    return result;
  }

  void _require(bool success, String operation) {
    if (!success) {
      final code = _kernel.lookupFunction<Uint32 Function(), int Function()>(
        'GetLastError',
      )();
      throw StateError('$operation failed: Windows error $code');
    }
  }

  void close() {
    if (_context != nullptr) {
      _require(_deleteContext(_context) != 0, 'wglDeleteContext');
      _context = nullptr;
    }
    if (_dc != nullptr) {
      _require(_releaseDc(_pbuffer, _dc) != 0, 'wglReleasePbufferDCARB');
      _dc = nullptr;
    }
    if (_pbuffer != nullptr) {
      _require(_destroyPbuffer(_pbuffer) != 0, 'wglDestroyPbufferARB');
      _pbuffer = nullptr;
    }
  }
}

final class _WindowClass extends Struct {
  @Uint32()
  external int style;
  external Pointer<
    NativeFunction<IntPtr Function(Pointer<Void>, Uint32, UintPtr, IntPtr)>
  >
  procedure;
  @Int32()
  external int classExtra;
  @Int32()
  external int windowExtra;
  external Pointer<Void> instance;
  external Pointer<Void> icon;
  external Pointer<Void> cursor;
  external Pointer<Void> background;
  external Pointer<Utf16> menuName;
  external Pointer<Utf16> className;
}

final class _PixelFormat extends Struct {
  @Uint16()
  external int size;
  @Uint16()
  external int version;
  @Uint32()
  external int flags;
  @Uint8()
  external int pixelType;
  @Uint8()
  external int colorBits;
  @Uint8()
  external int redBits;
  @Uint8()
  external int redShift;
  @Uint8()
  external int greenBits;
  @Uint8()
  external int greenShift;
  @Uint8()
  external int blueBits;
  @Uint8()
  external int blueShift;
  @Uint8()
  external int alphaBits;
  @Uint8()
  external int alphaShift;
  @Uint8()
  external int accumBits;
  @Uint8()
  external int accumRedBits;
  @Uint8()
  external int accumGreenBits;
  @Uint8()
  external int accumBlueBits;
  @Uint8()
  external int accumAlphaBits;
  @Uint8()
  external int depthBits;
  @Uint8()
  external int stencilBits;
  @Uint8()
  external int auxBuffers;
  @Uint8()
  external int layerType;
  @Uint8()
  external int reserved;
  @Uint32()
  external int layerMask;
  @Uint32()
  external int visibleMask;
  @Uint32()
  external int damageMask;
}
