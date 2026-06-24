//===-- asan_interceptors.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// Interceptors for operators new and delete.
//===----------------------------------------------------------------------===//

#include <stddef.h>

#include "asan_allocator.h"
#include "asan_internal.h"
#include "asan_report.h"
#include "asan_stack.h"
#include "interception/interception.h"

#if SANITIZER_CYGWIN
#  define WINVER
typedef void* HMODULE;
typedef unsigned int DWORD;
typedef void* HANDLE;
constexpr DWORD MAX_PATH = 260;
#  include <sys/cygwin.h>
#endif

// C++ operators can't have dllexport attributes on Windows. We export them
// anyway by passing extra -export flags to the linker, which is exactly that
// dllexport would normally do. We need to export them in order to make the
// VS2015 dynamic CRT (MD) work.
#if SANITIZER_WINDOWS && defined(_MSC_VER)
#  define CXX_OPERATOR_ATTRIBUTE
#  define COMMENT_EXPORT(sym) __pragma(comment(linker, "/export:" sym))
#  ifdef _WIN64
COMMENT_EXPORT("??2@YAPEAX_K@Z")                     // operator new
COMMENT_EXPORT("??2@YAPEAX_KAEBUnothrow_t@std@@@Z")  // operator new nothrow
COMMENT_EXPORT("??3@YAXPEAX@Z")                      // operator delete
COMMENT_EXPORT("??3@YAXPEAX_K@Z")                    // sized operator delete
COMMENT_EXPORT("??_U@YAPEAX_K@Z")                    // operator new[]
COMMENT_EXPORT("??_V@YAXPEAX@Z")                     // operator delete[]
#  else
COMMENT_EXPORT("??2@YAPAXI@Z")                    // operator new
COMMENT_EXPORT("??2@YAPAXIABUnothrow_t@std@@@Z")  // operator new nothrow
COMMENT_EXPORT("??3@YAXPAX@Z")                    // operator delete
COMMENT_EXPORT("??3@YAXPAXI@Z")                   // sized operator delete
COMMENT_EXPORT("??_U@YAPAXI@Z")                   // operator new[]
COMMENT_EXPORT("??_V@YAXPAX@Z")                   // operator delete[]
#  endif
#  undef COMMENT_EXPORT
#elif SANITIZER_CYGWIN
#  define CXX_OPERATOR_ATTRIBUTE INTERCEPTOR_ATTRIBUTE
#else
#  define CXX_OPERATOR_ATTRIBUTE INTERCEPTOR_ATTRIBUTE SANITIZER_WEAK_ATTRIBUTE
#endif

using namespace __asan;

// This code has issues on OSX.
// See https://github.com/google/sanitizers/issues/131.

// Fake std::nothrow_t and std::align_val_t to avoid including <new>.
namespace std {
using ::size_t;
struct nothrow_t {};
enum class align_val_t : size_t {};
}  // namespace std

// TODO(alekseyshl): throw std::bad_alloc instead of dying on OOM.
// For local pool allocation, align to SHADOW_GRANULARITY to match asan
// allocator behavior.
#define OPERATOR_NEW_BODY             \
  GET_STACK_TRACE_MALLOC;             \
  void* res = asan_new(size, &stack); \
  if (UNLIKELY(!res))                 \
    ReportOutOfMemory(size, &stack);  \
  return res
#define OPERATOR_NEW_BODY_NOTHROW \
  GET_STACK_TRACE_MALLOC;         \
  return asan_new(size, &stack)
#define OPERATOR_NEW_BODY_ARRAY             \
  GET_STACK_TRACE_MALLOC;                   \
  void* res = asan_new_array(size, &stack); \
  if (UNLIKELY(!res))                       \
    ReportOutOfMemory(size, &stack);        \
  return res
#define OPERATOR_NEW_BODY_ARRAY_NOTHROW \
  GET_STACK_TRACE_MALLOC;               \
  return asan_new_array(size, &stack)
#define OPERATOR_NEW_BODY_ALIGN                                         \
  GET_STACK_TRACE_MALLOC;                                               \
  void* res = asan_new_aligned(size, static_cast<uptr>(align), &stack); \
  if (UNLIKELY(!res))                                                   \
    ReportOutOfMemory(size, &stack);                                    \
  return res
