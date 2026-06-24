//===-- asan_win.cpp
//------------------------------------------------------===//>
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// Windows-specific details.
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"
#if SANITIZER_CYGWIN
#  define WIN32_LEAN_AND_MEAN
#  include <memoryapi.h>
#  include <pthread.h>
#  include <stdlib.h>
#  include <sys/cygwin.h>
#  include <windows.h>

#  include "asan_interceptors.h"
#  include "asan_internal.h"
#  include "asan_mapping.h"
#  include "asan_poisoning.h"
#  include "asan_report.h"
#  include "asan_stack.h"
#  include "asan_thread.h"
#  include "interception/interception.h"
#  include "lsan/lsan_common.h"
#  include "sanitizer_common/sanitizer_libc.h"
#  include "sanitizer_common/sanitizer_mutex.h"
#  include "sanitizer_common/sanitizer_win.h"
#  include "sanitizer_common/sanitizer_win_defs.h"

using namespace __asan;

extern "C" {
SANITIZER_INTERFACE_ATTRIBUTE
int __asan_should_detect_stack_use_after_return() {
  __asan_init();
  return __asan_option_detect_stack_use_after_return;
}

SANITIZER_INTERFACE_ATTRIBUTE
uptr __asan_get_shadow_memory_dynamic_address() {
  __asan_init();
  return __asan_shadow_memory_dynamic_address;
}
}  // extern "C"

// ---------------------- Windows-specific interceptors ---------------- {{{
#if 0
static LPTOP_LEVEL_EXCEPTION_FILTER default_seh_handler;
static LPTOP_LEVEL_EXCEPTION_FILTER user_seh_handler;

extern "C" SANITIZER_INTERFACE_ATTRIBUTE long __asan_unhandled_exception_filter(
    EXCEPTION_POINTERS* info) {
  EXCEPTION_RECORD* exception_record = info->ExceptionRecord;
  CONTEXT* context = info->ContextRecord;

  // FIXME: Handle EXCEPTION_STACK_OVERFLOW here.

  SignalContext sig(exception_record, context);
  ReportDeadlySignal(sig);
  UNREACHABLE("returned from reporting deadly signal");
}

// Wrapper SEH Handler. If the exception should be handled by asan, we call
// __asan_unhandled_exception_filter, otherwise, we execute the user provided
// exception handler or the default.
static LONG WINAPI SEHHandler(EXCEPTION_POINTERS* info) {
  DWORD exception_code = info->ExceptionRecord->ExceptionCode;
  if (__sanitizer::IsHandledDeadlyException(exception_code))
    return __asan_unhandled_exception_filter(info);
  if (user_seh_handler)
    return user_seh_handler(info);
  // Bubble out to the default exception filter.
  if (default_seh_handler)
    return default_seh_handler(info);
  return EXCEPTION_CONTINUE_SEARCH;
}

INTERCEPTOR_WINAPI(LPTOP_LEVEL_EXCEPTION_FILTER, SetUnhandledExceptionFilter,
                   LPTOP_LEVEL_EXCEPTION_FILTER ExceptionFilter) {
  CHECK(REAL(SetUnhandledExceptionFilter));
  if (ExceptionFilter == &SEHHandler)
    return REAL(SetUnhandledExceptionFilter)(ExceptionFilter);
  // We record the user provided exception handler to be called for all the
  // exceptions unhandled by asan.
  Swap(ExceptionFilter, user_seh_handler);
  return ExceptionFilter;
}

INTERCEPTOR_WINAPI(void, RtlRaiseException, EXCEPTION_RECORD* ExceptionRecord) {
  CHECK(REAL(RtlRaiseException));
  // This is a noreturn function, unless it's one of the exceptions raised to
  // communicate with the debugger, such as the one from OutputDebugString.
  if (ExceptionRecord->ExceptionCode != DBG_PRINTEXCEPTION_C)
    __asan_handle_no_return();
  REAL(RtlRaiseException)(ExceptionRecord);
}

INTERCEPTOR_WINAPI(void, RaiseException, void* a, void* b, void* c, void* d) {
  CHECK(REAL(RaiseException));
  __asan_handle_no_return();
  REAL(RaiseException)(a, b, c, d);
}

#  if SANITIZER_WINDOWS64 || SANITIZER_CYGWIN64

INTERCEPTOR_WINAPI(EXCEPTION_DISPOSITION, __C_specific_handler,
                   _EXCEPTION_RECORD* a, void* b, _CONTEXT* c,
                   _DISPATCHER_CONTEXT* d) {
  CHECK(REAL(__C_specific_handler));
  __asan_handle_no_return();
  return REAL(__C_specific_handler)(a, b, c, d);
}

