// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___THREAD_SUPPORT_PTHREAD_H
#define _LIBCPP___THREAD_SUPPORT_PTHREAD_H

#include <__chrono/convert_to_timespec.h>
#include <__chrono/duration.h>
#include <__config>
#include <ctime>
#include <errno.h>
#include <pthread.h>
#include <sched.h>

#ifdef __MVS__
#  include <__support/ibm/nanosleep.h>
#endif

// Some platforms require <bits/atomic_wide_counter.h> in order for
// PTHREAD_COND_INITIALIZER to be expanded. Normally that would come
// in via <pthread.h>, but it's a non-modular header on those platforms,
// so libc++'s <math.h> usually absorbs atomic_wide_counter.h into the
// module with <math.h> and makes atomic_wide_counter.h invisible.
// Include <math.h> here to work around that.
// This checks wheter a Clang module is built
#if __building_module(std)
#  include <math.h>
#endif

#ifndef _LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

using __libcpp_timespec_t _LIBCPP_NODEBUG = ::timespec;

#ifdef __CYGWIN__
template <typename __bare_pthread_t, uintptr_t __init_value>
struct _LIBCPP_HIDE_FROM_ABI __libcpp_cygwin_pthread_wrapper_t {
  uintptr_t __opaque_value                                                                       = __init_value;
  constexpr __libcpp_cygwin_pthread_wrapper_t() noexcept                                         = default;
  constexpr __libcpp_cygwin_pthread_wrapper_t(const __libcpp_cygwin_pthread_wrapper_t&) noexcept = default;
  __libcpp_cygwin_pthread_wrapper_t(__bare_pthread_t __m) noexcept : __opaque_value(reinterpret_cast<uintptr_t>(__m)) {}
  constexpr __libcpp_cygwin_pthread_wrapper_t& operator=(const __libcpp_cygwin_pthread_wrapper_t&) noexcept = default;
  __libcpp_cygwin_pthread_wrapper_t& operator=(__bare_pthread_t __m) noexcept {
    return operator=(__libcpp_cygwin_pthread_wrapper_t(__m));
  }
  // operator __bare_pthread_t &() noexcept { return *operator &(); }
  // operator const __bare_pthread_t &() const noexcept { return *operator &(); }
  //__bare_pthread_t *operator &() noexcept { return reinterpret_cast<__bare_pthread_t *>(&__opaque_value); }
  // const __bare_pthread_t *operator &() const noexcept { return reinterpret_cast<const __bare_pthread_t
  // *>(&__opaque_value); }
  constexpr bool operator==(const __libcpp_cygwin_pthread_wrapper_t& __m) const noexcept = default;
  constexpr friend bool
  operator<(const __libcpp_cygwin_pthread_wrapper_t& __l, const __libcpp_cygwin_pthread_wrapper_t& __r) noexcept {
    return __l.__opaque_value < __r.__opaque_value;
  }
  constexpr friend bool operator<(const __libcpp_cygwin_pthread_wrapper_t& __l, const __bare_pthread_t& __r) noexcept {
    return __l < __libcpp_cygwin_pthread_wrapper_t(__r);
  }
  constexpr friend bool operator<(const __bare_pthread_t& __l, const __libcpp_cygwin_pthread_wrapper_t& __r) noexcept {
    return __libcpp_cygwin_pthread_wrapper_t(__l) < __r;
  }
};
template <typename __T>
inline _LIBCPP_HIDE_FROM_ABI auto __libcpp_maybe_cast_to_bare_pthread(__T __t) {
  return __t;
}
template <typename __bare_pthread_t, uintptr_t __init_value>
inline _LIBCPP_HIDE_FROM_ABI __bare_pthread_t*
__libcpp_maybe_cast_to_bare_pthread(__libcpp_cygwin_pthread_wrapper_t<__bare_pthread_t, __init_value>* __m) {
  return reinterpret_cast<__bare_pthread_t*>(__m);
}
template <typename __bare_pthread_t, uintptr_t __init_value>
inline _LIBCPP_HIDE_FROM_ABI __bare_pthread_t&
__libcpp_maybe_cast_to_bare_pthread(__libcpp_cygwin_pthread_wrapper_t<__bare_pthread_t, __init_value>& __m) {
  return *reinterpret_cast<__bare_pthread_t*>(&__m);
}
#  define __WRAP_CYGPTHR_API(name)                                                                                     \
    template <typename... __Ts>                                                                                        \
    inline _LIBCPP_HIDE_FROM_ABI decltype(auto) name(__Ts&&... __vs) {                                                 \
      return ::name(__libcpp_maybe_cast_to_bare_pthread(__vs)...);                                                     \
    }
#endif