#define OPERATOR_NEW_BODY_ALIGN_NOTHROW \
  GET_STACK_TRACE_MALLOC;               \
  return asan_new_aligned(size, static_cast<uptr>(align), &stack)
#define OPERATOR_NEW_BODY_ALIGN_ARRAY                                         \
  GET_STACK_TRACE_MALLOC;                                                     \
  void* res = asan_new_array_aligned(size, static_cast<uptr>(align), &stack); \
  if (UNLIKELY(!res))                                                         \
    ReportOutOfMemory(size, &stack);                                          \
  return res
#define OPERATOR_NEW_BODY_ALIGN_ARRAY_NOTHROW \
  GET_STACK_TRACE_MALLOC;                     \
  return asan_new_array_aligned(size, static_cast<uptr>(align), &stack)

// On OS X it's not enough to just provide our own 'operator new' and
// 'operator delete' implementations, because they're going to be in the
// runtime dylib, and the main executable will depend on both the runtime
// dylib and libstdc++, each of those'll have its implementation of new and
// delete.
// To make sure that C++ allocation/deallocation operators are overridden on
// OS X we need to intercept them using their mangled names.
#if !SANITIZER_APPLE
CXX_OPERATOR_ATTRIBUTE
void* operator new(size_t size) { OPERATOR_NEW_BODY; }
CXX_OPERATOR_ATTRIBUTE
void* operator new[](size_t size) { OPERATOR_NEW_BODY_ARRAY; }
CXX_OPERATOR_ATTRIBUTE
void* operator new(size_t size, std::nothrow_t const&) {
  OPERATOR_NEW_BODY_NOTHROW;
}
CXX_OPERATOR_ATTRIBUTE
void* operator new[](size_t size, std::nothrow_t const&) {
  OPERATOR_NEW_BODY_ARRAY_NOTHROW;
}
CXX_OPERATOR_ATTRIBUTE
void* operator new(size_t size, std::align_val_t align) {
  OPERATOR_NEW_BODY_ALIGN;
}
CXX_OPERATOR_ATTRIBUTE
void* operator new[](size_t size, std::align_val_t align) {
  OPERATOR_NEW_BODY_ALIGN_ARRAY;
}
CXX_OPERATOR_ATTRIBUTE
void* operator new(size_t size, std::align_val_t align, std::nothrow_t const&) {
  OPERATOR_NEW_BODY_ALIGN_NOTHROW;
}
CXX_OPERATOR_ATTRIBUTE
void* operator new[](size_t size, std::align_val_t align,
                     std::nothrow_t const&) {
  OPERATOR_NEW_BODY_ALIGN_ARRAY_NOTHROW;
}

#else  // SANITIZER_APPLE
INTERCEPTOR(void*, _Znwm, size_t size) { OPERATOR_NEW_BODY; }
INTERCEPTOR(void*, _Znam, size_t size) { OPERATOR_NEW_BODY_ARRAY; }
INTERCEPTOR(void*, _ZnwmRKSt9nothrow_t, size_t size, std::nothrow_t const&) {
  OPERATOR_NEW_BODY_NOTHROW;
}
INTERCEPTOR(void*, _ZnamRKSt9nothrow_t, size_t size, std::nothrow_t const&) {
  OPERATOR_NEW_BODY_ARRAY_NOTHROW;
}
#endif  // !SANITIZER_APPLE

#define OPERATOR_DELETE_BODY \
  GET_STACK_TRACE_FREE;      \
  asan_delete(ptr, &stack)
#define OPERATOR_DELETE_BODY_ARRAY \
  GET_STACK_TRACE_FREE;            \
  asan_delete_array(ptr, &stack)
#define OPERATOR_DELETE_BODY_ALIGN \
  GET_STACK_TRACE_FREE;            \
  asan_delete_aligned(ptr, static_cast<uptr>(align), &stack)
#define OPERATOR_DELETE_BODY_ALIGN_ARRAY \
  GET_STACK_TRACE_FREE;                  \
  asan_delete_array_aligned(ptr, static_cast<uptr>(align), &stack)
#define OPERATOR_DELETE_BODY_SIZE \
  GET_STACK_TRACE_FREE;           \
  asan_delete_sized(ptr, size, &stack)
#define OPERATOR_DELETE_BODY_SIZE_ARRAY \
  GET_STACK_TRACE_FREE;                 \
  asan_delete_array_sized(ptr, size, &stack)