#  else

INTERCEPTOR(int, _except_handler3, void* a, void* b, void* c, void* d) {
  CHECK(REAL(_except_handler3));
  __asan_handle_no_return();
  return REAL(_except_handler3)(a, b, c, d);
}

#    if ASAN_DYNAMIC
// This handler is named differently in -MT and -MD CRTs.
#      define _except_handler4 _except_handler4_common
#    endif
INTERCEPTOR(int, _except_handler4, void* a, void* b, void* c, void* d) {
  CHECK(REAL(_except_handler4));
  __asan_handle_no_return();
  return REAL(_except_handler4)(a, b, c, d);
}
#  endif

#  if !SANITIZER_CYGWIN
struct ThreadStartParams {
  thread_callback_t start_routine;
  void* arg;
};

static thread_return_t THREAD_CALLING_CONV asan_thread_start(void* arg) {
  AsanThread* t = (AsanThread*)arg;
  SetCurrentThread(t);
  t->ThreadStart(GetTid());

  ThreadStartParams params;
  t->GetStartData(params);

  auto res = (*params.start_routine)(params.arg);
  return res;
}

INTERCEPTOR_WINAPI(HANDLE, CreateThread, LPSECURITY_ATTRIBUTES security,
                   SIZE_T stack_size, LPTHREAD_START_ROUTINE start_routine,
                   void* arg, DWORD thr_flags, DWORD* tid) {
  // Strict init-order checking is thread-hostile.
  if (flags()->strict_init_order)
    StopInitOrderChecking();
  GET_STACK_TRACE_THREAD;
  // FIXME: The CreateThread interceptor is not the same as a pthread_create
  // one.  This is a bandaid fix for PR22025.
  bool detached = false;  // FIXME: how can we determine it on Windows?
  u32 current_tid = GetCurrentTidOrInvalid();
  ThreadStartParams params = {start_routine, arg};
  AsanThread* t = AsanThread::Create(params, current_tid, &stack, detached);
  return REAL(CreateThread)(security, stack_size, asan_thread_start, t,
                            thr_flags, tid);
}

INTERCEPTOR_WINAPI(void, ExitThread, DWORD dwExitCode) {
  AsanThread* t = (AsanThread*)__asan::GetCurrentThread();
  if (t)
    t->Destroy();
  REAL(ExitThread)(dwExitCode);
}
#  endif
#endif
// }}}

