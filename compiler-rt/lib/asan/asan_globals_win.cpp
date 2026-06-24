//===-- asan_globals_win.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Global registration code that is linked into every Windows DLL and EXE.
//
//===----------------------------------------------------------------------===//

#include "asan_interface_internal.h"
#if SANITIZER_WINDOWS || SANITIZER_CYGWIN

namespace __asan {

#  if SANITIZER_CYGWIN
asm(".section        .drectve,\"yni\"");
asm(".ascii  \" /merge:.ASAN=.data\"");
#  else
#    pragma section(".ASAN$GA", read, write)
#    pragma section(".ASAN$GZ", read, write)
#    pragma comment(linker, "/merge:.ASAN=.data")
#  endif

extern "C" alignas(sizeof(__asan_global))
    SANITIZER_SECTION(".ASAN$GA") __asan_global __asan_globals_start = {};
extern "C" alignas(sizeof(__asan_global))
    SANITIZER_SECTION(".ASAN$GZ") __asan_global __asan_globals_end = {};

static void call_on_globals(void (*hook)(__asan_global *, uptr)) {
  __asan_global *start = &__asan_globals_start + 1;
  __asan_global *end = &__asan_globals_end;
  uptr bytediff = (uptr)end - (uptr)start;
  if (bytediff % sizeof(__asan_global) != 0) {
#  if !SANITIZER_CYGWIN && (defined(SANITIZER_DLL_THUNK) ||             \
                            defined(SANITIZER_DYNAMIC_RUNTIME_THUNK) || \
                            defined(SANITIZER_STATIC_RUNTIME_THUNK))
    __debugbreak();
#else
    CHECK("corrupt asan global array");
#endif
  }
  // We know end >= start because the linker sorts the portion after the dollar
  // sign alphabetically.
  uptr n = end - start;
  hook(start, n);
}

static void register_dso_globals() {
  call_on_globals(&__asan_register_globals);
}

static void unregister_dso_globals() {
  call_on_globals(&__asan_unregister_globals);
}

// Register globals
#  if !SANITIZER_CYGWIN
#    pragma section(".CRT$XCU", long, read)
#    pragma section(".CRT$XTX", long, read)
#  endif
extern "C" SANITIZER_SECTION(".CRT$XCU") void (*const __asan_dso_reg_hook)() =
    &register_dso_globals;
extern "C" SANITIZER_SECTION(".CRT$XTX") void (*const __asan_dso_unreg_hook)() =
    &unregister_dso_globals;

} // namespace __asan

#endif  // SANITIZER_WINDOWS