#define OPERATOR_DELETE_BODY_SIZE_ALIGN \
  GET_STACK_TRACE_FREE;                 \
  asan_delete_sized_aligned(ptr, size, static_cast<uptr>(align), &stack)
#define OPERATOR_DELETE_BODY_SIZE_ALIGN_ARRAY \
  GET_STACK_TRACE_FREE;                       \
  asan_delete_array_sized_aligned(ptr, size, static_cast<uptr>(align), &stack)

#if !SANITIZER_APPLE
CXX_OPERATOR_ATTRIBUTE
void operator delete(void* ptr) NOEXCEPT { OPERATOR_DELETE_BODY; }
CXX_OPERATOR_ATTRIBUTE
void operator delete[](void* ptr) NOEXCEPT { OPERATOR_DELETE_BODY_ARRAY; }
CXX_OPERATOR_ATTRIBUTE
void operator delete(void* ptr, std::nothrow_t const&) { OPERATOR_DELETE_BODY; }
CXX_OPERATOR_ATTRIBUTE
void operator delete[](void* ptr, std::nothrow_t const&) {
  OPERATOR_DELETE_BODY_ARRAY;
}
CXX_OPERATOR_ATTRIBUTE
void operator delete(void* ptr, size_t size) NOEXCEPT {
  OPERATOR_DELETE_BODY_SIZE;
}
CXX_OPERATOR_ATTRIBUTE
void operator delete[](void* ptr, size_t size) NOEXCEPT {
  OPERATOR_DELETE_BODY_SIZE_ARRAY;
}
CXX_OPERATOR_ATTRIBUTE
void operator delete(void* ptr, std::align_val_t align) NOEXCEPT {
  OPERATOR_DELETE_BODY_ALIGN;
}
CXX_OPERATOR_ATTRIBUTE
void operator delete[](void* ptr, std::align_val_t align) NOEXCEPT {
  OPERATOR_DELETE_BODY_ALIGN_ARRAY;
}
CXX_OPERATOR_ATTRIBUTE
void operator delete(void* ptr, std::align_val_t align, std::nothrow_t const&) {
  OPERATOR_DELETE_BODY_ALIGN;
}
CXX_OPERATOR_ATTRIBUTE
void operator delete[](void* ptr, std::align_val_t align,
                       std::nothrow_t const&) {
  OPERATOR_DELETE_BODY_ALIGN_ARRAY;
}
CXX_OPERATOR_ATTRIBUTE
void operator delete(void* ptr, size_t size, std::align_val_t align) NOEXCEPT {
  OPERATOR_DELETE_BODY_SIZE_ALIGN;
}
CXX_OPERATOR_ATTRIBUTE
void operator delete[](void* ptr, size_t size,
                       std::align_val_t align) NOEXCEPT {
  OPERATOR_DELETE_BODY_SIZE_ALIGN_ARRAY;
}

#else   // SANITIZER_APPLE
INTERCEPTOR(void, _ZdlPv, void* ptr) { OPERATOR_DELETE_BODY; }
INTERCEPTOR(void, _ZdaPv, void* ptr) { OPERATOR_DELETE_BODY_ARRAY; }
INTERCEPTOR(void, _ZdlPvRKSt9nothrow_t, void* ptr, std::nothrow_t const&) {
  OPERATOR_DELETE_BODY;
}
INTERCEPTOR(void, _ZdaPvRKSt9nothrow_t, void* ptr, std::nothrow_t const&) {
  OPERATOR_DELETE_BODY_ARRAY;
}
#endif  // !SANITIZER_APPLE

#define ASAN_INTERCEPT_NEW_OP(mangled, overrider)                          \
  do {                                                                     \
    if (!::__interception::OverrideFunction("__wrap_" mangled,             \
                                            (uptr)overrider, nullptr))     \
      VReport(1, "AddressSanitizer: failed to intercept '%s'\n",           \
              "__wrap_" mangled);                                          \
    if (!::__interception::OverrideFunction(mangled, (uptr)overrider,      \
                                            nullptr))                      \
      VReport(1, "AddressSanitizer: failed to intercept '%s'\n", mangled); \
  } while (0)