namespace __asan {

void InitializePlatformInterceptors() {
  __interception::SetErrorReportCallback(Report);
#if 0

  // The interceptors were not designed to be removable, so we have to keep this
  // module alive for the life of the process.
  HMODULE pinned;
  CHECK(GetModuleHandleExW(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
      (LPCWSTR)&InitializePlatformInterceptors, &pinned));

#  if !SANITIZER_CYGWIN
  ASAN_INTERCEPT_FUNC(CreateThread);
  ASAN_INTERCEPT_FUNC(ExitThread);
#  endif
  ASAN_INTERCEPT_FUNC(SetUnhandledExceptionFilter);

#  if SANITIZER_WINDOWS64 || SANITIZER_CYGWIN64
  ASAN_INTERCEPT_FUNC(__C_specific_handler);
#  else
  ASAN_INTERCEPT_FUNC(_except_handler3);
  ASAN_INTERCEPT_FUNC(_except_handler4);
#  endif

  // Try to intercept kernel32!RaiseException, and if that fails, intercept
  // ntdll!RtlRaiseException instead.
  if (!::__interception::OverrideFunction("RaiseException",
                                          (uptr)WRAP(RaiseException),
                                          (uptr*)&REAL(RaiseException))) {
#  if !SANITIZER_CYGWIN
    CHECK(::__interception::OverrideFunction("RtlRaiseException",
                                             (uptr)WRAP(RtlRaiseException),
                                             (uptr*)&REAL(RtlRaiseException)));
#  endif
  }
#endif
}

void AsanApplyToGlobals(globals_op_fptr op, const void* needle) {
  UNIMPLEMENTED();
}

void FlushUnneededASanShadowMemory(uptr p, uptr size) {
  // Only asan on 64-bit Windows supports committing shadow memory on demand.
#  if SANITIZER_WINDOWS64 || SANITIZER_CYGWIN64
  // Since asan's mapping is compacting, the shadow chunk may be
  // not page-aligned, so we only flush the page-aligned portion.
  ReleaseMemoryPagesToOS(MemToShadow(p), MemToShadow(p + size));
#  endif
}

// ---------------------- Various stuff ---------------- {{{
uptr FindDynamicShadowStart() {
  return MapDynamicShadow(MemToShadowSize(kHighMemEnd), ASAN_SHADOW_SCALE,
                          /*min_shadow_base_alignment*/ 0, kHighMemEnd,
                          GetMmapGranularity());
}

// Not used
void TryReExecWithoutASLR() {}

void AsanCheckDynamicRTPrereqs() {}

void AsanCheckIncompatibleRT() {}

// Exception handler for dealing with shadow memory.
static LONG CALLBACK
ShadowExceptionHandler(PEXCEPTION_POINTERS exception_pointers) {
  uptr page_size = GetPageSizeCached();
  // Only handle access violations.
  if (exception_pointers->ExceptionRecord->ExceptionCode !=
          EXCEPTION_ACCESS_VIOLATION ||
      exception_pointers->ExceptionRecord->NumberParameters < 2) {
    //__asan_handle_no_return();
    return EXCEPTION_CONTINUE_SEARCH;
  }

  // Only handle access violations that land within the shadow memory.
  uptr addr =
      (uptr)(exception_pointers->ExceptionRecord->ExceptionInformation[1]);

  // Check valid shadow range.
  if (!AddrIsInShadow(addr)) {
    //__asan_handle_no_return();
    return EXCEPTION_CONTINUE_SEARCH;
  }

  // This is an access violation while trying to read from the shadow. Commit
  // the relevant page and let execution continue.

  // Determine the address of the page that is being accessed.
  uptr page = RoundDownTo(addr, page_size);

  // Commit the page.
  uptr result =
      (uptr)::VirtualAlloc((LPVOID)page, page_size, MEM_COMMIT, PAGE_READWRITE);
  if (result != page)
    return EXCEPTION_CONTINUE_SEARCH;

  // The page mapping succeeded, so continue execution as usual.
  return EXCEPTION_CONTINUE_EXECUTION;
}

void InitializePlatformExceptionHandlers() {
  CHECK(AddVectoredExceptionHandler(TRUE, &ShadowExceptionHandler));
}

void CommitShadowMemoryPage(uptr shadow_first, uptr shadow_last) {
  uptr page_size = GetPageSizeCached();
  uptr page_low = RoundDownTo(shadow_first, page_size);
  uptr length = RoundUpTo(shadow_last - page_low + 1, page_size);
  if (!VirtualAlloc((void*)page_low, length, MEM_COMMIT, PAGE_READWRITE))
    VirtualAlloc((void*)page_low, length, MEM_RESERVE | MEM_COMMIT,
                 PAGE_READWRITE);
}

bool IsSystemHeapAddress(uptr addr) {
  return ::HeapValidate(GetProcessHeap(), 0, (void*)addr) != FALSE;
}

// We want to install our own exception handler (EH) to print helpful reports
// on access violations and whatnot.  Unfortunately, the CRT initializers assume
// they are run before any user code and drop any previously-installed EHs on
// the floor, so we can't install our handler inside __asan_init.
// (See crt0dat.c in the CRT sources for the details)
//
// Things get even more complicated with the dynamic runtime, as it finishes its
// initialization before the .exe module CRT begins to initialize.
//
// For the static runtime (-MT), it's enough to put a callback to
// __asan_set_seh_filter in the last section for C initializers.
//
// For the dynamic runtime (-MD), we want link the same
// asan_dynamic_runtime_thunk.lib to all the modules, thus __asan_set_seh_filter
// will be called for each instrumented module.  This ensures that at least one
// __asan_set_seh_filter call happens after the .exe module CRT is initialized.
extern "C" SANITIZER_INTERFACE_ATTRIBUTE int __asan_set_seh_filter() {
#if 0
  // We should only store the previous handler if it's not our own handler in
  // order to avoid loops in the EH chain.
  auto prev_seh_handler = SetUnhandledExceptionFilter(SEHHandler);
  if (prev_seh_handler != &SEHHandler)
    default_seh_handler = prev_seh_handler;
#endif
  return 0;
}

bool HandleDlopenInit() {
  // Not supported on this platform.
  static_assert(!SANITIZER_SUPPORTS_INIT_FOR_DLOPEN,
                "Expected SANITIZER_SUPPORTS_INIT_FOR_DLOPEN to be false");
  return false;
}

// constexpr unsigned TLS_OUT_OF_INDEXES = 0xFFFFFFFF;
// extern "C" __declspec(stdcall) unsigned TlsAlloc();
// extern "C" __declspec(stdcall) unsigned TlsFree(unsigned);
// extern "C" __declspec(stdcall) void *TlsGetValue(unsigned);
// extern "C" __declspec(stdcall) void TlsSetValue(unsigned, void*);
static unsigned tsd_key = TLS_OUT_OF_INDEXES;
void AsanTSDInit(void (*)(void*)) {
  CHECK_EQ(TLS_OUT_OF_INDEXES, tsd_key);
  tsd_key = TlsAlloc();
  CHECK_NE(TLS_OUT_OF_INDEXES, tsd_key);
}
void* AsanTSDGet() {
  CHECK_NE(TLS_OUT_OF_INDEXES, tsd_key);
  return TlsGetValue(tsd_key);
}
void AsanTSDSet(void* tsd) {
  CHECK_NE(TLS_OUT_OF_INDEXES, tsd_key);
  TlsSetValue(tsd_key, tsd);
}
void PlatformTSDDtor(void* tsd) {
  CHECK_NE(TLS_OUT_OF_INDEXES, tsd_key);
  AsanThread::TSDDtor(tsd);
  CHECK(TlsFree(tsd_key));
}

void InstallAtForkHandler() {
  static HANDLE ph, ev1, ev2;
  static void* tsd_value;
  static constexpr auto reinit = []() {
    if (ph)
      CloseHandle(ph);
    ph = nullptr;
    DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),
                    GetCurrentProcess(), &ph,
                    PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, TRUE, 0);
    ev1 = CreateEvent(nullptr, FALSE, TRUE, nullptr);
  };
  reinit();

  pthread_atfork(
      []() {
        WaitForSingleObject(ev1, INFINITE);
#  if 0
        __lsan::LockThreads();
        __lsan::LockAllocator();

        AcquirePoisonRecords();

        StackDepotLockBeforeFork();
#  endif

        SECURITY_ATTRIBUTES attr{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
        ev2 = CreateEvent(&attr, FALSE, FALSE, nullptr);
        tsd_value = TlsGetValue(tsd_key);
      },
      []() {
        WaitForSingleObject(ev2, INFINITE);
        CloseHandle(ev2);
#  if 0
        StackDepotUnlockAfterFork(false);

        ReleasePoisonRecords();

        // `_lsan` functions defined regardless of `CAN_SANITIZE_LEAKS` and
        // unlock the stuff we need.
        __lsan::UnlockAllocator();
        __lsan::UnlockThreads();
#  endif
        SetEvent(ev1);
      },
      []() {
        __asan_set_seh_filter();
        InitializePlatformExceptionHandlers();
        CopyShadowsAtForkHandler(ph);
        tsd_key = TlsAlloc();
        TlsSetValue(tsd_key, tsd_value);
        reinit();
        SetEvent(ev2);
        CloseHandle(ev2);
#  if 0
        StackDepotUnlockAfterFork(true);

        ReleasePoisonRecords();

        // `_lsan` functions defined regardless of `CAN_SANITIZE_LEAKS` and
        // unlock the stuff we need.
        __lsan::UnlockAllocator();
        __lsan::UnlockThreads();
#  endif
      });
}

