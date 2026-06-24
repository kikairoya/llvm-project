//===-- sanitizer_win_thunk_interception.cpp -----------------------  -----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines things that need to be present in the application modules
// to interact with sanitizer DLL correctly and cannot be implemented using the
// default "import library" generated when linking the DLL.
//
// This includes the common infrastructure required to intercept local functions
// that must be replaced with sanitizer-aware versions, as well as the
// registration of weak functions with the sanitizer DLL. With this in-place,
// other sanitizer components can simply write to the .INTR and .WEAK sections.
//
//===----------------------------------------------------------------------===//

#if defined(SANITIZER_STATIC_RUNTIME_THUNK) || \
    defined(SANITIZER_DYNAMIC_RUNTIME_THUNK)
#  include "sanitizer_win_thunk_interception.h"

extern "C" void abort();

namespace __sanitizer {

int override_function(const char *export_name, const uptr user_function) {
  if (!__sanitizer_override_function(export_name, user_function)) {
    abort();
  }

  return 0;
}

int register_weak(const char *export_name, const uptr user_function) {
  if (!__sanitizer_register_weak_function(export_name, user_function)) {
    abort();
  }

  return 0;
}

void initialize_thunks(const sanitizer_thunk *first,
                       const sanitizer_thunk *last) {
  for (const sanitizer_thunk *it = first; it < last; ++it) {
    if (*it) {
      (*it)();
    }
  }
}
}  // namespace __sanitizer

#  define INTERFACE_FUNCTION(Name)
#  define INTERFACE_WEAK_FUNCTION(Name) REGISTER_WEAK_FUNCTION(Name)
#  include "sanitizer_common_interface.inc"

#  if !SANITIZER_CYGWIN
#    pragma section(".INTR$A", read)  // intercept begin
#    pragma section(".INTR$Z", read)  // intercept end
#    pragma section(".WEAK$A", read)  // weak begin
#    pragma section(".WEAK$Z", read)  // weak end
#  endif

extern "C" {
SANITIZER_SECTION(".INTR$A") sanitizer_thunk __sanitizer_intercept_thunk_begin;
SANITIZER_SECTION(".INTR$Z") sanitizer_thunk __sanitizer_intercept_thunk_end;

SANITIZER_SECTION(".WEAK$A")
sanitizer_thunk __sanitizer_register_weak_thunk_begin;
SANITIZER_SECTION(".WEAK$Z")
sanitizer_thunk __sanitizer_register_weak_thunk_end;
}

extern "C" int __sanitizer_thunk_init() {
  // __sanitizer_static_thunk_init is expected to be called by only one thread.
  static bool flag = false;
  if (flag) {
    return 0;
  }
  flag = true;

  __sanitizer::initialize_thunks(&__sanitizer_intercept_thunk_begin,
                                 &__sanitizer_intercept_thunk_end);
  __sanitizer::initialize_thunks(&__sanitizer_register_weak_thunk_begin,
                                 &__sanitizer_register_weak_thunk_end);

  // In DLLs, the callbacks are expected to return 0,
  // otherwise CRT initialization fails.
  return 0;
}

// We want to call dll_thunk_init before C/C++ initializers / constructors are
// executed, otherwise functions like memset might be invoked.
#  if !SANITIZER_CYGWIN
#    pragma section(".CRT$XIB", long, read)
#  endif
SANITIZER_SECTION(".CRT$XIB")
int (*__sanitizer_thunk_init_ptr)() = __sanitizer_thunk_init;

static void WINAPI sanitizer_thunk_thread_init(void *mod, unsigned long reason,
                                               void *reserved) {
  if (reason == /*DLL_PROCESS_ATTACH=*/1)
    __sanitizer_thunk_init();
}

#  if !SANITIZER_CYGWIN
#    pragma section(".CRT$XLAB", long, read)
#  endif
SANITIZER_SECTION(".CRT$XLAB")
void(WINAPI* __sanitizer_thunk_thread_init_ptr)(void*, unsigned long, void*) =
    sanitizer_thunk_thread_init;

#  if SANITIZER_CYGWIN
extern "C"
    __attribute__((section(".CRT$XIA"))) void (*__xi_a[])(void*, unsigned,
                                                          void*) = {NULL};
extern "C"
    __attribute__((section(".CRT$XIZ"))) void (*__xi_z[])(void*, unsigned,
                                                          void*) = {NULL};
extern "C"
    __attribute__((section(".CRT$XLA"))) void (*__xl_a[])(void*, unsigned,
                                                          void*) = {NULL};
extern "C"
    __attribute__((section(".CRT$XLZ"))) void (*__xl_z[])(void*, unsigned,
                                                          void*) = {NULL};
extern "C" __attribute__((section(".CRT$XCA"))) void (*__xc_a[])() = {NULL};
extern "C" __attribute__((section(".CRT$XCZ"))) void (*__xc_z[])() = {NULL};
extern "C" __attribute__((section(".CRT$XPA"))) void (*__xp_a[])() = {NULL};
extern "C" __attribute__((section(".CRT$XPZ"))) void (*__xp_z[])() = {NULL};
extern "C" __attribute__((section(".CRT$XTA"))) void (*__xt_a[])() = {NULL};
extern "C" __attribute__((section(".CRT$XTZ"))) void (*__xt_z[])() = {NULL};
extern "C" void __asan_init();
__attribute__((visibility("default"), used)) extern "C" void cygwin_premain3() {
  __asan_init();
  for (auto** p = __xi_a; p < __xi_z; ++p)
    if (*p)
      (**p)(nullptr, 1, nullptr);
  for (auto** p = __xl_a; p < __xl_z; ++p)
    if (*p)
      (**p)(nullptr, 1, nullptr);
  for (auto** p = __xc_a; p < __xc_z; ++p)
    if (*p)
      (**p)();
}
extern "C" int __real_DllMain(void* h, unsigned reason, void* ptr);
__attribute__((visibility("default"))) extern "C" int __wrap_DllMain(
    void* h, unsigned reason, void* ptr) {
  for (auto** p = __xi_a; p < __xi_z; ++p)
    if (*p)
      (**p)(h, reason, ptr);
  for (auto** p = __xl_a; p < __xl_z; ++p)
    if (*p)
      (**p)(h, reason, ptr);
  switch (reason) {
    case 0:  // DLL_PROCESS_DETACH
      for (auto** p = __xp_a; p < __xp_z; ++p)
        if (*p)
          (**p)();
      for (auto** p = __xt_a; p < __xt_z; ++p)
        if (*p)
          (**p)();
      break;
    case 1:  // DLL_PROCESS_ATTACH
      for (auto** p = __xc_a; p < __xc_z; ++p)
        if (*p)
          (**p)();
      break;
    case 2:  // DLL_THREAD_ATTACH
    case 3:  // DLL_THREAD_DETACH
      break;
  }
  return __real_DllMain(h, reason, ptr);
}
#  endif

#endif  // defined(SANITIZER_STATIC_RUNTIME_THUNK) ||
        // defined(SANITIZER_DYNAMIC_RUNTIME_THUNK)