#if SANITIZER_CYGWIN
__attribute__((weak)) extern "C" void* __real__Znwm(std::size_t);
__attribute__((weak)) extern "C" void* __real__Znam(std::size_t);
__attribute__((weak)) extern "C" void __real__ZdlPv(void*);
__attribute__((weak)) extern "C" void __real__ZdaPv(void*);
__attribute__((weak)) extern "C" void* __real__ZnwmRKSt9nothrow_t(
    std::size_t, const std::nothrow_t&);
__attribute__((weak)) extern "C" void* __real__ZnamRKSt9nothrow_t(
    std::size_t, const std::nothrow_t&);
__attribute__((weak)) extern "C" void __real__ZdlPvRKSt9nothrow_t(
    void*, const std::nothrow_t&);
__attribute__((weak)) extern "C" void __real__ZdaPvRKSt9nothrow_t(
    void*, const std::nothrow_t&);
__attribute__((weak)) extern "C" void __real__ZdlPvm(void*, std::size_t);
__attribute__((weak)) extern "C" void __real__ZdaPvm(void*, std::size_t);
__attribute__((weak)) extern "C" void* __real__ZnwmSt11align_val_t(
    std::size_t, std::align_val_t);
__attribute__((weak)) extern "C" void* __real__ZnamSt11align_val_t(
    std::size_t, std::align_val_t);
__attribute__((weak)) extern "C" void __real__ZdlPvSt11align_val_t(
    void*, std::align_val_t);
__attribute__((weak)) extern "C" void __real__ZdaPvSt11align_val_t(
    void*, std::align_val_t);
__attribute__((weak)) extern "C" void __real__ZdlPvmSt11align_val_t(
    void*, std::size_t, std::align_val_t);
__attribute__((weak)) extern "C" void __real__ZdaPvmSt11align_val_t(
    void*, std::size_t, std::align_val_t);
__attribute__((weak)) extern "C" void*
__real__ZnwmSt11align_val_tRKSt9nothrow_t(std::size_t, std::align_val_t,
                                          const std::nothrow_t&);
__attribute__((weak)) extern "C" void*
__real__ZnamSt11align_val_tRKSt9nothrow_t(std::size_t, std::align_val_t,
                                          const std::nothrow_t&);
__attribute__((weak)) extern "C" void
__real__ZdlPvSt11align_val_tRKSt9nothrow_t(void*, std::align_val_t,
                                           const std::nothrow_t&);
__attribute__((weak)) extern "C" void
__real__ZdaPvSt11align_val_tRKSt9nothrow_t(void*, std::align_val_t,
                                           const std::nothrow_t&);
static struct per_process_cxx_malloc {
  void* (*oper_new)(std::size_t);
  void* (*oper_new__)(std::size_t);
  void (*oper_delete)(void*);
  void (*oper_delete__)(void*);
  void* (*oper_new_nt)(std::size_t, const std::nothrow_t&);
  void* (*oper_new___nt)(std::size_t, const std::nothrow_t&);
  void (*oper_delete_nt)(void*, const std::nothrow_t&);
  void (*oper_delete___nt)(void*, const std::nothrow_t&);
  /* New in C++14: sized delete */
  void (*oper_delete_sz)(void*, std::size_t);
  void (*oper_delete___sz)(void*, std::size_t);
  /* New in C++17: aligned new/delete, and combinations with size and nothrow */
  void* (*oper_new_al)(std::size_t, std::align_val_t);
  void* (*oper_new___al)(std::size_t, std::align_val_t);
  void (*oper_delete_al)(void*, std::align_val_t);
  void (*oper_delete___al)(void*, std::align_val_t);
  void (*oper_delete_sz_al)(void*, std::size_t, std::align_val_t);
  void (*oper_delete___sz_al)(void*, std::size_t, std::align_val_t);
  void* (*oper_new_al_nt)(std::size_t, std::align_val_t, const std::nothrow_t&);
  void* (*oper_new___al_nt)(std::size_t, std::align_val_t,
                            const std::nothrow_t&);
  void (*oper_delete_al_nt)(void*, std::align_val_t, const std::nothrow_t&);
  void (*oper_delete___al_nt)(void*, std::align_val_t, const std::nothrow_t&);
} asan_cygwin_cxx_malloc = {&__real__Znwm,
                            &__real__Znam,
                            &__real__ZdlPv,
                            &__real__ZdaPv,
                            &__real__ZnwmRKSt9nothrow_t,
                            &__real__ZnamRKSt9nothrow_t,
                            &__real__ZdlPvRKSt9nothrow_t,
                            &__real__ZdaPvRKSt9nothrow_t,
                            &__real__ZdlPvm,
                            &__real__ZdaPvm,
                            &__real__ZnwmSt11align_val_t,
                            &__real__ZnamSt11align_val_t,
                            &__real__ZdlPvSt11align_val_t,
                            &__real__ZdaPvSt11align_val_t,
                            &__real__ZdlPvmSt11align_val_t,
                            &__real__ZdaPvmSt11align_val_t,
                            &__real__ZnwmSt11align_val_tRKSt9nothrow_t,
                            &__real__ZnamSt11align_val_tRKSt9nothrow_t,
                            &__real__ZdlPvSt11align_val_tRKSt9nothrow_t,
                            &__real__ZdaPvSt11align_val_tRKSt9nothrow_t};