SANITIZER_SECTION(".CRT$XCAB") int (*__intercept_seh)() = __asan_set_seh_filter;

// Piggyback on the TLS initialization callback directory to initialize asan as
// early as possible. Initializers in .CRT$XL* are called directly by ntdll,
// which run before the CRT. Users also add code to .CRT$XLC, so it's important
// to run our initializers first.
static void NTAPI asan_thread_init(void* module, DWORD reason, void* reserved) {
  if (reason == DLL_PROCESS_ATTACH)
    __asan_init();
}

SANITIZER_SECTION(".CRT$XLAB")
void(NTAPI* __asan_tls_init)(void*, unsigned, void*) = asan_thread_init;

static void NTAPI asan_thread_exit(void* module, DWORD reason, void* reserved) {
  if (reason == DLL_THREAD_DETACH) {
    // Unpoison the thread's stack because the memory may be re-used.
    // NT_TIB *tib = (NT_TIB *)NtCurrentTeb();
    // uptr stackSize = (uptr)tib->StackBase - (uptr)tib->StackLimit;
    //__asan_unpoison_memory_region(tib->StackLimit, stackSize);
  }
}

SANITIZER_SECTION(".CRT$XLY")
void(NTAPI* __asan_tls_exit)(void*, unsigned, void*) = asan_thread_exit;

WIN_FORCE_LINK(__asan_dso_reg_hook)

// }}}
}  // namespace __asan

#endif  // SANITIZER_WINDOWS