//
// Mutex
//
#ifdef __CYGWIN__
typedef __libcpp_cygwin_pthread_wrapper_t<pthread_mutex_t, 19> __libcpp_mutex_t;
#  define _LIBCPP_MUTEX_INITIALIZER                                                                                    \
    {                                                                                                                  \
    }
__WRAP_CYGPTHR_API(pthread_mutexattr_init)
__WRAP_CYGPTHR_API(pthread_mutexattr_settype)
__WRAP_CYGPTHR_API(pthread_mutexattr_destroy)
__WRAP_CYGPTHR_API(pthread_mutex_init)
__WRAP_CYGPTHR_API(pthread_mutex_destroy)
__WRAP_CYGPTHR_API(pthread_mutex_lock)
__WRAP_CYGPTHR_API(pthread_mutex_trylock)
__WRAP_CYGPTHR_API(pthread_mutex_unlock)

#else
typedef pthread_mutex_t __libcpp_mutex_t;
#  define _LIBCPP_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif

typedef __libcpp_mutex_t __libcpp_recursive_mutex_t;

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_recursive_mutex_init(__libcpp_recursive_mutex_t* __m) {
  pthread_mutexattr_t __attr;
  int __ec = pthread_mutexattr_init(&__attr);
  if (__ec)
    return __ec;
  __ec = pthread_mutexattr_settype(&__attr, PTHREAD_MUTEX_RECURSIVE);
  if (__ec) {
    pthread_mutexattr_destroy(&__attr);
    return __ec;
  }
  __ec = pthread_mutex_init(__m, &__attr);
  if (__ec) {
    pthread_mutexattr_destroy(&__attr);
    return __ec;
  }
  __ec = pthread_mutexattr_destroy(&__attr);
  if (__ec) {
    pthread_mutex_destroy(__m);
    return __ec;
  }
  return 0;
}

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_NO_THREAD_SAFETY_ANALYSIS int
__libcpp_recursive_mutex_lock(__libcpp_recursive_mutex_t* __m) {
  return pthread_mutex_lock(__m);
}

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_NO_THREAD_SAFETY_ANALYSIS bool
__libcpp_recursive_mutex_trylock(__libcpp_recursive_mutex_t* __m) {
  return pthread_mutex_trylock(__m) == 0;
}

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_NO_THREAD_SAFETY_ANALYSIS int
__libcpp_recursive_mutex_unlock(__libcpp_recursive_mutex_t* __m) {
  return pthread_mutex_unlock(__m);
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_recursive_mutex_destroy(__libcpp_recursive_mutex_t* __m) {
  return pthread_mutex_destroy(__m);
}

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_NO_THREAD_SAFETY_ANALYSIS int __libcpp_mutex_lock(__libcpp_mutex_t* __m) {
  return pthread_mutex_lock(__m);
}

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_NO_THREAD_SAFETY_ANALYSIS bool __libcpp_mutex_trylock(__libcpp_mutex_t* __m) {
  return pthread_mutex_trylock(__m) == 0;
}

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_NO_THREAD_SAFETY_ANALYSIS int __libcpp_mutex_unlock(__libcpp_mutex_t* __m) {
  return pthread_mutex_unlock(__m);
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_mutex_destroy(__libcpp_mutex_t* __m) { return pthread_mutex_destroy(__m); }

//
// Condition Variable
//
#ifdef __CYGWIN__
typedef __libcpp_cygwin_pthread_wrapper_t<pthread_cond_t, 21> __libcpp_condvar_t;
#  define _LIBCPP_CONDVAR_INITIALIZER                                                                                  \
    {                                                                                                                  \
    }
__WRAP_CYGPTHR_API(pthread_cond_signal)
__WRAP_CYGPTHR_API(pthread_cond_broadcast)
__WRAP_CYGPTHR_API(pthread_cond_wait)
__WRAP_CYGPTHR_API(pthread_cond_timedwait)
__WRAP_CYGPTHR_API(pthread_cond_destroy)
#else
typedef pthread_cond_t __libcpp_condvar_t;
#  define _LIBCPP_CONDVAR_INITIALIZER PTHREAD_COND_INITIALIZER
#endif

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_condvar_signal(__libcpp_condvar_t* __cv) { return pthread_cond_signal(__cv); }

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_condvar_broadcast(__libcpp_condvar_t* __cv) {
  return pthread_cond_broadcast(__cv);
}

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_NO_THREAD_SAFETY_ANALYSIS int
__libcpp_condvar_wait(__libcpp_condvar_t* __cv, __libcpp_mutex_t* __m) {
  return pthread_cond_wait(__cv, __m);
}

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_NO_THREAD_SAFETY_ANALYSIS int
__libcpp_condvar_timedwait(__libcpp_condvar_t* __cv, __libcpp_mutex_t* __m, __libcpp_timespec_t* __ts) {
  return pthread_cond_timedwait(__cv, __m, __ts);
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_condvar_destroy(__libcpp_condvar_t* __cv) {
  return pthread_cond_destroy(__cv);
}

//
// Execute once
//
#ifdef __CYGWIN
typedef struct __libcpp_exec_once_flag {
  __libcpp_mutex_t mutex;
  int state;
};
#  define _LIBCPP_EXEC_ONCE_INITIALIZER {{}, 0}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_execute_once(__libcpp_exec_once_flag* __flag, void (*__init_routine)()) {
  return pthread_once(reinterpret_cast<pthread_once_t*>(__flag), __init_routine);
}

#else
typedef pthread_once_t __libcpp_exec_once_flag;
#  define _LIBCPP_EXEC_ONCE_INITIALIZER PTHREAD_ONCE_INIT

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_execute_once(__libcpp_exec_once_flag* __flag, void (*__init_routine)()) {
  return pthread_once(__flag, __init_routine);
}
#endif

//
// Thread id
//
#if defined(__MVS__)
typedef unsigned long long __libcpp_thread_id;
#else
#  ifdef __CYGWIN__
typedef uintptr_t __libcpp_thread_id;
#  else
typedef pthread_t __libcpp_thread_id;
#  endif
#endif

// Returns non-zero if the thread ids are equal, otherwise 0
inline _LIBCPP_HIDE_FROM_ABI bool __libcpp_thread_id_equal(__libcpp_thread_id __t1, __libcpp_thread_id __t2) {
  return __t1 == __t2;
}

// Returns non-zero if t1 < t2, otherwise 0
inline _LIBCPP_HIDE_FROM_ABI bool __libcpp_thread_id_less(__libcpp_thread_id __t1, __libcpp_thread_id __t2) {
  return __t1 < __t2;
}

//
// Thread
//
#define _LIBCPP_NULL_THREAD ((__libcpp_thread_t()))

#ifdef __CYGWIN__
typedef __libcpp_cygwin_pthread_wrapper_t<pthread_t, 0> __libcpp_thread_t;
__WRAP_CYGPTHR_API(pthread_create)
__WRAP_CYGPTHR_API(pthread_join)
__WRAP_CYGPTHR_API(pthread_detach)
#else
typedef pthread_t __libcpp_thread_t;
#endif

inline _LIBCPP_HIDE_FROM_ABI __libcpp_thread_id __libcpp_thread_get_id(const __libcpp_thread_t* __t) {
#if defined(__MVS__)
  return __t->__;
#else
#  ifdef __CYGWIN__
  return __t->__opaque_value;
#  else
  return *__t;
#  endif
#endif
}

inline _LIBCPP_HIDE_FROM_ABI bool __libcpp_thread_isnull(const __libcpp_thread_t* __t) {
  return __libcpp_thread_get_id(__t) == 0;
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_thread_create(__libcpp_thread_t* __t, void* (*__func)(void*), void* __arg) {
  return pthread_create(__t, nullptr, __func, __arg);
}

inline _LIBCPP_HIDE_FROM_ABI __libcpp_thread_id __libcpp_thread_get_current_id() {
  const __libcpp_thread_t __current_thread = pthread_self();
  return __libcpp_thread_get_id(&__current_thread);
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_thread_join(__libcpp_thread_t* __t) { return pthread_join(*__t, nullptr); }

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_thread_detach(__libcpp_thread_t* __t) { return pthread_detach(*__t); }

inline _LIBCPP_HIDE_FROM_ABI void __libcpp_thread_yield() { sched_yield(); }

inline _LIBCPP_HIDE_FROM_ABI void __libcpp_thread_sleep_for(const chrono::nanoseconds& __ns) {
  __libcpp_timespec_t __ts = std::__convert_to_timespec<__libcpp_timespec_t>(__ns);
  while (nanosleep(&__ts, &__ts) == -1 && errno == EINTR)
    ;
}

//
// Thread local storage
//
#define _LIBCPP_TLS_DESTRUCTOR_CC /* nothing */

#ifdef __CYGWIN__
typedef __libcpp_cygwin_pthread_wrapper_t<pthread_key_t, 0> __libcpp_tls_key;
__WRAP_CYGPTHR_API(pthread_key_create)
__WRAP_CYGPTHR_API(pthread_getspecific)
__WRAP_CYGPTHR_API(pthread_setspecific)
#else
typedef pthread_key_t __libcpp_tls_key;
#endif

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_tls_create(__libcpp_tls_key* __key, void (*__at_exit)(void*)) {
  return pthread_key_create(__key, __at_exit);
}

inline _LIBCPP_HIDE_FROM_ABI void* __libcpp_tls_get(__libcpp_tls_key __key) { return pthread_getspecific(__key); }

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_tls_set(__libcpp_tls_key __key, void* __p) {
  return pthread_setspecific(__key, __p);
}

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___THREAD_SUPPORT_PTHREAD_H