void __asan::ReplaceSystemNewDelete() {
  auto* ud = (per_process*)cygwin_internal(CW_USER_DATA);
  ud->cxx_malloc = &asan_cygwin_cxx_malloc;
#  if 0
  ASAN_INTERCEPT_NEW_OP("_ZdaPv", (void (*)(void *))::operator delete[]);
  ASAN_INTERCEPT_NEW_OP("_ZdaPvRKSt9nothrow_t", (void (*)(void *, const std::nothrow_t &))::operator delete[]);
  ASAN_INTERCEPT_NEW_OP("_ZdaPvSt11align_val_t", (void (*)(void *, std::align_val_t))::operator delete[]);
  ASAN_INTERCEPT_NEW_OP("_ZdaPvSt11align_val_tRKSt9nothrow_t", (void (*)(void *, std::align_val_t, const std::nothrow_t&))::operator delete[]);
  ASAN_INTERCEPT_NEW_OP("_ZdaPvm", (void (*)(void *, size_t))::operator delete[]);
  ASAN_INTERCEPT_NEW_OP("_ZdaPvmSt11align_val_t", (void (*)(void *, size_t, std::align_val_t))::operator delete[]);
  ASAN_INTERCEPT_NEW_OP("_ZdlPv", (void (*)(void *))::operator delete);
  ASAN_INTERCEPT_NEW_OP("_ZdlPvRKSt9nothrow_t", (void (*)(void *, const std::nothrow_t &))::operator delete);
  ASAN_INTERCEPT_NEW_OP("_ZdlPvSt11align_val_t", (void (*)(void *, std::align_val_t))::operator delete);
  ASAN_INTERCEPT_NEW_OP("_ZdlPvSt11align_val_tRKSt9nothrow_t", (void (*)(void *, std::align_val_t, const std::nothrow_t&))::operator delete);
  ASAN_INTERCEPT_NEW_OP("_ZdlPvm", (void (*)(void *, size_t))::operator delete);
  ASAN_INTERCEPT_NEW_OP("_ZdlPvmSt11align_val_t", (void (*)(void *, size_t, std::align_val_t))::operator delete);
  ASAN_INTERCEPT_NEW_OP("_Znam", (void *(*)(size_t))::operator new[]);
  ASAN_INTERCEPT_NEW_OP("_ZnamRKSt9nothrow_t", (void *(*)(size_t, const std::nothrow_t &))::operator new[]);
  ASAN_INTERCEPT_NEW_OP("_ZnamSt11align_val_t", (void *(*)(size_t, std::align_val_t))::operator new[]);
  ASAN_INTERCEPT_NEW_OP("_ZnamSt11align_val_tRKSt9nothrow_t", (void *(*)(size_t, std::align_val_t, const std::nothrow_t &))::operator new[]);
  ASAN_INTERCEPT_NEW_OP("_Znwm", (void *(*)(size_t))::operator new);
  ASAN_INTERCEPT_NEW_OP("_ZnwmRKSt9nothrow_t", (void *(*)(size_t, const std::nothrow_t &))::operator new);
  ASAN_INTERCEPT_NEW_OP("_ZnwmSt11align_val_t", (void *(*)(size_t, std::align_val_t))::operator new);
  ASAN_INTERCEPT_NEW_OP("_ZnwmSt11align_val_tRKSt9nothrow_t", (void *(*)(size_t, std::align_val_t, const std::nothrow_t &))::operator new);
#  endif
}

#endif
