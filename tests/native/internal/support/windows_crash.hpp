#pragma once

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <cstdio>

#include <windows.h>
// dbghelp.h needs the types that windows.h declares.
#include <dbghelp.h>

namespace mln::native_tests {

inline LONG CALLBACK report_access_violation(EXCEPTION_POINTERS* exception) {
  if (exception->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
    return EXCEPTION_CONTINUE_SEARCH;
  static LONG reporting = 0;
  if (InterlockedExchange(&reporting, 1)) return EXCEPTION_CONTINUE_SEARCH;
  const auto& record = *exception->ExceptionRecord;
  std::fprintf(
    stderr, "AV: instruction=%p thread=%lu", record.ExceptionAddress,
    GetCurrentThreadId()
  );
  if (record.NumberParameters >= 2)
    std::fprintf(
      stderr, " access=%llu address=%p",
      static_cast<unsigned long long>(record.ExceptionInformation[0]),
      reinterpret_cast<void*>(record.ExceptionInformation[1])
    );
  std::fprintf(stderr, "\n");
  std::fflush(stderr);
  auto context = *exception->ContextRecord;
  auto frame = STACKFRAME64{};
#if defined(_M_ARM64) || defined(__aarch64__)
  const auto machine = DWORD{IMAGE_FILE_MACHINE_ARM64};
  frame.AddrPC.Offset = context.Pc;
  frame.AddrFrame.Offset = context.Fp;
  frame.AddrStack.Offset = context.Sp;
#elif defined(_M_X64) || defined(__x86_64__)
  const auto machine = DWORD{IMAGE_FILE_MACHINE_AMD64};
  frame.AddrPC.Offset = context.Rip;
  frame.AddrFrame.Offset = context.Rbp;
  frame.AddrStack.Offset = context.Rsp;
#else
#error Unsupported Windows diagnostic architecture
#endif
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Mode = AddrModeFlat;
  const auto process = GetCurrentProcess();
  for (unsigned index = 0; index != 64 && frame.AddrPC.Offset != 0; ++index) {
    alignas(SYMBOL_INFO) char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    auto displacement = DWORD64{0};
    std::fprintf(
      stderr, "#%u 0x%llx", index,
      static_cast<unsigned long long>(frame.AddrPC.Offset)
    );
    if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
      std::fprintf(
        stderr, " %s+0x%llx", symbol->Name,
        static_cast<unsigned long long>(displacement)
      );
    auto line = IMAGEHLP_LINE64{};
    line.SizeOfStruct = sizeof(line);
    auto line_offset = DWORD{0};
    if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &line_offset, &line))
      std::fprintf(stderr, " %s:%lu", line.FileName, line.LineNumber);
    std::fprintf(stderr, "\n");
    std::fflush(stderr);
    const auto previous = frame.AddrPC.Offset;
    if (
      !StackWalk64(
        machine, process, GetCurrentThread(), &frame, &context, nullptr,
        SymFunctionTableAccess64, SymGetModuleBase64, nullptr
      ) ||
      frame.AddrPC.Offset == previous
    )
      break;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

inline void install_access_violation_reporter() {
  SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
  if (!SymInitialize(GetCurrentProcess(), nullptr, TRUE))
    std::fprintf(stderr, "SymInitialize failed: %lu\n", GetLastError());
  if (AddVectoredExceptionHandler(1, report_access_violation) == nullptr)
    std::fprintf(
      stderr, "AddVectoredExceptionHandler failed: %lu\n", GetLastError()
    );
  std::fflush(stderr);
}

}  // namespace mln::native_tests
#endif
